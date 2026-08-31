#!/usr/bin/env python3
"""Validated NIST UTC publisher with bounded monotonic holdover maintenance."""

from __future__ import annotations

import argparse
import datetime as dt
import json
import logging
from pathlib import Path
import socket
import struct
import time

from gateway import atomic_write, canonical_record

BUILD_ID = "NCC-GW-260810-P3A-01A"
NIST_HOST = "time.nist.gov"
NTP_PORT = 123
NTP_EPOCH_DELTA = 2_208_988_800
RESOURCE_NAME = "ncc_time.dat"
STATE_NAME = "ncc_time_state.json"
SOCKET_TIMEOUT = 5.0


class NtpError(ValueError):
    pass


def ntp_timestamp(now_ns: int | None = None) -> bytes:
    value = time.time_ns() if now_ns is None else now_ns
    seconds, nanoseconds = divmod(value, 1_000_000_000)
    seconds += NTP_EPOCH_DELTA
    fraction = (nanoseconds << 32) // 1_000_000_000
    return struct.pack("!II", seconds & 0xFFFFFFFF, fraction & 0xFFFFFFFF)


def make_request(origin: bytes | None = None) -> tuple[bytes, bytes]:
    token = ntp_timestamp() if origin is None else origin
    if len(token) != 8 or token == b"\0" * 8:
        raise ValueError("request timestamp must be a nonzero 8-byte NTP timestamp")
    packet = bytearray(48)
    packet[0] = (4 << 3) | 3  # LI=0, VN=4, client mode=3
    packet[40:48] = token
    return bytes(packet), token


def validate_response(packet: bytes, request_timestamp: bytes) -> dict[str, object]:
    if len(packet) < 48:
        raise NtpError("short NTP response")
    li = packet[0] >> 6
    version = (packet[0] >> 3) & 7
    mode = packet[0] & 7
    stratum = packet[1]
    if mode != 4:
        raise NtpError("response mode is not Server")
    if version not in (3, 4):
        raise NtpError("unsupported NTP version")
    if li == 3:
        raise NtpError("server reports unsynchronized leap state")
    if stratum == 0:
        code = packet[12:16].decode("ascii", "replace")
        raise NtpError(f"stratum-0 Kiss-o'-Death rejected: {code}")
    if not 1 <= stratum <= 15:
        raise NtpError("invalid NTP stratum")
    if packet[24:32] != request_timestamp:
        raise NtpError("originate timestamp does not match request")
    transmit = packet[40:48]
    if transmit == b"\0" * 8:
        raise NtpError("zero server transmit timestamp")
    seconds, fraction = struct.unpack("!II", transmit)
    if seconds <= NTP_EPOCH_DELTA:
        raise NtpError("server transmit timestamp predates Unix epoch")
    unix_seconds = seconds - NTP_EPOCH_DELTA
    utc = dt.datetime.fromtimestamp(unix_seconds, tz=dt.timezone.utc)
    return {
        "li": li,
        "version": version,
        "mode": mode,
        "stratum": stratum,
        "transmit_seconds": seconds,
        "transmit_fraction": fraction,
        "unix_seconds": unix_seconds,
        "utc": utc,
    }


def acquire_nist(hostname: str = NIST_HOST, timeout: float = SOCKET_TIMEOUT,
                 resolved_ip: str | None = None) -> dict[str, object]:
    errors: list[str] = []
    lookup_name = hostname if resolved_ip is None else resolved_ip
    addresses = socket.getaddrinfo(lookup_name, NTP_PORT, socket.AF_INET, socket.SOCK_DGRAM)
    for family, socktype, proto, _, target in addresses:
        request, token = make_request()
        try:
            with socket.socket(family, socktype, proto) as client:
                client.settimeout(timeout)
                client.sendto(request, target)
                packet, source = client.recvfrom(512)
            if source[0] != target[0] or source[1] != NTP_PORT:
                raise NtpError("response source does not match queried NIST endpoint")
            result = validate_response(packet, token)
            result.update({
                "hostname": hostname,
                "resolved_ip": target[0],
                "response_source": f"{source[0]}:{source[1]}",
                "response_length": len(packet),
            })
            return result
        except (OSError, NtpError) as exc:
            errors.append(f"{target[0]}: {exc}")
    raise NtpError("all NIST endpoints failed: " + "; ".join(errors))


def next_sequence(state_path: Path) -> int:
    try:
        value = int(json.loads(state_path.read_text(encoding="utf-8"))["sequence"])
    except FileNotFoundError:
        value = 0
    if not 0 <= value < 999999:
        raise ValueError("stored sequence is outside supported range")
    return value + 1


def build_time_record(sequence: int, result: dict[str, object]) -> bytes:
    utc = result["utc"]
    assert isinstance(utc, dt.datetime)
    payload = utc.strftime("%Y-%m-%dT%H:%MZ")
    record = canonical_record(
        sequence, int(result["unix_seconds"]), int(result["stratum"]), payload,
        record_type="TIME",
    )
    if len(payload) != 17 or len(record) != 64:
        raise ValueError("TIME payload/envelope is not exactly 17/64 bytes")
    return record


def publish_once(store_path: Path, hostname: str, timeout: float, logger: logging.Logger,
                 resolved_ip: str | None = None) -> dict[str, object]:
    state_path = store_path / STATE_NAME
    sequence = next_sequence(state_path)
    result = acquire_nist(hostname, timeout, resolved_ip)
    record = build_time_record(sequence, result)
    final_path = store_path / RESOURCE_NAME
    atomic_write(final_path, record)
    atomic_write(state_path, (json.dumps({"sequence": sequence}) + "\n").encode("ascii"))
    result.update({"sequence": sequence, "record": record, "path": str(final_path)})
    logger.info(
        "nist_publish sequence=%d utc=%s hostname=%s resolved=%s source=%s stratum=%d bytes=%d",
        sequence, result["utc"].isoformat(), hostname, result["resolved_ip"],
        result["response_source"], result["stratum"], len(record),
    )
    return result


def publish_holdover(store_path: Path, synchronized: dict[str, object],
                     elapsed_seconds: int, logger: logging.Logger) -> dict[str, object]:
    """Publish disciplined elapsed time; the host wall clock is not an authority."""
    if elapsed_seconds < 0:
        raise ValueError("negative monotonic holdover")
    state_path = store_path / STATE_NAME
    sequence = next_sequence(state_path)
    unix_seconds = int(synchronized["unix_seconds"]) + elapsed_seconds
    result = dict(synchronized)
    result["unix_seconds"] = unix_seconds
    result["utc"] = dt.datetime.fromtimestamp(unix_seconds, tz=dt.timezone.utc)
    record = build_time_record(sequence, result)
    final_path = store_path / RESOURCE_NAME
    atomic_write(final_path, record)
    atomic_write(state_path, (json.dumps({"sequence": sequence}) + "\n").encode("ascii"))
    result.update({"sequence": sequence, "record": record, "path": str(final_path),
                   "authority": "NIST disciplined monotonic holdover"})
    logger.info("nist_holdover_publish sequence=%d utc=%s age=%d bytes=%d",
                sequence, result["utc"].isoformat(), elapsed_seconds, len(record))
    return result


def maintain_time(store_path: Path, hostname: str, timeout: float,
                  resync_interval: float, publish_interval: float,
                  max_holdover: float, logger: logging.Logger,
                  stop_event: object | None = None) -> int:
    if resync_interval < 60 or publish_interval < 1 or max_holdover < resync_interval:
        raise ValueError("invalid NIST maintenance intervals")
    synchronized: dict[str, object] | None = None
    sync_monotonic = 0.0
    next_sync = 0.0
    next_publish = float("inf")
    while stop_event is None or not stop_event.is_set():
        now = time.monotonic()
        if now >= next_sync:
            try:
                synchronized = publish_once(store_path, hostname, timeout, logger)
                sync_monotonic = now
                next_sync = now + resync_interval
                next_publish = now + publish_interval
            except Exception as exc:
                logger.error("NIST resynchronization failed: %s", exc)
                next_sync = now + min(60.0, resync_interval)
        elif synchronized is not None and now >= next_publish:
            age = now - sync_monotonic
            if age <= max_holdover:
                publish_holdover(store_path, synchronized, int(age), logger)
            else:
                logger.error("NIST holdover expired at %.0f seconds; publication stopped", age)
            next_publish = now + publish_interval
        time.sleep(0.10)
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--store-path", type=Path, default=Path(r"D:\NABU Internet Adapter\Store"))
    parser.add_argument("--hostname", default=NIST_HOST)
    parser.add_argument("--server-ip", help="IP resolved from the configured hostname for this transaction")
    parser.add_argument("--timeout", type=float, default=SOCKET_TIMEOUT)
    parser.add_argument("--maintain", action="store_true")
    parser.add_argument("--resync-interval", type=float, default=3600.0)
    parser.add_argument("--publish-interval", type=float, default=10.0)
    parser.add_argument("--max-holdover", type=float, default=7200.0)
    parser.add_argument("--log", type=Path, default=Path("logs/nist_time_gateway.log"))
    args = parser.parse_args()
    args.log.parent.mkdir(parents=True, exist_ok=True)
    logging.basicConfig(level=logging.INFO, format="%(asctime)sZ %(levelname)s %(message)s",
                        handlers=[logging.FileHandler(args.log, encoding="utf-8"), logging.StreamHandler()])
    logger = logging.getLogger("ncc_nist")
    if args.maintain:
        try:
            return maintain_time(args.store_path, args.hostname, args.timeout,
                                 args.resync_interval, args.publish_interval,
                                 args.max_holdover, logger)
        except Exception as exc:
            logger.error("NIST maintenance failed: %s", exc)
            return 1
    try:
        result = publish_once(args.store_path, args.hostname, args.timeout, logger, args.server_ip)
    except Exception as exc:
        logger.error("NIST acquisition/publication failed: %s", exc)
        return 1
    print(f"BUILD {BUILD_ID}")
    print(f"UTC {result['utc'].isoformat()}")
    print(f"HOST {result['hostname']} RESOLVED {result['resolved_ip']} SOURCE {result['response_source']}")
    print(f"STRATUM {result['stratum']} SEQUENCE {result['sequence']} BYTES {len(result['record'])}")
    print(f"RECORD {result['record'].decode('ascii').rstrip()}")
    print(f"PATH {result['path']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
