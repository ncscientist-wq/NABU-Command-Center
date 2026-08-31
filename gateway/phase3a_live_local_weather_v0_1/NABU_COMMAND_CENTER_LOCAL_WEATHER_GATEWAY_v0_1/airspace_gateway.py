"""Bounded live ADS-B publisher for the NABU Command Center Gateway."""

from __future__ import annotations

import configparser
import math
import os
import struct
from pathlib import Path
from typing import Any, Callable, Mapping

import weather_gateway

AIRSPACE_RADIUS_NM = 100
RESOURCE_NAME = "ncc_airspace.dat"
RECORD_SIZE = 64
MAX_AIRCRAFT = 3
SLOT_SIZE = 17
STATE_LIVE, STATE_STALE, STATE_OFFLINE = 1, 2, 3
SOURCE_LOCAL, SOURCE_ADSBLOL = 1, 2
DEFAULT_REFRESH_SECONDS = 30.0
MIN_DISPLAY_RADIUS_NM = 10.0
DISPLAY_MARGIN_FRACTION = 0.82
PLOT_CENTER_X, PLOT_CENTER_Y = 90, 105
PLOT_RADIUS_X, PLOT_RADIUS_Y = 72, 43


def current_profile_center(weather_config: Path) -> tuple[str, float, float]:
    config = configparser.ConfigParser()
    if not config.read(weather_config, encoding="utf-8"):
        raise FileNotFoundError(weather_config)
    section = config["weather"]
    store = Path(section["store_path"])
    _, zip_code = weather_gateway.parse_request((store / weather_gateway.REQUEST_NAME).read_bytes())
    center = weather_gateway.load_zcta(Path(section["gazetteer_path"])).get(zip_code)
    if center is None:
        raise ValueError(f"current ZIP {zip_code} is unresolved")
    return zip_code, center[0], center[1]


def fetch_aircraft(latitude: float, longitude: float, timeout: float = 15.0,
                   local_url: str | None = None) -> tuple[str, Mapping[str, Any]]:
    if local_url:
        url, source = local_url, "LOCAL"
    else:
        url = f"https://api.adsb.lol/v2/point/{latitude:.6f}/{longitude:.6f}/{AIRSPACE_RADIUS_NM}"
        source = "ADSBLOL"
    payload = weather_gateway.get_json(url, "NABU-Command-Center/1.0", timeout)
    if not isinstance(payload, dict) or not isinstance(payload.get("ac"), list):
        raise ValueError("ADS-B provider response lacks aircraft list")
    return source, payload


def _number(value: Any) -> float | None:
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        return None
    result = float(value)
    return result if math.isfinite(result) else None


def _safe_ascii(value: Any, maximum: int) -> str:
    text = str(value or "").strip().upper()
    return "".join(ch for ch in text if "A" <= ch <= "Z" or "0" <= ch <= "9")[:maximum]


def _range_bearing(lat1: float, lon1: float, lat2: float, lon2: float) -> tuple[float, float]:
    p1, p2 = math.radians(lat1), math.radians(lat2)
    dp, dl = math.radians(lat2 - lat1), math.radians(lon2 - lon1)
    a = math.sin(dp / 2) ** 2 + math.cos(p1) * math.cos(p2) * math.sin(dl / 2) ** 2
    distance = 3440.065 * 2 * math.atan2(math.sqrt(a), math.sqrt(max(0.0, 1.0 - a)))
    y = math.sin(dl) * math.cos(p2)
    x = math.cos(p1) * math.sin(p2) - math.sin(p1) * math.cos(p2) * math.cos(dl)
    return distance, (math.degrees(math.atan2(y, x)) + 360.0) % 360.0


def reduce_aircraft(payload: Mapping[str, Any], center: tuple[float, float]) -> list[dict[str, Any]]:
    retained = []
    for raw in payload.get("ac", []):
        if not isinstance(raw, dict):
            continue
        latitude, longitude = _number(raw.get("lat")), _number(raw.get("lon"))
        if latitude is None or longitude is None or not (-90 <= latitude <= 90 and -180 <= longitude <= 180):
            continue
        icao = _safe_ascii(raw.get("hex"), 6)
        if not icao:
            continue
        distance, bearing = _range_bearing(center[0], center[1], latitude, longitude)
        if distance > AIRSPACE_RADIUS_NM:
            continue
        altitude, speed, heading = _number(raw.get("alt_baro")), _number(raw.get("gs")), _number(raw.get("track"))
        retained.append({
            "call": _safe_ascii(raw.get("flight"), 6) or icao, "icao": icao,
            "alt100": max(0, min(255, round((altitude or 0.0) / 100.0))),
            "speed2": max(0, min(255, round((speed or 0.0) / 2.0))),
            "heading2": max(0, min(179, round(((heading or 0.0) % 360.0) / 2.0))),
            "distance": distance, "bearing": bearing,
        })
    retained.sort(key=lambda aircraft: aircraft["distance"])
    retained = retained[:MAX_AIRCRAFT]
    if retained:
        display_radius = min(float(AIRSPACE_RADIUS_NM),
                             max(MIN_DISPLAY_RADIUS_NM, retained[-1]["distance"] / DISPLAY_MARGIN_FRACTION))
        for aircraft in retained:
            radial = min(1.0, aircraft["distance"] / display_radius)
            angle = math.radians(aircraft["bearing"])
            aircraft["x"] = max(18, min(162, round(PLOT_CENTER_X + math.sin(angle) * radial * PLOT_RADIUS_X)))
            # Flying-bird depth: near tracks are low/wide; far tracks approach the vanishing region.
            aircraft["y"] = max(68, min(148, round(148.0 - radial * 80.0)))
            aircraft["display_radius"] = display_radius
    return retained


def build_record(aircraft: list[dict[str, Any]], sequence: int, state: int = STATE_LIVE,
                 source: int = SOURCE_ADSBLOL) -> bytes:
    if len(aircraft) > MAX_AIRCRAFT or state not in (STATE_LIVE, STATE_STALE, STATE_OFFLINE):
        raise ValueError("invalid AS01 state/count")
    record = bytearray(RECORD_SIZE)
    record[0:4] = b"AS01"
    record[4], record[5], record[6], record[7] = sequence & 0xFF, state, source, len(aircraft)
    for index, item in enumerate(aircraft):
        offset = 8 + index * SLOT_SIZE
        record[offset:offset + 6] = str(item["call"]).encode("ascii")[:6].ljust(6, b" ")
        record[offset + 6:offset + 12] = str(item["icao"]).encode("ascii")[:6].ljust(6, b" ")
        record[offset + 12:offset + 17] = bytes((item["x"], item["y"], item["alt100"], item["speed2"], item["heading2"]))
    struct.pack_into("<H", record, 60, sum(record[:60]) & 0xFFFF)
    record[62], record[63] = ord("E"), 0x0A
    return bytes(record)


def _atomic_write(destination: Path, payload: bytes) -> None:
    temporary = destination.with_suffix(destination.suffix + ".tmp")
    with temporary.open("wb") as stream:
        stream.write(payload)
        stream.flush()
        os.fsync(stream.fileno())
    os.replace(temporary, destination)


def publish_once(store: Path, sequence: int, center: tuple[float, float], local_url: str | None = None,
                 fetcher: Callable[..., tuple[str, Mapping[str, Any]]] = fetch_aircraft) -> tuple[bytes, str, int]:
    source_name, payload = fetcher(center[0], center[1], local_url=local_url)
    aircraft = reduce_aircraft(payload, center)
    source = SOURCE_LOCAL if source_name == "LOCAL" else SOURCE_ADSBLOL
    record = build_record(aircraft, sequence, STATE_LIVE, source)
    _atomic_write(store / RESOURCE_NAME, record)
    return record, source_name, len(payload["ac"])


def run(store: Path, weather_config: Path, stop_event, local_url: str | None = None) -> None:
    refresh_seconds = float(os.environ.get("NCC_AIRSPACE_REFRESH_SECONDS", DEFAULT_REFRESH_SECONDS))
    sequence, last_aircraft, last_source = 0, [], SOURCE_ADSBLOL
    destination = store / RESOURCE_NAME
    destination.parent.mkdir(parents=True, exist_ok=True)
    while not stop_event.is_set():
        try:
            zip_code, latitude, longitude = current_profile_center(weather_config)
            source_name, payload = fetch_aircraft(latitude, longitude, local_url=local_url)
            aircraft = reduce_aircraft(payload, (latitude, longitude))
            last_aircraft = aircraft
            last_source = SOURCE_LOCAL if source_name == "LOCAL" else SOURCE_ADSBLOL
            _atomic_write(destination, build_record(aircraft, sequence, STATE_LIVE, last_source))
            print(f"published AIRSPACE source={source_name} ZIP={zip_code} returned={len(payload['ac'])} retained={len(aircraft)} bytes=64", flush=True)
        except Exception as exc:
            state = STATE_STALE if last_aircraft else STATE_OFFLINE
            _atomic_write(destination, build_record(last_aircraft, sequence, state, last_source))
            print(f"AIRSPACE update failed; published {'STALE' if last_aircraft else 'OFFLINE'}: {exc}", flush=True)
        sequence = (sequence + 1) & 0xFF
        stop_event.wait(refresh_seconds)
