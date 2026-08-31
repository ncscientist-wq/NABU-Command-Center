"""Bounded ISS position publisher for the NABU Command Center Gateway."""

from __future__ import annotations

import json
import os
import struct
import time
import urllib.request
from pathlib import Path
from typing import Callable, Mapping, Any


API_URL = "https://api.wheretheiss.at/v1/satellites/25544"
RESOURCE_NAME = "ncc_satellite.dat"
RECORD_SIZE = 64
DEFAULT_REFRESH_SECONDS = 30.0


def fetch_satellite(timeout: float = 15.0) -> Mapping[str, Any]:
    request = urllib.request.Request(
        API_URL,
        headers={"User-Agent": "NABU-Command-Center/1.0"},
    )
    with urllib.request.urlopen(request, timeout=timeout) as response:
        payload = json.load(response)
    if not isinstance(payload, dict):
        raise ValueError("ISS provider response is not an object")
    return payload


def _number(sample: Mapping[str, Any], name: str) -> float:
    value = sample.get(name)
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise ValueError(f"invalid {name}")
    return float(value)


def build_record(sample: Mapping[str, Any], sequence: int) -> bytes:
    satellite_id = int(_number(sample, "id"))
    if satellite_id != 25544:
        raise ValueError("provider response is not ISS (NORAD 25544)")

    latitude = _number(sample, "latitude")
    longitude = _number(sample, "longitude")
    altitude = _number(sample, "altitude")
    velocity = _number(sample, "velocity")
    footprint = _number(sample, "footprint")
    timestamp = int(_number(sample, "timestamp"))
    units = str(sample.get("units", "")).lower()
    if units not in ("kilometers", "kilometres"):
        raise ValueError("unexpected ISS provider units")
    if not -90.0 <= latitude <= 90.0:
        raise ValueError("latitude out of range")
    if not -180.0 <= longitude <= 180.0:
        raise ValueError("longitude out of range")
    if not 0.0 <= altitude <= 65535.0:
        raise ValueError("altitude out of range")
    if not 0.0 <= velocity / 10.0 <= 65535.0:
        raise ValueError("velocity out of range")
    if not 0.0 <= footprint <= 65535.0:
        raise ValueError("footprint out of range")
    if not 0 < timestamp <= 0xFFFFFFFF:
        raise ValueError("timestamp out of range")

    visibility = str(sample.get("visibility", "")).lower()
    visibility_code = {"daylight": 1, "eclipsed": 2}.get(visibility, 0)

    record = bytearray(RECORD_SIZE)
    record[0:4] = b"SA01"
    record[4] = sequence & 0xFF
    record[5] = 1
    struct.pack_into("<h", record, 6, round(latitude * 100.0))
    struct.pack_into("<h", record, 8, round(longitude * 100.0))
    struct.pack_into("<H", record, 10, round(altitude))
    struct.pack_into("<H", record, 12, round(velocity / 10.0))
    struct.pack_into("<I", record, 14, timestamp)
    record[18:23] = b"ISS  "
    record[23:28] = b"25544"
    record[28] = visibility_code
    struct.pack_into("<H", record, 29, round(footprint))
    struct.pack_into("<H", record, 60, sum(record[:60]) & 0xFFFF)
    record[62] = ord("E")
    record[63] = 0x0A
    return bytes(record)


def _atomic_write(destination: Path, payload: bytes) -> None:
    temporary = destination.with_suffix(destination.suffix + ".tmp")
    with temporary.open("wb") as stream:
        stream.write(payload)
        stream.flush()
        os.fsync(stream.fileno())
    os.replace(temporary, destination)


def publish_once(
    store_path: str | os.PathLike[str],
    sequence: int,
    fetcher: Callable[[], Mapping[str, Any]] = fetch_satellite,
) -> bytes:
    record = build_record(fetcher(), sequence)
    destination = Path(store_path) / RESOURCE_NAME
    destination.parent.mkdir(parents=True, exist_ok=True)
    _atomic_write(destination, record)
    return record


def run(store_path: str | os.PathLike[str], stop_event) -> None:
    refresh_seconds = float(
        os.environ.get("NCC_SATELLITE_REFRESH_SECONDS", DEFAULT_REFRESH_SECONDS)
    )
    sequence = 0
    while not stop_event.is_set():
        try:
            record = publish_once(store_path, sequence)
            latitude = struct.unpack_from("<h", record, 6)[0] / 100.0
            longitude = struct.unpack_from("<h", record, 8)[0] / 100.0
            print(
                "published SATELLITE/ISS "
                f"lat={latitude:.2f} lon={longitude:.2f} bytes={len(record)}"
            )
            sequence = (sequence + 1) & 0xFF
        except Exception as exc:
            # Keep the last valid Store resource untouched.
            print(f"SATELLITE/ISS update failed; preserving last valid: {exc}")
        stop_event.wait(refresh_seconds)


if __name__ == "__main__":
    import threading

    run(os.environ.get("NCC_STORE_PATH", r"D:\NABU Internet Adapter\Store"), threading.Event())
