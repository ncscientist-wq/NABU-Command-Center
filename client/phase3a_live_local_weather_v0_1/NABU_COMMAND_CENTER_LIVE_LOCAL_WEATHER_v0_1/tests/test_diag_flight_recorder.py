"""Deterministic host model/static checks for the NABU flight recorder."""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
SOURCE = (ROOT / "main.c").read_text(encoding="utf-8")


class Ring:
    def __init__(self):
        self.slots = [bytearray(8) for _ in range(32)]
        self.head = 0
        self.sequence = 0

    def append(self, event, context=0, arg0=0, arg1=0, state=0, tick=0):
        self.sequence = (self.sequence + 1) & 0xFFFF
        slot = self.slots[self.head]
        slot[0] = 0                       # invalidate first
        slot[1:] = bytes((self.sequence & 0xFF, self.sequence >> 8,
                          context, arg0, arg1, state, tick))
        slot[0] = event                   # commit last
        self.head = (self.head + 1) & 31


def test_model():
    ring = Ring()
    assert all(slot[0] == 0 for slot in ring.slots)
    ring.append(0x21, 1, 1, 0xFF)
    assert ring.head == 1 and ring.sequence == 1
    assert ring.slots[0] == bytes((0x21, 1, 0, 1, 1, 0xFF, 0, 0))
    for event in range(2, 35):
        ring.append((event & 0x7F) or 1)
    assert ring.head == 2 and ring.sequence == 34
    assert ring.slots[1][1:3] == bytes((34, 0))


def test_source_contract():
    assert "#define DIAG_RING_DEPTH 32" in SOURCE
    assert "slot->event_id=0;" in SOURCE
    assert SOURCE.index("slot->event_id=0;") < SOURCE.index("slot->event_id=event_id;")
    assert "(DIAG_RING_DEPTH-1)" in SOURCE
    assert "ncc_diag_pre_guard" in SOURCE and "ncc_diag_post_guard" in SOURCE
    assert "++ncc_diag_time_attempt" in SOURCE
    assert "++ncc_diag_weather_attempt" in SOURCE
    ids = [int(value, 16) for value in re.findall(r"#define DE_\w+ (0x[0-9A-F]+)", SOURCE)]
    assert 0 not in ids and len(ids) == len(set(ids))
    isr = SOURCE.split("static void clock_frame_isr(void)", 1)[1].split("static void clock_draw_value", 1)[0]
    assert "diag_log" not in isr and "diag_visible" not in isr
    logger = SOURCE.split("static void diag_log", 1)[1].split("static void diag_init", 1)[0]
    for forbidden in ("rn_file", "printf", "sprintf", "malloc", "clg(", "micro_text", "set_psg"):
        assert forbidden not in logger
    assert "#if NCC_DIAG_FLIGHT_RECORDER" in logger


if __name__ == "__main__":
    test_model()
    test_source_contract()
    print("PASS: flight recorder model and source contract")
