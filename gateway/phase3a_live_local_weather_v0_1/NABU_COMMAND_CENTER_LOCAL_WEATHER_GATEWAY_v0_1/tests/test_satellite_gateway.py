import struct
import tempfile
import unittest
from pathlib import Path

import satellite_gateway


SAMPLE = {
    "name": "iss",
    "id": 25544,
    "latitude": 25.7617,
    "longitude": -80.1918,
    "altitude": 418.6,
    "velocity": 27584.2,
    "visibility": "daylight",
    "footprint": 4501.4,
    "timestamp": 1787220000,
    "units": "kilometers",
}


class SatelliteGatewayTests(unittest.TestCase):
    def test_record_is_bounded_and_contains_normalized_live_fields(self):
        record = satellite_gateway.build_record(SAMPLE, 7)
        self.assertEqual(len(record), 64)
        self.assertEqual(record[:4], b"SA01")
        self.assertEqual(record[4:6], bytes((7, 1)))
        self.assertEqual(struct.unpack_from("<h", record, 6)[0], 2576)
        self.assertEqual(struct.unpack_from("<h", record, 8)[0], -8019)
        self.assertEqual(struct.unpack_from("<H", record, 10)[0], 419)
        self.assertEqual(struct.unpack_from("<H", record, 12)[0], 2758)
        self.assertEqual(struct.unpack_from("<I", record, 14)[0], 1787220000)
        self.assertEqual(record[18:23], b"ISS  ")
        self.assertEqual(record[23:28], b"25544")
        self.assertEqual(record[28], 1)
        self.assertEqual(struct.unpack_from("<H", record, 29)[0], 4501)
        self.assertEqual(struct.unpack_from("<H", record, 60)[0], sum(record[:60]) & 0xFFFF)
        self.assertEqual(record[62:], b"E\n")

    def test_wrong_satellite_is_rejected(self):
        bad = dict(SAMPLE, id=12345)
        with self.assertRaises(ValueError):
            satellite_gateway.build_record(bad, 0)

    def test_failed_fetch_preserves_last_valid_resource(self):
        with tempfile.TemporaryDirectory() as directory:
            destination = Path(directory) / satellite_gateway.RESOURCE_NAME
            destination.write_bytes(b"previous-valid")

            def fail():
                raise RuntimeError("provider unavailable")

            with self.assertRaises(RuntimeError):
                satellite_gateway.publish_once(directory, 1, fail)
            self.assertEqual(destination.read_bytes(), b"previous-valid")

    def test_successful_publish_replaces_resource_with_exact_record(self):
        with tempfile.TemporaryDirectory() as directory:
            record = satellite_gateway.publish_once(directory, 9, lambda: SAMPLE)
            self.assertEqual(
                (Path(directory) / satellite_gateway.RESOURCE_NAME).read_bytes(), record
            )


if __name__ == "__main__":
    unittest.main()
