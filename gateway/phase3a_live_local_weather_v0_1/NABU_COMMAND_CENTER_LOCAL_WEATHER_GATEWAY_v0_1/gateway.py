#!/usr/bin/env python3
"""
NABU COMMAND CENTER GATEWAY SIMULATOR v0.1

Standard-library-only deterministic gateway simulator.

Publisher modes:
- store: atomically writes ncc_test.dat into a configured directory.
- http: serves only /ncc_test.dat on loopback by default.
- both: enables both publishers.

This simulator does not expose itself publicly by default.
"""

from __future__ import annotations

import argparse
import configparser
import dataclasses
import datetime as dt
import http.server
import json
import logging
import os
from pathlib import Path
import queue
import socketserver
import sys
import tempfile
import threading
import time
from typing import Optional

PROGRAM_NAME = "NABU COMMAND CENTER GATEWAY SIM"
PROGRAM_VERSION = "0.1"
BUILD_ID = "NCC-GW-260806-1043-P01"
RESOURCE_NAME = "ncc_test.dat"
RESOURCE_PATH = "/" + RESOURCE_NAME
DEFAULT_TEXT = "GATEWAY_RECORD_01"
DEFAULT_VALUE = 42
DEFAULT_SEQUENCE = 1
DEFAULT_HOST = "127.0.0.1"
DEFAULT_PORT = 8765
DEFAULT_INTERVAL = 10.0

FAILURE_MODES = (
    "valid",
    "missing",
    "empty",
    "truncated",
    "bad_magic",
    "unsupported_version",
    "bad_length",
    "bad_integrity",
    "oversized",
    "stale_timestamp",
    "sequence_rollback",
    "offline",
)


@dataclasses.dataclass
class Settings:
    mode: str
    host: str
    port: int
    store_path: Path
    interval: float
    auto_refresh: bool
    sequence: int
    value: int
    text: str
    failure_mode: str
    fixed_utc: Optional[int]
    log_path: Path


@dataclasses.dataclass
class PublishedRecord:
    content: Optional[bytes]
    http_status: int
    sequence: int
    generated_utc: int
    failure_mode: str
    note: str


class State:
    def __init__(self, settings: Settings) -> None:
        self.lock = threading.Lock()
        self.settings = settings
        self.current = PublishedRecord(
            content=None,
            http_status=503,
            sequence=settings.sequence,
            generated_utc=0,
            failure_mode="offline",
            note="not generated",
        )
        self.stop_event = threading.Event()

    def snapshot(self) -> PublishedRecord:
        with self.lock:
            return dataclasses.replace(self.current)

    def replace(self, record: PublishedRecord) -> None:
        with self.lock:
            self.current = record


def utc_now() -> int:
    return int(time.time())


def checksum16(body: bytes) -> int:
    """Unsigned additive checksum, low 16 bits."""
    return sum(body) & 0xFFFF


def sanitize_text(text: str, required_length: Optional[int] = None) -> str:
    sanitized = "".join(ch if 0x20 <= ord(ch) <= 0x7E and ch != "|" else "?"
                        for ch in text)
    if required_length is not None:
        if len(sanitized) < required_length:
            sanitized = sanitized + ("_" * (required_length - len(sanitized)))
        sanitized = sanitized[:required_length]
    return sanitized


def canonical_record(
    sequence: int,
    generated_utc: int,
    value: int,
    text: str,
    *,
    magic: str = "NCC",
    version: str = "1",
    record_type: str = "TEST",
    declared_length: Optional[int] = None,
    checksum_override: Optional[int] = None,
    end_marker: str = "END",
) -> bytes:
    if not 0 <= sequence <= 999999:
        raise ValueError("sequence must be 0..999999")
    if not 0 <= generated_utc <= 4294967295:
        raise ValueError("UTC must fit unsigned 32-bit")
    if not 0 <= value <= 9999:
        raise ValueError("value must be 0..9999")

    clean_text = sanitize_text(text)
    text_length = len(clean_text) if declared_length is None else declared_length
    if not 0 <= text_length <= 99:
        raise ValueError("declared text length must be 0..99")

    body_text = (
        f"{magic}|{version}|{record_type}|{sequence:06d}|"
        f"{generated_utc:010d}|{text_length:02d}|{value:04d}|"
        f"{clean_text}|"
    )
    body = body_text.encode("ascii")
    integrity = checksum16(body) if checksum_override is None else checksum_override & 0xFFFF
    return body + f"{integrity:04X}|{end_marker}\n".encode("ascii")


def build_failure_record(settings: Settings, sequence: int, now: int) -> PublishedRecord:
    mode = settings.failure_mode
    text = sanitize_text(settings.text, 17)

    if mode == "missing":
        return PublishedRecord(None, 404, sequence, now, mode, "resource intentionally missing")
    if mode == "empty":
        return PublishedRecord(b"", 200, sequence, now, mode, "empty resource")
    if mode == "offline":
        return PublishedRecord(None, 503, sequence, now, mode, "publisher offline")

    generated_utc = now
    effective_sequence = sequence
    magic = "NCC"
    version = "1"
    declared_length = None
    checksum_override = None

    if mode == "bad_magic":
        magic = "BAD"
    elif mode == "unsupported_version":
        version = "9"
    elif mode == "bad_length":
        declared_length = (len(text) + 1) % 100
    elif mode == "bad_integrity":
        checksum_override = 0
    elif mode == "stale_timestamp":
        generated_utc = max(0, now - 86400)
    elif mode == "sequence_rollback":
        effective_sequence = max(0, sequence - 2)
    elif mode == "oversized":
        oversized_text = sanitize_text("X" * 300)
        body = (
            f"NCC|1|TEST|{sequence:06d}|{generated_utc:010d}|99|"
            f"{settings.value:04d}|{oversized_text}|"
        ).encode("ascii")
        integrity = checksum16(body)
        content = body + f"{integrity:04X}|END\n".encode("ascii")
        return PublishedRecord(content, 200, effective_sequence, generated_utc,
                               mode, "oversized resource")
    elif mode not in FAILURE_MODES:
        raise ValueError(f"unsupported failure mode: {mode}")

    content = canonical_record(
        effective_sequence,
        generated_utc,
        settings.value,
        text,
        magic=magic,
        version=version,
        declared_length=declared_length,
        checksum_override=checksum_override,
    )

    if mode == "truncated":
        content = content[: max(1, len(content) // 2)]

    return PublishedRecord(content, 200, effective_sequence, generated_utc,
                           mode, "generated")


def atomic_write(final_path: Path, content: bytes) -> None:
    final_path.parent.mkdir(parents=True, exist_ok=True)
    fd, temp_name = tempfile.mkstemp(
        prefix=final_path.name + ".",
        suffix=".tmp",
        dir=str(final_path.parent),
    )
    temp_path = Path(temp_name)
    try:
        with os.fdopen(fd, "wb") as handle:
            handle.write(content)
            handle.flush()
            os.fsync(handle.fileno())
        os.replace(temp_path, final_path)
    except Exception:
        try:
            temp_path.unlink(missing_ok=True)
        finally:
            raise


def publish_store(state: State, record: PublishedRecord, logger: logging.Logger) -> None:
    final_path = state.settings.store_path / RESOURCE_NAME

    if record.failure_mode == "missing":
        final_path.unlink(missing_ok=True)
        logger.warning("store_missing path=%s", final_path)
        return

    if record.failure_mode == "offline":
        logger.warning("store_offline path=%s existing_file_preserved=true", final_path)
        return

    assert record.content is not None
    atomic_write(final_path, record.content)
    logger.info(
        "store_publish path=%s bytes=%d sequence=%d mode=%s",
        final_path,
        len(record.content),
        record.sequence,
        record.failure_mode,
    )


def generate_and_publish(state: State, logger: logging.Logger,
                         increment: bool = True) -> PublishedRecord:
    settings = state.settings
    if increment:
        settings.sequence = (settings.sequence + 1) % 1000000

    now = settings.fixed_utc if settings.fixed_utc is not None else utc_now()
    record = build_failure_record(settings, settings.sequence, now)
    state.replace(record)

    logger.info(
        "record_generated sequence=%d utc=%d mode=%s bytes=%s value=%d text=%r",
        record.sequence,
        record.generated_utc,
        record.failure_mode,
        "missing" if record.content is None else len(record.content),
        settings.value,
        sanitize_text(settings.text, 17),
    )

    if settings.mode in ("store", "both"):
        try:
            publish_store(state, record, logger)
        except Exception:
            logger.exception("store_publish_failed")
            raise

    return record


class ReusableThreadingHTTPServer(socketserver.ThreadingMixIn, http.server.HTTPServer):
    daemon_threads = True
    allow_reuse_address = True


def make_handler(state: State, logger: logging.Logger):
    class Handler(http.server.BaseHTTPRequestHandler):
        server_version = "NCCGatewaySim/0.1"

        def log_message(self, fmt: str, *args) -> None:
            logger.info("http_client=%s message=%s", self.client_address[0], fmt % args)

        def do_GET(self) -> None:
            logger.info("http_request method=GET path=%s client=%s",
                        self.path, self.client_address[0])

            if self.path != RESOURCE_PATH:
                self.send_error(404, "Not Found")
                return

            record = state.snapshot()
            if record.http_status != 200:
                self.send_error(record.http_status)
                return

            content = record.content if record.content is not None else b""
            self.send_response(200)
            self.send_header("Content-Type", "text/plain; charset=us-ascii")
            self.send_header("Content-Length", str(len(content)))
            self.send_header("Cache-Control", "no-store, max-age=0")
            self.send_header("X-NCC-Sequence", str(record.sequence))
            self.send_header("X-NCC-Failure-Mode", record.failure_mode)
            self.end_headers()
            self.wfile.write(content)

        def do_HEAD(self) -> None:
            if self.path != RESOURCE_PATH:
                self.send_error(404, "Not Found")
                return
            record = state.snapshot()
            if record.http_status != 200:
                self.send_error(record.http_status)
                return
            content = record.content if record.content is not None else b""
            self.send_response(200)
            self.send_header("Content-Type", "text/plain; charset=us-ascii")
            self.send_header("Content-Length", str(len(content)))
            self.send_header("Cache-Control", "no-store, max-age=0")
            self.end_headers()

    return Handler


def configure_logging(log_path: Path) -> logging.Logger:
    log_path.parent.mkdir(parents=True, exist_ok=True)
    logger = logging.getLogger("ncc_gateway")
    logger.setLevel(logging.INFO)

    for existing_handler in list(logger.handlers):
        logger.removeHandler(existing_handler)
        existing_handler.close()

    formatter = logging.Formatter(
        "%(asctime)sZ %(levelname)s %(message)s",
        datefmt="%Y-%m-%dT%H:%M:%S",
    )
    formatter.converter = time.gmtime

    file_handler = logging.FileHandler(log_path, encoding="utf-8")
    file_handler.setFormatter(formatter)
    logger.addHandler(file_handler)

    console_handler = logging.StreamHandler()
    console_handler.setFormatter(formatter)
    logger.addHandler(console_handler)
    return logger


def load_config(path: Optional[Path], args: argparse.Namespace) -> Settings:
    parser = configparser.ConfigParser()
    if path:
        if not path.exists():
            raise FileNotFoundError(f"configuration not found: {path}")
        parser.read(path, encoding="utf-8")

    section = parser["gateway"] if parser.has_section("gateway") else {}

    mode = args.mode or section.get("mode", "store")
    host = args.host or section.get("host", DEFAULT_HOST)
    port = args.port if args.port is not None else int(section.get("port", DEFAULT_PORT))
    store_path = Path(args.store_path or section.get("store_path", "./store"))
    interval = args.interval if args.interval is not None else float(
        section.get("interval_seconds", DEFAULT_INTERVAL)
    )
    auto_refresh = args.auto or section.get("auto_refresh", "false").lower() in (
        "1", "true", "yes", "on"
    )
    sequence = args.sequence if args.sequence is not None else int(
        section.get("initial_sequence", DEFAULT_SEQUENCE)
    )
    value = args.value if args.value is not None else int(
        section.get("fixed_value", DEFAULT_VALUE)
    )
    text = args.text or section.get("fixed_text", DEFAULT_TEXT)
    failure_mode = args.failure or section.get("failure_mode", "valid")
    fixed_utc = args.fixed_utc
    if fixed_utc is None and section.get("fixed_utc", "").strip():
        fixed_utc = int(section["fixed_utc"])
    log_path = Path(args.log_path or section.get("log_path", "./logs/gateway.log"))

    if mode not in ("store", "http", "both"):
        raise ValueError("mode must be store, http, or both")
    if host != "127.0.0.1" and not args.allow_non_loopback:
        raise ValueError(
            "non-loopback bind refused; add --allow-non-loopback deliberately"
        )
    if not 1 <= port <= 65535:
        raise ValueError("port must be 1..65535")
    if interval <= 0:
        raise ValueError("interval must be positive")
    if failure_mode not in FAILURE_MODES:
        raise ValueError("unsupported failure mode")
    if not 0 <= sequence <= 999999:
        raise ValueError("sequence must be 0..999999")
    if not 0 <= value <= 9999:
        raise ValueError("value must be 0..9999")

    return Settings(
        mode=mode,
        host=host,
        port=port,
        store_path=store_path,
        interval=interval,
        auto_refresh=auto_refresh,
        sequence=sequence,
        value=value,
        text=text,
        failure_mode=failure_mode,
        fixed_utc=fixed_utc,
        log_path=log_path,
    )


def auto_worker(state: State, logger: logging.Logger) -> None:
    while not state.stop_event.wait(state.settings.interval):
        try:
            generate_and_publish(state, logger, increment=True)
        except Exception:
            logger.exception("automatic_publish_failed")


def print_help() -> None:
    print(
        "\nCommands:\n"
        "  r                 generate/publish and increment sequence\n"
        "  s                 show current state\n"
        "  f MODE            set failure mode\n"
        "  v INTEGER         set known integer 0..9999\n"
        "  t TEXT            set text (sanitized and fixed to 17 chars)\n"
        "  a on|off          enable/disable automatic refresh\n"
        "  h                 show commands\n"
        "  q                 shut down\n"
    )


def show_state(state: State) -> None:
    record = state.snapshot()
    content_preview = (
        "<missing>" if record.content is None
        else repr(record.content[:120])
    )
    print(
        f"mode={state.settings.mode} failure={state.settings.failure_mode} "
        f"sequence={record.sequence} utc={record.generated_utc} "
        f"http_status={record.http_status} content={content_preview}"
    )


def parse_args(argv: Optional[list[str]] = None) -> argparse.Namespace:
    p = argparse.ArgumentParser(description=PROGRAM_NAME)
    p.add_argument("--config", type=Path)
    p.add_argument("--mode", choices=("store", "http", "both"))
    p.add_argument("--host")
    p.add_argument("--port", type=int)
    p.add_argument("--store-path")
    p.add_argument("--interval", type=float)
    p.add_argument("--auto", action="store_true")
    p.add_argument("--once", action="store_true")
    p.add_argument("--sequence", type=int)
    p.add_argument("--value", type=int)
    p.add_argument("--text")
    p.add_argument("--failure", choices=FAILURE_MODES)
    p.add_argument("--fixed-utc", type=int)
    p.add_argument("--log-path")
    p.add_argument("--allow-non-loopback", action="store_true")
    return p.parse_args(argv)


def main(argv: Optional[list[str]] = None) -> int:
    args = parse_args(argv)
    try:
        settings = load_config(args.config, args)
    except Exception as exc:
        print(f"CONFIGURATION ERROR: {exc}", file=sys.stderr)
        return 2

    logger = configure_logging(settings.log_path)
    state = State(settings)

    print(f"{PROGRAM_NAME} {PROGRAM_VERSION}")
    print(f"BUILD {BUILD_ID}")
    print(f"Publisher mode: {settings.mode}")
    if settings.mode in ("http", "both"):
        print(f"HTTP resource: http://{settings.host}:{settings.port}{RESOURCE_PATH}")
    if settings.mode in ("store", "both"):
        print(f"Store resource: {settings.store_path / RESOURCE_NAME}")
    print("Public exposure: DISABLED BY DEFAULT")
    logger.info(
        "gateway_start build=%s mode=%s host=%s port=%d store=%s",
        BUILD_ID, settings.mode, settings.host, settings.port, settings.store_path
    )

    http_server = None
    http_thread = None
    auto_thread = None

    try:
        generate_and_publish(state, logger, increment=False)

        if settings.mode in ("http", "both"):
            handler = make_handler(state, logger)
            http_server = ReusableThreadingHTTPServer(
                (settings.host, settings.port), handler
            )
            http_thread = threading.Thread(
                target=http_server.serve_forever,
                name="ncc-http",
                daemon=True,
            )
            http_thread.start()
            logger.info("http_started host=%s port=%d", settings.host, settings.port)

        if args.once:
            show_state(state)
            return 0

        if settings.auto_refresh:
            auto_thread = threading.Thread(
                target=auto_worker,
                args=(state, logger),
                name="ncc-auto",
                daemon=True,
            )
            auto_thread.start()
            logger.info("auto_refresh_started interval=%s", settings.interval)

        print_help()
        while True:
            try:
                command = input("ncc-gateway> ").strip()
            except EOFError:
                command = "q"

            if not command:
                continue

            name, _, rest = command.partition(" ")
            name = name.lower()
            rest = rest.strip()

            if name == "q":
                break
            if name == "h":
                print_help()
            elif name == "r":
                generate_and_publish(state, logger, increment=True)
                show_state(state)
            elif name == "s":
                show_state(state)
            elif name == "f":
                if rest not in FAILURE_MODES:
                    print("Invalid mode. Valid modes:", ", ".join(FAILURE_MODES))
                else:
                    settings.failure_mode = rest
                    print(f"Failure mode set to {rest}; press r to publish.")
            elif name == "v":
                try:
                    value = int(rest)
                    if not 0 <= value <= 9999:
                        raise ValueError
                    settings.value = value
                    print(f"Known integer set to {value}; press r to publish.")
                except ValueError:
                    print("Value must be 0..9999.")
            elif name == "t":
                settings.text = sanitize_text(rest, 17)
                print(f"Text set to {settings.text!r}; press r to publish.")
            elif name == "a":
                if rest.lower() == "on":
                    if auto_thread is None or not auto_thread.is_alive():
                        settings.auto_refresh = True
                        auto_thread = threading.Thread(
                            target=auto_worker,
                            args=(state, logger),
                            name="ncc-auto",
                            daemon=True,
                        )
                        auto_thread.start()
                    print("Automatic refresh enabled.")
                elif rest.lower() == "off":
                    settings.auto_refresh = False
                    print(
                        "Automatic refresh disable requested. "
                        "Restart gateway to stop an already running auto worker."
                    )
                else:
                    print("Use: a on  or  a off")
            else:
                print("Unknown command. Press h for help.")

        return 0
    except KeyboardInterrupt:
        print()
        return 0
    except Exception:
        logger.exception("gateway_fatal")
        return 1
    finally:
        state.stop_event.set()
        if http_server is not None:
            http_server.shutdown()
            http_server.server_close()
        logger.info("gateway_shutdown")
        print("NABU COMMAND CENTER GATEWAY SIMULATOR SHUTDOWN")


if __name__ == "__main__":
    raise SystemExit(main())
