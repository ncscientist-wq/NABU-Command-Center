import datetime as dt
import importlib.util
import logging
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
SPEC = importlib.util.spec_from_file_location("nist_time_gateway", ROOT / "nist_time_gateway.py")
mod = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
sys.modules[SPEC.name] = mod
SPEC.loader.exec_module(mod)


def response(token: bytes, *, li=0, version=4, mode=4, stratum=2,
             transmit_seconds=mod.NTP_EPOCH_DELTA + 1_786_200_100) -> bytes:
    packet = bytearray(48)
    packet[0] = (li << 6) | (version << 3) | mode
    packet[1] = stratum
    packet[24:32] = token
    packet[40:48] = struct.pack("!II", transmit_seconds, 0)
    return bytes(packet)


class NistGatewayTests(unittest.TestCase):
    token = struct.pack("!II", mod.NTP_EPOCH_DELTA + 100, 123)

    def test_fixed_utc_time_record_is_exact_64_bytes(self):
        result = {"utc": dt.datetime(2026, 8, 10, 12, 34, tzinfo=dt.timezone.utc),
                  "unix_seconds": 1_786_368_840, "stratum": 2}
        record = mod.build_time_record(1, result)
        self.assertEqual(len(record), 64)
        self.assertEqual(record, b"NCC|1|TIME|000001|1786368840|17|0002|2026-08-10T12:34Z|0E11|END\n")

    def test_valid_response(self):
        result = mod.validate_response(response(self.token), self.token)
        self.assertEqual(result["mode"], 4)
        self.assertEqual(result["version"], 4)
        self.assertEqual(result["stratum"], 2)

    def test_short_response_rejected(self):
        with self.assertRaisesRegex(mod.NtpError, "short"):
            mod.validate_response(b"x" * 47, self.token)

    def test_invalid_mode_rejected(self):
        with self.assertRaisesRegex(mod.NtpError, "mode"):
            mod.validate_response(response(self.token, mode=3), self.token)

    def test_unsynchronized_leap_rejected(self):
        with self.assertRaisesRegex(mod.NtpError, "unsynchronized"):
            mod.validate_response(response(self.token, li=3), self.token)

    def test_stratum_zero_kod_rejected(self):
        packet = bytearray(response(self.token, stratum=0)); packet[12:16] = b"RATE"
        with self.assertRaisesRegex(mod.NtpError, "RATE"):
            mod.validate_response(bytes(packet), self.token)

    def test_bad_origin_rejected(self):
        with self.assertRaisesRegex(mod.NtpError, "originate"):
            mod.validate_response(response(b"12345678"), self.token)

    def test_zero_transmit_rejected(self):
        with self.assertRaisesRegex(mod.NtpError, "zero"):
            mod.validate_response(response(self.token, transmit_seconds=0), self.token)

    def test_failed_acquisition_does_not_advance_sequence(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td); state = root / mod.STATE_NAME
            state.write_text('{"sequence": 7}\n', encoding="ascii")
            with mock.patch.object(mod, "acquire_nist", side_effect=mod.NtpError("failed")):
                with self.assertRaises(mod.NtpError):
                    mod.publish_once(root, mod.NIST_HOST, 1, logging.getLogger("test"))
            self.assertEqual(mod.next_sequence(state), 8)
            self.assertFalse((root / mod.RESOURCE_NAME).exists())

    def test_atomic_valid_publish_advances_once(self):
        fixed = {"utc": dt.datetime(2026, 8, 10, 12, 34, tzinfo=dt.timezone.utc),
                 "unix_seconds": 1_786_368_840, "stratum": 2,
                 "hostname": mod.NIST_HOST, "resolved_ip": "192.0.2.1",
                 "response_source": "192.0.2.1:123", "response_length": 48,
                 "mode": 4, "version": 4, "li": 0}
        with tempfile.TemporaryDirectory() as td, mock.patch.object(
                mod, "acquire_nist", side_effect=[fixed.copy(), fixed.copy()]):
            root = Path(td)
            first = mod.publish_once(root, mod.NIST_HOST, 1, logging.getLogger("test"))
            second = mod.publish_once(root, mod.NIST_HOST, 1, logging.getLogger("test"))
            self.assertEqual((first["sequence"], second["sequence"]), (1, 2))
            self.assertEqual(len((root / mod.RESOURCE_NAME).read_bytes()), 64)
            self.assertFalse(list(root.glob("*.tmp")))

    def test_monotonic_holdover_advances_without_wall_clock(self):
        synchronized = {
            "utc": dt.datetime(2026, 8, 11, 6, 26, tzinfo=dt.timezone.utc),
            "unix_seconds": 1_786_426_760, "stratum": 2,
        }
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            result = mod.publish_holdover(root, synchronized, 65, logging.getLogger("test"))
            self.assertEqual(result["unix_seconds"], 1_786_426_825)
            self.assertEqual(result["authority"], "NIST disciplined monotonic holdover")
            self.assertEqual(len((root / mod.RESOURCE_NAME).read_bytes()), 64)
            self.assertEqual(result["sequence"], 1)


if __name__ == "__main__":
    unittest.main(verbosity=2)
