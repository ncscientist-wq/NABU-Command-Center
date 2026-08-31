#!/usr/bin/env python3
"""Bounded official NOAA/NWS active-alert publisher for Command Center."""

from __future__ import annotations

import configparser
import hashlib
import time
from pathlib import Path
from typing import Any

import weather_gateway

RESOURCE_NAME = "ncc_alert.dat"
ALERT_URL = "https://api.weather.gov/alerts/active?point={lat:.6f},{lon:.6f}"
SEVERITY = {"Unknown": 0, "Minor": 1, "Moderate": 2, "Severe": 3, "Extreme": 4}
TEXT_CAPACITY = 38


def _clean(value: Any) -> str:
    text = " ".join(str(value or "").split()).encode("ascii", "replace").decode("ascii")
    return text.replace("|", "/")


def normalize(payload: dict[str, Any]) -> dict[str, Any]:
    features = payload.get("features", [])
    alerts: list[tuple[int, str, str]] = []
    for feature in features if isinstance(features, list) else []:
        props = feature.get("properties", {}) if isinstance(feature, dict) else {}
        severity_text = str(props.get("severity", "Unknown"))
        severity = SEVERITY.get(severity_text, 0)
        headline = _clean(props.get("headline") or props.get("event"))
        if headline:
            alerts.append((severity, severity_text.upper(), headline))
    alerts.sort(key=lambda item: item[0], reverse=True)
    if not alerts:
        return {"severity": 0, "count": 0, "text": "NO ACTIVE NOAA/NWS ALERTS"}
    severity, label, headline = alerts[0]
    return {"severity": severity, "count": min(255, len(alerts)),
            "text": f"{label}: {headline}"[:TEXT_CAPACITY]}


def build_record(sequence: int, token: str, zip_code: str, state: dict[str, Any], source_utc: int | None = None) -> bytes:
    text = _clean(state["text"])[:TEXT_CAPACITY].encode("ascii")
    record = bytearray(64)
    record[:4] = b"WA01"
    record[4:10] = token.encode("ascii")
    record[10:15] = zip_code.encode("ascii")
    record[15] = sequence & 0xFF
    record[16] = int(state["severity"]) & 0xFF
    record[17] = int(state["count"]) & 0xFF
    record[18] = len(text)
    record[19:23] = int(source_utc if source_utc is not None else time.time()).to_bytes(4, "little")
    record[23:23 + len(text)] = text
    record[61:63] = (sum(record[:61]) & 0xFFFF).to_bytes(2, "little")
    record[63] = 10
    return bytes(record)


def run(config_path: Path, stop_event: Any | None = None) -> int:
    config = configparser.ConfigParser()
    if not config.read(config_path, encoding="utf-8"):
        raise FileNotFoundError(config_path)
    section = config["weather"]
    store = Path(section["store_path"])
    request_path = store / "ncc_zip.req"
    zcta = weather_gateway.load_zcta(Path(section["gazetteer_path"]))
    user_agent = section["user_agent"].strip()
    timeout = section.getfloat("timeout_seconds", 15.0)
    refresh = max(30.0, section.getfloat("alert_refresh_seconds", 300.0))
    sequence, last_signature, last_publish = 1, None, 0.0
    while stop_event is None or not stop_event.is_set():
        try:
            signature = weather_gateway.request_signature(request_path)
            now = time.monotonic()
            if signature != last_signature or now - last_publish >= refresh:
                token, zip_code = weather_gateway.parse_request(request_path.read_bytes())
                if zip_code not in zcta:
                    raise ValueError(f"ZIP {zip_code} unresolved")
                lat, lon = zcta[zip_code]
                state = normalize(weather_gateway.get_json(ALERT_URL.format(lat=lat, lon=lon), user_agent, timeout))
                record = build_record(sequence, token, zip_code, state)
                weather_gateway.atomic_write(store / RESOURCE_NAME, record)
                print(f"published WEATHER ALERT ZIP={zip_code} active={state['count']} bytes=64 sha256={hashlib.sha256(record).hexdigest().upper()}", flush=True)
                sequence = (sequence + 1) & 0xFF
                last_signature, last_publish = signature, now
        except FileNotFoundError:
            pass
        except Exception as exc:
            print(f"weather-alert acquisition failed: {exc}; prior WEATHER ALERT preserved", flush=True)
        if stop_event is None:
            return 0
        stop_event.wait(0.25)
    return 0


if __name__ == "__main__":
    raise SystemExit(run(Path("weather.ini")))
