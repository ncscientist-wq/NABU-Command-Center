#!/usr/bin/env python3
"""Bounded official NOAA/SWPC publisher for NABU Command Center."""

from __future__ import annotations

import configparser
import hashlib
import time
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

import weather_gateway

RESOURCE_NAME = "ncc_space.dat"
KP_URL = "https://services.swpc.noaa.gov/products/noaa-planetary-k-index.json"
WIND_URL = "https://services.swpc.noaa.gov/products/geospace/propagated-solar-wind-1-hour.json"
MAG_URL = "https://services.swpc.noaa.gov/products/summary/solar-wind-mag-field.json"
FLUX_URL = "https://services.swpc.noaa.gov/products/summary/10cm-flux.json"


def _bounded(value: float, scale: float, maximum: int = 255) -> int:
    return max(0, min(maximum, round(value * scale)))


def _latest_rows(rows: list[Any], count: int = 12) -> list[Any]:
    return rows[-count:] if len(rows) > count else rows


def _utc(value: str) -> int:
    text = value.replace("Z", "+00:00")
    parsed = datetime.fromisoformat(text)
    if parsed.tzinfo is None:
        parsed = parsed.replace(tzinfo=timezone.utc)
    return int(parsed.timestamp())


def normalize(kp_data: list[Any], wind_data: list[Any], mag_data: list[Any], flux_data: list[Any]) -> dict[str, Any]:
    kp_rows = [row for row in kp_data if isinstance(row, dict) and row.get("Kp") is not None]
    if not kp_rows:
        raise ValueError("NOAA Kp product has no observations")
    if not wind_data or not isinstance(wind_data[0], list):
        raise ValueError("NOAA propagated solar-wind product has no header")
    header = wind_data[0]
    speed_index = header.index("speed")
    time_index = header.index("time_tag")
    wind_rows = [row for row in wind_data[1:] if isinstance(row, list) and len(row) > speed_index and row[speed_index] not in (None, "")]
    if not wind_rows:
        raise ValueError("NOAA propagated solar-wind product has no observations")
    kp_history = [_bounded(float(row["Kp"]), 10.0, 90) for row in _latest_rows(kp_rows)]
    wind_history = [max(0, min(65535, round(float(row[speed_index])))) for row in _latest_rows(wind_rows)]
    mag = mag_data[0] if mag_data and isinstance(mag_data[0], dict) else {}
    flux = flux_data[0] if flux_data and isinstance(flux_data[0], dict) else {}
    bz = max(-128, min(127, round(float(mag.get("bz_gsm", 0)) * 10.0)))
    return {
        "kp10": kp_history[-1], "speed": wind_history[-1],
        "bt10": _bounded(float(mag.get("bt", 0)), 10.0), "bz10": bz,
        "flux": _bounded(float(flux.get("flux", 0)), 1.0),
        "kp_history": kp_history, "wind_history": wind_history,
        "utc": _utc(str(wind_rows[-1][time_index])),
    }


def build_record(sequence: int, state: dict[str, Any]) -> bytes:
    record = bytearray(64)
    count = min(12, len(state["kp_history"]), len(state["wind_history"]))
    record[:4] = b"SW01"
    record[4] = sequence & 0xFF
    record[5] = state["kp10"]
    record[6:8] = int(state["speed"]).to_bytes(2, "little")
    record[8] = state["bt10"]
    record[9] = int(state["bz10"]) & 0xFF
    record[10] = state["flux"]
    record[11] = count
    for index in range(count):
        record[12 + index] = state["kp_history"][-count + index]
        offset = 24 + index * 2
        record[offset:offset + 2] = int(state["wind_history"][-count + index]).to_bytes(2, "little")
    record[48:52] = int(state["utc"]).to_bytes(4, "little")
    record[52] = 2 if state["kp10"] >= 50 else (1 if state["kp10"] >= 40 else 0)
    record[60:62] = (sum(record[:60]) & 0xFFFF).to_bytes(2, "little")
    record[62], record[63] = ord("E"), 10
    return bytes(record)


def publish(config_path: Path, sequence: int = 1) -> bytes:
    config = configparser.ConfigParser()
    if not config.read(config_path, encoding="utf-8"):
        raise FileNotFoundError(config_path)
    section = config["weather"]
    user_agent = section["user_agent"].strip()
    timeout = section.getfloat("timeout_seconds", 15.0)
    state = normalize(
        weather_gateway.get_json(KP_URL, user_agent, timeout),
        weather_gateway.get_json(WIND_URL, user_agent, timeout),
        weather_gateway.get_json(MAG_URL, user_agent, timeout),
        weather_gateway.get_json(FLUX_URL, user_agent, timeout),
    )
    record = build_record(sequence, state)
    weather_gateway.atomic_write(Path(section["store_path"]) / RESOURCE_NAME, record)
    print(f"published SPACE WEATHER kp={state['kp10']/10:.1f} wind={state['speed']} bytes=64 sha256={hashlib.sha256(record).hexdigest().upper()}", flush=True)
    return record


def run(config_path: Path, stop_event: Any | None = None) -> int:
    sequence = 1
    while stop_event is None or not stop_event.is_set():
        try:
            publish(config_path, sequence)
            sequence = (sequence + 1) & 0xFF
        except Exception as exc:
            print(f"space-weather acquisition failed: {exc}; prior SPACE WEATHER preserved", flush=True)
        if stop_event is None:
            return 0
        stop_event.wait(300.0)
    return 0


if __name__ == "__main__":
    raise SystemExit(run(Path("weather.ini")))
