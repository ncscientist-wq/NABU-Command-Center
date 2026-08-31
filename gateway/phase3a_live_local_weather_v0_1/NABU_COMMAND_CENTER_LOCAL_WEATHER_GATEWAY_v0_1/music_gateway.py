#!/usr/bin/env python3
"""Mutopia-only MIDI acquisition and bounded NabuTracker-style music publication."""

from __future__ import annotations

import hashlib
import io
import json
import math
import os
from pathlib import Path
import subprocess
import tempfile
import time

import mido

MUSIC_NAME = "ncc_music.dat"
RECORD_SIZE = 512
MAX_EVENTS = 82
EVENT_OFFSET = 12
MUTOPIA_PAGE = "https://www.mutopiaproject.org/cgibin/piece-info.cgi?id=257"
MUTOPIA_MIDI = "https://www.mutopiaproject.org/ftp/BachJS/BWV508/BistDuBeiMir/BistDuBeiMir.mid"
WORK_ID = "Mutopia-2007/07/08-257"
TITLE = "Aria Bist Du Bei Mir"
COMPOSER = "J. S. Bach"
STYLE = "Baroque"
LICENSE = "Public Domain"
CONVERTER_VERSION = "MU01-LV2-60HZ"
CACHE_STEM = "mutopia-257"


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


def acquire_midi(url: str, timeout: float = 20.0) -> bytes:
    curl = Path(r"C:\Windows\System32\curl.exe")
    if not curl.is_file():
        raise FileNotFoundError(curl)
    result = subprocess.run(
        [str(curl), "--fail", "--location", "--silent", "--show-error",
         "--max-time", str(max(1, int(timeout))),
         "-H", "User-Agent: NABU-Command-Center/1.0", url],
        check=True, capture_output=True,
    )
    body = result.stdout
    if len(body) < 14 or len(body) > 1_000_000 or body[:4] != b"MThd":
        raise ValueError("Mutopia response is not a bounded complete MIDI file")
    return body


def validate_midi(body: bytes) -> mido.MidiFile:
    if len(body) < 14 or body[:4] != b"MThd":
        raise ValueError("invalid MIDI header")
    midi = mido.MidiFile(file=io.BytesIO(body), clip=False)
    if midi.type not in (0, 1) or midi.ticks_per_beat <= 0:
        raise ValueError("unsupported MIDI type or division")
    return midi


def ay_period(note: int) -> int:
    if note == 0:
        return 0
    frequency = 440.0 * math.pow(2.0, (note - 69) / 12.0)
    # NABU-LIB/NabuTracker's verified table truncates the calculated period.
    return max(1, min(4095, int(3_579_545.0 / (16.0 * frequency))))


def conversion_events(body: bytes) -> tuple[list[tuple[int, int, int, int]], dict[str, object]]:
    """Return tempo-aware 60 Hz state spans and conversion evidence."""
    midi = validate_midi(body)
    messages = list(mido.merge_tracks(midi.tracks))
    tempo = 500_000
    seconds = 0.0
    active: dict[tuple[int, int], tuple[int, int]] = {}
    serial = 0
    source_note_events = 0
    events: list[tuple[int, int, int, int]] = []

    def state() -> tuple[int, int, int]:
        notes = sorted(active.values(), key=lambda item: (item[0], item[1]), reverse=True)[:2]
        note_b = notes[0][1] if notes else 0
        note_c = notes[1][1] if len(notes) > 1 else 0
        volume_b = min(15, max(1, notes[0][0] // 8)) if notes else 0
        volume_c = min(15, max(1, notes[1][0] // 8)) if len(notes) > 1 else 0
        return note_b, note_c, (volume_b << 4) | volume_c

    def append_span(frames: int, current: tuple[int, int, int]) -> None:
        note_b, note_c, volume = current
        while frames:
            duration = min(255, frames)
            if events and events[-1][1:] == (note_b, note_c, volume) and events[-1][0] + duration <= 255:
                old = events[-1]
                events[-1] = (old[0] + duration, note_b, note_c, volume)
            else:
                events.append((duration, note_b, note_c, volume))
            frames -= duration

    index = 0
    current_state = state()
    current_frame = 0
    while index < len(messages):
        delta_ticks = int(messages[index].time)
        seconds += mido.tick2second(delta_ticks, midi.ticks_per_beat, tempo)
        next_frame = int(seconds * 60.0 + 0.5)
        append_span(max(0, next_frame - current_frame), current_state)
        current_frame = next_frame

        message = messages[index]
        same_tick = [message]
        index += 1
        while index < len(messages) and int(messages[index].time) == 0:
            same_tick.append(messages[index]); index += 1
        for message in same_tick:
            if message.type == "set_tempo":
                tempo = message.tempo
            elif message.type == "note_on" and message.velocity:
                source_note_events += 1
                serial += 1
                if 24 <= message.note <= 96:
                    active[(message.channel, message.note)] = (message.velocity, message.note)
            elif message.type in ("note_off", "note_on"):
                source_note_events += 1
                active.pop((message.channel, message.note), None)
        current_state = state()

    if current_state != (0, 0, 0):
        events.append((1, 0, 0, 0))
    if not events:
        raise ValueError("MIDI has no supported tonal events")

    eligible_count = len(events)
    omitted = 0
    if len(events) > MAX_EVENTS:
        boundary = max((i for i, event in enumerate(events[:MAX_EVENTS])
                        if event[1] == 0 and event[2] == 0), default=-1)
        if boundary < 0:
            raise ValueError(f"no complete silent phrase boundary fits MU01; eligible_events={len(events)}")
        omitted = len(events) - boundary - 1
        events = events[:boundary + 1]
    if events[-1][1] != 0 or events[-1][2] != 0:
        if len(events) >= MAX_EVENTS:
            raise ValueError("MU01 segment has no room for final release")
        events.append((1, 0, 0, 0))
    stats = {
        "ticks_per_beat": midi.ticks_per_beat,
        "source_note_events": source_note_events,
        "eligible_events": eligible_count,
        "final_events": len(events),
        "omitted_events": omitted,
        "duration_frames": sum(event[0] for event in events),
        "duration_seconds": sum(event[0] for event in events) / 60.0,
        "final_silent": events[-1][1] == 0 and events[-1][2] == 0,
    }
    return events, stats


def convert_midi(body: bytes) -> bytes:
    """Convert a complete MIDI to a bounded two-voice NabuTracker event window."""
    events, _ = conversion_events(body)

    record = bytearray(RECORD_SIZE)
    record[:4] = b"MU01"
    record[4] = 1
    record[5] = len(events)
    record[6] = 1  # Mutopia
    record[7] = 1  # Mutopia acquisition; publisher changes this to 2 for cache reuse
    record[8:12] = hashlib.sha256(body).digest()[:4]
    for index, event in enumerate(events):
        duration, note_b, note_c, volume = event
        period_b, period_c = ay_period(note_b), ay_period(note_c)
        offset = EVENT_OFFSET + index * 6
        record[offset:offset + 6] = bytes((duration, period_b & 255, period_b >> 8,
                                           period_c & 255, period_c >> 8, volume))
    checksum = sum(record[:504]) & 0xFFFF
    record[504:506] = checksum.to_bytes(2, "little")
    record[507:511] = b"END!"
    record[511] = 10
    return bytes(record)


def validate_record(record: bytes) -> bool:
    return (len(record) == RECORD_SIZE and record[:4] == b"MU01" and
            0 < record[5] <= MAX_EVENTS and record[6] == 1 and
            record[507:511] == b"END!" and record[511] == 10 and
            int.from_bytes(record[504:506], "little") == (sum(record[:504]) & 0xFFFF))


def publish(store_path: Path, cache_path: Path, timeout: float = 20.0,
            acquire=acquire_midi) -> dict[str, object]:
    midi_path = cache_path / f"{CACHE_STEM}.mid"
    converted_path = cache_path / f"{CACHE_STEM}.mu01"
    metadata_path = cache_path / f"{CACHE_STEM}.json"
    source = "CACHE"
    try:
        cached_metadata = {}
        if metadata_path.is_file():
            try:
                cached_metadata = json.loads(metadata_path.read_text(encoding="utf-8"))
            except (OSError, ValueError):
                cached_metadata = {}
        if (converted_path.is_file() and validate_record(converted_path.read_bytes()) and
                cached_metadata.get("converter_version") == CONVERTER_VERSION):
            body = midi_path.read_bytes()
            record = converted_path.read_bytes()
        elif midi_path.is_file():
            body = midi_path.read_bytes()
            record = convert_midi(body)
            atomic_write(converted_path, record)
        else:
            body = acquire(MUTOPIA_MIDI, timeout)
            record = convert_midi(body)
            if not validate_record(record):
                raise ValueError("converted MU01 validation failed")
            atomic_write(midi_path, body)
            atomic_write(converted_path, record)
            source = "MUTOPIA"
        published = bytearray(record)
        published[7] = 1 if source == "MUTOPIA" else 2
        published[504:506] = (sum(published[:504]) & 0xFFFF).to_bytes(2, "little")
        metadata = {
            "title": TITLE, "composer": COMPOSER, "mutopia_id": WORK_ID,
            "page_url": MUTOPIA_PAGE, "midi_url": MUTOPIA_MIDI,
            "style": STYLE, "license": LICENSE,
            "acquired_utc": int(time.time()), "acquisition_status": source,
            "source_midi_size": len(body), "source_sha256": hashlib.sha256(body).hexdigest(),
            "conversion_result": "MU01_OK", "converted_size": len(record),
            "converter_version": CONVERTER_VERSION,
            "converted_sha256": hashlib.sha256(record).hexdigest(),
        }
        atomic_write(metadata_path, json.dumps(metadata, indent=2, sort_keys=True).encode("utf-8") + b"\n")
        atomic_write(store_path / MUSIC_NAME, bytes(published))
        return metadata
    except Exception:
        if converted_path.is_file():
            cached = converted_path.read_bytes()
            if validate_record(cached):
                atomic_write(store_path / MUSIC_NAME, cached)
        raise


def run(store_path: Path, stop, cache_path: Path | None = None) -> None:
    cache = cache_path or (Path(__file__).resolve().parent / "cache" / "music")
    while not stop.is_set():
        try:
            publish(store_path, cache)
        except Exception as exc:
            print(f"music source unavailable; validated cache preserved: {exc}", flush=True)
        stop.wait(3600.0)
