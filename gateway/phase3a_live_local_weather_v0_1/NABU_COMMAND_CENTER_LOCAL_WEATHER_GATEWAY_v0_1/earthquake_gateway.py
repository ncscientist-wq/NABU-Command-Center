#!/usr/bin/env python3
"""Bounded official-USGS Earthquake publisher for NABU Command Center."""

from __future__ import annotations

import configparser
import hashlib
import math
import re
import time
from pathlib import Path
from typing import Any

import weather_gateway

RESOURCE_NAME = "ncc_quake.dat"
USGS_FEED = "https://earthquake.usgs.gov/earthquakes/feed/v1.0/summary/2.5_week.geojson"
LOCAL_RADIUS_KM = 2000.0


def distance_km(a: tuple[float, float], b: tuple[float, float]) -> float:
    lat1, lon1, lat2, lon2 = map(math.radians, (a[0], a[1], b[0], b[1]))
    dlat, dlon = lat2 - lat1, lon2 - lon1
    h = math.sin(dlat / 2) ** 2 + math.cos(lat1) * math.cos(lat2) * math.sin(dlon / 2) ** 2
    return 6371.0 * 2.0 * math.asin(min(1.0, math.sqrt(h)))


def compact_region(place: str) -> bytes:
    part = place.rsplit(",", 1)[-1].strip().upper()
    clean = "".join(ch for ch in part if ch.isascii() and ch.isalnum())[:4]
    return clean.ljust(4, "_").encode("ascii")


def normalize(feature: dict[str, Any], now_ms: int) -> dict[str, Any]:
    props, geometry = feature["properties"], feature["geometry"]
    coords = geometry["coordinates"]
    magnitude, longitude, latitude, depth = float(props["mag"]), float(coords[0]), float(coords[1]), float(coords[2])
    when = int(props["time"])
    if not (-90 <= latitude <= 90 and -180 <= longitude <= 180 and magnitude >= 0 and depth >= 0):
        raise ValueError("USGS event outside bounded fields")
    event_id = str(feature.get("id", ""))
    return {
        "mag10": max(0, min(99, round(magnitude * 10))),
        "depth": max(0, min(255, round(depth))),
        "lat100": max(-9000, min(9000, round(latitude * 100))),
        "lon100": max(-18000, min(18000, round(longitude * 100))),
        "age": max(0, min(65535, (now_ms - when) // 60000)),
        "region": compact_region(str(props.get("place", "UNKN"))),
        "id": sum(event_id.encode("ascii", "ignore")) & 0xFF,
        "event_id": event_id,
        "mag": magnitude,
        "location": (latitude, longitude),
    }


def select_events(feed: dict[str, Any], location: tuple[float, float], now_ms: int) -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:
    events = []
    for feature in feed.get("features", []):
        try:
            event = normalize(feature, now_ms)
        except (KeyError, TypeError, ValueError, OverflowError):
            continue
        event["distance"] = distance_km(location, event["location"])
        events.append(event)
    local = sorted((e for e in events if e["distance"] <= LOCAL_RADIUS_KM), key=lambda e: (e["distance"], -e["mag"]))[:2]
    local_ids = {e["event_id"] for e in local}
    global_events = sorted((e for e in events if e["event_id"] not in local_ids), key=lambda e: (-e["mag"], e["age"]))[:2]
    return local, global_events


def build_record(sequence: int, local: list[dict[str, Any]], global_events: list[dict[str, Any]]) -> bytes:
    record = bytearray(64)
    record[:4] = b"EQ01"
    record[4], record[5], record[6], record[7] = sequence & 0xFF, len(local), len(global_events), 1
    for index, event in enumerate((local + global_events)[:4]):
        offset = 8 + index * 13
        record[offset], record[offset + 1] = event["mag10"], event["depth"]
        record[offset + 2:offset + 4] = int(event["lat100"]).to_bytes(2, "little", signed=True)
        record[offset + 4:offset + 6] = int(event["lon100"]).to_bytes(2, "little", signed=True)
        record[offset + 6:offset + 8] = int(event["age"]).to_bytes(2, "little")
        record[offset + 8:offset + 12] = event["region"]
        record[offset + 12] = event["id"]
    record[60:62] = (sum(record[:60]) & 0xFFFF).to_bytes(2, "little")
    record[62], record[63] = ord("E"), 10
    return bytes(record)


def current_location(store: Path, gazetteer: dict[str, tuple[float, float]]) -> tuple[str, tuple[float, float]]:
    try:
        _, zip_code = weather_gateway.parse_request((store / weather_gateway.REQUEST_NAME).read_bytes())
    except (FileNotFoundError, OSError, ValueError):
        zip_code = "90210"
    location = gazetteer.get(zip_code)
    if location is None:
        raise ValueError(f"unresolved ZCTA {zip_code}")
    return zip_code, location


def publish(config_path: Path, sequence: int = 1) -> bytes:
    config = configparser.ConfigParser()
    if not config.read(config_path, encoding="utf-8"):
        raise FileNotFoundError(config_path)
    section = config["weather"]
    store = Path(section["store_path"])
    gazetteer = weather_gateway.load_zcta(Path(section["gazetteer_path"]))
    zip_code, location = current_location(store, gazetteer)
    feed = weather_gateway.get_json(USGS_FEED, section["user_agent"].strip(), section.getfloat("timeout_seconds", 15.0))
    local, global_events = select_events(feed, location, int(time.time() * 1000))
    record = build_record(sequence, local, global_events)
    weather_gateway.atomic_write(store / RESOURCE_NAME, record)
    print(f"published EARTHQUAKE ZIP={zip_code} local={len(local)} global={len(global_events)} bytes=64 sha256={hashlib.sha256(record).hexdigest().upper()}", flush=True)
    return record


def run(config_path: Path, stop_event: Any | None = None) -> int:
    sequence = 1
    while stop_event is None or not stop_event.is_set():
        try:
            publish(config_path, sequence)
            sequence = (sequence + 1) & 0xFF
        except Exception as exc:
            print(f"earthquake acquisition failed: {exc}; prior EARTHQUAKE preserved", flush=True)
        if stop_event is None:
            return 0
        stop_event.wait(300.0)
    return 0


if __name__ == "__main__":
    raise SystemExit(run(Path("weather.ini")))
