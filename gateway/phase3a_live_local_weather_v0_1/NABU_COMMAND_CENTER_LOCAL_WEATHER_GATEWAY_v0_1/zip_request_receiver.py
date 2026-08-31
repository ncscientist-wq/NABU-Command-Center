#!/usr/bin/env python3
"""Bounded Phase 3A-02A ZIP request receiver; no weather work."""

from __future__ import annotations

import argparse
import re
import time
from pathlib import Path

RESOURCE_NAME = "ncc_zip.req"
REQUEST_SIZE = 17
REQUEST_RE = re.compile(rb"ZIP\|([0-9]{6})\|([0-9]{5})\n\Z")


def parse_request(content: bytes) -> tuple[str, str]:
    if len(content) != REQUEST_SIZE:
        raise ValueError(f"request size {len(content)} != {REQUEST_SIZE}")
    match = REQUEST_RE.fullmatch(content)
    if match is None:
        raise ValueError("request format invalid")
    return match.group(1).decode("ascii"), match.group(2).decode("ascii")


def read_complete(path: Path) -> tuple[str, str, bytes]:
    content = path.read_bytes()
    sequence, zip_code = parse_request(content)
    return sequence, zip_code, content


def run(store_path: Path, once: bool, poll_seconds: float) -> int:
    request_path = store_path / RESOURCE_NAME
    previous: bytes | None = None
    print(f"ZIP request resource = {request_path}", flush=True)
    while True:
        try:
            sequence, zip_code, content = read_complete(request_path)
        except (FileNotFoundError, OSError, ValueError):
            time.sleep(poll_seconds)
            continue
        if content != previous:
            print(f"received ZIP request = {zip_code}", flush=True)
            print(f"request sequence/token = {sequence}", flush=True)
            previous = content
            if once:
                return 0
        time.sleep(poll_seconds)


def main() -> int:
    parser = argparse.ArgumentParser(description="NABU Command Center Phase 3A-02A ZIP proof receiver")
    parser.add_argument("--store-path", type=Path, required=True)
    parser.add_argument("--once", action="store_true")
    parser.add_argument("--poll-seconds", type=float, default=0.10)
    args = parser.parse_args()
    if args.poll_seconds <= 0:
        parser.error("--poll-seconds must be positive")
    return run(args.store_path, args.once, args.poll_seconds)


if __name__ == "__main__":
    raise SystemExit(main())
