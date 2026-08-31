#!/usr/bin/env python3
"""Phase 3A-02B ZIP/ZCTA/NWS gateway with fixed 64-byte publication."""

from __future__ import annotations

import argparse
import configparser
import csv
import datetime as dt
import hashlib
import json
import math
import os
import re
import subprocess
import tempfile
import time
import unicodedata
from pathlib import Path
from typing import Any

REQUEST_NAME = "ncc_zip.req"
WEATHER_NAME = "ncc_weather.dat"
WEATHER_HISTORY_NAME = "ncc_wxhist.dat"
LOCATION_NAME = "ncc_location.dat"
LOCATION_CITY_MAX = 16
REQUEST_RE = re.compile(rb"ZIP\|([0-9]{6})\|([0-9]{5})\n\Z")
GAZETTEER_NAME = "2025_Gaz_zcta_national.txt"
GAZETTEER_ZIP_NAME = "2025_Gaz_zcta_national.zip"
GAZETTEER_SHA256 = "51516A4283BAB5CD2376EEC75609DDC4B363A18297E8ADEEAAC7B03CF7C84DBE"
GAZETTEER_URL = "https://www2.census.gov/geo/docs/maps-data/data/gazetteer/2025_Gazetteer/2025_Gaz_zcta_national.zip"
CONDITION_MAP = (
    ("THUNDER", "STORMS"), ("T-STORM", "STORMS"), ("SNOW", "SNOW"),
    ("SLEET", "SNOW"), ("RAIN", "RAIN"), ("DRIZZLE", "RAIN"),
    ("SHOWER", "RAIN"), ("FOG", "FOG"), ("HAZE", "FOG"),
    ("WIND", "WIND"), ("CLEAR", "CLEAR"), ("SUNNY", "CLEAR"),
    ("CLOUD", "CLOUDY"), ("OVERCAST", "CLOUDY"),
)


def checksum16(body: bytes) -> int:
    return sum(body) & 0xFFFF


def round_half_away(value: float) -> int:
    return math.floor(value + 0.5) if value >= 0 else math.ceil(value - 0.5)


def normalize_condition(text: str) -> str:
    upper = "".join(ch for ch in text.upper() if 0x20 <= ord(ch) <= 0x7E)
    for needle, result in CONDITION_MAP:
        if needle in upper:
            return result.ljust(8, "_")
    return "UNKNOWN_"


def normalize_location(city: Any, state: Any) -> tuple[str, str] | None:
    if not isinstance(city, str) or not isinstance(state, str):
        return None
    city = unicodedata.normalize("NFKD", city).encode("ascii", "ignore").decode("ascii").upper()
    city = re.sub(r"[^A-Z0-9 -]+", " ", city)
    city = re.sub(r"\s+", " ", city).strip(" -")[:LOCATION_CITY_MAX].rstrip()
    state = state.strip().upper()
    if not city or not re.fullmatch(r"[A-Z]{2}", state):
        return None
    return city, state


def point_location(properties: dict[str, Any]) -> tuple[str, str] | None:
    relative = properties.get("relativeLocation")
    if not isinstance(relative, dict):
        return None
    location = relative.get("properties")
    if not isinstance(location, dict):
        return None
    return normalize_location(location.get("city"), location.get("state"))


def build_location_record(token: str, zip_code: str, city: str = "UNKNOWN",
                          state: str = "--", resolved: bool = False) -> bytes:
    if not re.fullmatch(r"[0-9]{6}", token):
        raise ValueError("token must be six digits")
    if not re.fullmatch(r"[0-9]{5}", zip_code):
        raise ValueError("ZIP/ZCTA must be five digits")
    if resolved:
        normalized = normalize_location(city, state)
        if normalized is None:
            raise ValueError("resolved location requires a bounded city and two-letter state")
        city, state = normalized
    else:
        city, state = "UNKNOWN", "--"
    record = bytearray(64)
    record[0:4] = b"LC01"
    record[4:10] = token.encode("ascii")
    record[10:15] = zip_code.encode("ascii")
    record[15] = 1 if resolved else 0
    record[16] = len(city)
    record[17] = len(state)
    record[18:18 + len(city)] = city.encode("ascii")
    record[34:36] = state.encode("ascii")
    record[57:59] = (sum(record[:57]) & 0xFFFF).to_bytes(2, "little")
    record[59:63] = b"END!"
    record[63] = 10
    return bytes(record)


def build_weather_record(token: str, zip_code: str, temperature_f: int,
                         condition: str, wind_mph: int, pressure_hpa: int,
                         source_utc: int) -> bytes:
    if not re.fullmatch(r"[0-9]{6}", token):
        raise ValueError("token must be six digits")
    if not re.fullmatch(r"[0-9]{5}", zip_code):
        raise ValueError("ZIP/ZCTA must be five digits")
    if not -99 <= temperature_f <= 999:
        raise ValueError("temperature outside three-character field")
    if not 0 <= wind_mph <= 999:
        raise ValueError("wind outside three-digit field")
    if not 0 <= pressure_hpa <= 9999:
        raise ValueError("pressure outside four-digit field")
    if not 0 <= source_utc <= 4294967295:
        raise ValueError("source timestamp outside uint32")
    condition = normalize_condition(condition) if len(condition) != 8 else condition
    if not re.fullmatch(r"[ -~]{8}", condition) or "|" in condition:
        raise ValueError("condition must be eight safe ASCII characters")
    body = (
        f"NCC|1|WX|{token}|{zip_code}|{temperature_f:03d}|{condition}|"
        f"{wind_mph:03d}|{pressure_hpa:04d}|{source_utc:010d}|"
    ).encode("ascii")
    record = body + f"{checksum16(body):04X}|END\n".encode("ascii")
    if len(record) != 64:
        raise AssertionError(f"WEATHER record is {len(record)}, expected 64")
    return record


def build_history_record(token: str, zip_code: str,
                         samples: list[tuple[int, int, int]], source_utc: int) -> bytes:
    """Build one bounded binary record containing up to 12 real observations."""
    if not re.fullmatch(r"[0-9]{6}", token) or not re.fullmatch(r"[0-9]{5}", zip_code):
        raise ValueError("history identity is invalid")
    selected = samples[-12:]
    record = bytearray(64)
    record[0:4] = b"WXH1"
    record[4:10] = token.encode("ascii")
    record[10:15] = zip_code.encode("ascii")
    record[15] = len(selected)
    record[16] = len(selected) - 1 if selected else 0
    record[17:21] = int(source_utc).to_bytes(4, "little")
    for index, (temperature_f, pressure_hpa, wind_mph) in enumerate(selected):
        offset = 21 + index * 3
        record[offset] = max(0, min(255, temperature_f + 100))
        record[offset + 1] = max(0, min(255, pressure_hpa - 900))
        record[offset + 2] = max(0, min(255, wind_mph))
    integrity = sum(record[:57]) & 0xFFFF
    record[57:59] = integrity.to_bytes(2, "little")
    record[59:63] = b"END!"
    record[63] = 10
    return bytes(record)


def atomic_write(path: Path, content: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, name = tempfile.mkstemp(prefix=path.name + ".", suffix=".tmp", dir=path.parent)
    try:
        with os.fdopen(fd, "wb") as stream:
            stream.write(content)
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(name, path)
    except BaseException:
        try:
            os.unlink(name)
        except FileNotFoundError:
            pass
        raise


def load_zcta(path: Path) -> dict[str, tuple[float, float]]:
    result: dict[str, tuple[float, float]] = {}
    with path.open("r", encoding="ascii", newline="") as stream:
        rows = csv.DictReader(stream, delimiter="|")
        expected = {"GEOID", "INTPTLAT", "INTPTLONG"}
        if rows.fieldnames is None or not expected.issubset(rows.fieldnames):
            raise ValueError("Census Gazetteer columns missing")
        for row in rows:
            code = row["GEOID"]
            if not re.fullmatch(r"[0-9]{5}", code):
                raise ValueError(f"invalid Gazetteer GEOID {code!r}")
            result[code] = (float(row["INTPTLAT"]), float(row["INTPTLONG"]))
    if not result:
        raise ValueError("Census Gazetteer is empty")
    return result


def get_json(url: str, user_agent: str, timeout: float) -> dict[str, Any]:
    # The installed Python/OpenSSL trust path rejects the locally intercepted
    # chain. Use the installed Windows curl/Schannel client; verification stays on.
    curl = Path(r"C:\Windows\System32\curl.exe")
    if not curl.is_file():
        raise FileNotFoundError(curl)
    result = subprocess.run(
        [str(curl), "--fail", "--location", "--silent", "--show-error",
         "--max-time", str(max(1, int(timeout))),
         "-H", f"User-Agent: {user_agent}",
         "-H", "Accept: application/geo+json, application/json", url],
        check=True, capture_output=True, text=True, encoding="utf-8",
    )
    return json.loads(result.stdout)


def qv_value(properties: dict[str, Any], name: str, expected_unit: str) -> float | None:
    field = properties.get(name)
    if not isinstance(field, dict) or field.get("value") is None:
        return None
    if field.get("unitCode") != expected_unit:
        raise ValueError(f"unexpected {name} unit {field.get('unitCode')!r}")
    return float(field["value"])


def acquire_weather(latitude: float, longitude: float, user_agent: str,
                    timeout: float) -> tuple[int, str, int, int, int, str, str, str]:
    point = get_json(f"https://api.weather.gov/points/{latitude:.6f},{longitude:.6f}", user_agent, timeout)
    point_props = point["properties"]
    location = point_location(point_props)
    if location is None:
        raise ValueError("NWS point response lacks reliable city/state")
    stations_url = point_props["observationStations"]
    stations = get_json(stations_url, user_agent, timeout)
    features = stations.get("features")
    if not isinstance(features, list) or not features:
        raise ValueError("NWS returned no observation stations")
    last_error = "no station attempted"
    for feature in features[:5]:
        station_id = feature["properties"]["stationIdentifier"]
        observation = get_json(f"https://api.weather.gov/stations/{station_id}/observations/latest", user_agent, timeout)
        props = observation["properties"]
        temperature_c = qv_value(props, "temperature", "wmoUnit:degC")
        wind_kph = qv_value(props, "windSpeed", "wmoUnit:km_h-1")
        pressure_pa = qv_value(props, "barometricPressure", "wmoUnit:Pa")
        description = props.get("textDescription")
        timestamp = props.get("timestamp")
        if temperature_c is None or wind_kph is None or pressure_pa is None:
            last_error = f"required current observation field is null at {station_id}"
            continue
        if not isinstance(description, str) or not description:
            last_error = f"current condition text is unavailable at {station_id}"
            continue
        if not isinstance(timestamp, str):
            last_error = f"observation timestamp is unavailable at {station_id}"
            continue
        break
    else:
        raise ValueError(last_error)
    parsed = dt.datetime.fromisoformat(timestamp.replace("Z", "+00:00"))
    source_utc = int(parsed.timestamp())
    return (
        round_half_away(temperature_c * 9.0 / 5.0 + 32.0),
        normalize_condition(description),
        round_half_away(wind_kph / 1.609344),
        round_half_away(pressure_pa / 100.0),
        source_utc,
        station_id,
        location[0],
        location[1],
    )


def parse_request(content: bytes) -> tuple[str, str]:
    match = REQUEST_RE.fullmatch(content)
    if match is None:
        raise ValueError("ZIP request must be exact 17-byte format")
    return match.group(1).decode("ascii"), match.group(2).decode("ascii")


def request_signature(path: Path) -> tuple[int, int, bytes]:
    """Identify a write event even when a restarted client rewrites identical bytes."""
    stat = path.stat()
    content = path.read_bytes()
    return stat.st_mtime_ns, stat.st_size, content


def publish_weather(weather_path: Path, token: str, zip_code: str,
                    location: tuple[float, float], user_agent: str,
                    timeout: float, history_path: Path | None = None,
                    history: list[tuple[int, int, int]] | None = None,
                    location_path: Path | None = None) -> tuple[bytes, bytes, str]:
    weather = acquire_weather(location[0], location[1], user_agent, timeout)
    record = build_weather_record(token, zip_code, *weather[:5])
    location_record = build_location_record(token, zip_code, weather[6], weather[7], True)
    atomic_write(weather_path, record)
    if location_path is not None:
        atomic_write(location_path, location_record)
    if history_path is not None and history is not None:
        history.append((weather[0], weather[3], weather[2]))
        del history[:-12]
        atomic_write(history_path, build_history_record(token, zip_code, history, weather[4]))
    return record, location_record, weather[5]


def run(config_path: Path, once: bool, stop_event: Any | None = None) -> int:
    config = configparser.ConfigParser()
    if not config.read(config_path, encoding="utf-8"):
        raise FileNotFoundError(config_path)
    section = config["weather"]
    store = Path(section["store_path"])
    gazetteer = Path(section["gazetteer_path"])
    user_agent = section["user_agent"].strip()
    if not user_agent or "CONFIGURE" in user_agent.upper():
        raise ValueError("configure a descriptive NWS user_agent before running")
    timeout = section.getfloat("timeout_seconds", 15.0)
    poll = section.getfloat("poll_seconds", 0.10)
    refresh = section.getfloat("refresh_seconds", 900.0)
    if refresh < 60.0:
        raise ValueError("weather refresh_seconds must be at least 60")
    zcta = load_zcta(gazetteer)
    request_path = store / REQUEST_NAME
    weather_path = store / WEATHER_NAME
    location_path = store / LOCATION_NAME
    if not location_path.is_file():
        atomic_write(location_path, build_location_record("000000", "00000"))
    try:
        previous_signature: tuple[int, int, bytes] | None = request_signature(request_path)
        print("existing ZIP request ignored until a new write", flush=True)
    except (FileNotFoundError, OSError):
        previous_signature = None
    active: tuple[str, str, tuple[float, float]] | None = None
    next_refresh = float("inf")
    print(f"ZIP request resource = {request_path}", flush=True)
    print(f"WEATHER resource = {weather_path}", flush=True)
    print(f"LOCATION resource = {location_path}", flush=True)
    print(f"Census ZCTA entries = {len(zcta)}", flush=True)
    while True:
        if stop_event is not None and stop_event.is_set():
            return 0
        try:
            signature = request_signature(request_path)
        except (FileNotFoundError, OSError, ValueError):
            time.sleep(poll)
            continue
        if signature != previous_signature:
            previous_signature = signature
            try:
                token, zip_code = parse_request(signature[2])
            except ValueError as exc:
                active = None
                next_refresh = float("inf")
                print(f"invalid ZIP request ignored: {exc}", flush=True)
                if once:
                    return 1
                time.sleep(poll)
                continue
            print(f"received ZIP request = {zip_code}", flush=True)
            print(f"request sequence/token = {token}", flush=True)
            location = zcta.get(zip_code)
            active = None
            next_refresh = float("inf")
            if location is None:
                atomic_write(location_path, build_location_record(token, zip_code))
                print(f"unresolved ZCTA = {zip_code}; prior WEATHER preserved", flush=True)
            else:
                active = (token, zip_code, location)
                try:
                    record, location_record, station = publish_weather(
                        weather_path, token, zip_code, location, user_agent, timeout,
                        location_path=location_path)
                    print(f"published WEATHER ZIP={zip_code} station={station} bytes=64 sha256={hashlib.sha256(record).hexdigest().upper()}", flush=True)
                    print(f"published LOCATION ZIP={zip_code} bytes=64 sha256={hashlib.sha256(location_record).hexdigest().upper()}", flush=True)
                except Exception as exc:
                    print(f"weather acquisition failed ZIP={zip_code}: {exc}; prior WEATHER preserved", flush=True)
                next_refresh = time.monotonic() + refresh
            if once:
                return 0
        elif active is not None and time.monotonic() >= next_refresh:
            token, zip_code, location = active
            try:
                record, location_record, station = publish_weather(
                    weather_path, token, zip_code, location, user_agent, timeout,
                    location_path=location_path)
                print(f"refreshed WEATHER ZIP={zip_code} station={station} bytes=64 sha256={hashlib.sha256(record).hexdigest().upper()}", flush=True)
                print(f"refreshed LOCATION ZIP={zip_code} bytes=64 sha256={hashlib.sha256(location_record).hexdigest().upper()}", flush=True)
            except Exception as exc:
                print(f"weather refresh failed ZIP={zip_code}: {exc}; prior WEATHER preserved", flush=True)
            next_refresh = time.monotonic() + refresh
        time.sleep(poll)


def main() -> int:
    parser = argparse.ArgumentParser(description="NABU Command Center Phase 3A-02B live weather gateway")
    parser.add_argument("--config", type=Path, default=Path("weather.ini"))
    parser.add_argument("--once", action="store_true")
    args = parser.parse_args()
    return run(args.config, args.once)


if __name__ == "__main__":
    raise SystemExit(main())
