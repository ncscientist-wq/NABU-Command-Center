import json
import os
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import weather_gateway as wg


class WeatherGatewayTests(unittest.TestCase):
    def test_weather_record_exact_64_and_identity(self):
        record = wg.build_weather_record("000007", "02108", 76, "CLEAR___", 5, 1013, 1786038180)
        self.assertEqual(len(record), 64)
        self.assertTrue(record.startswith(b"NCC|1|WX|000007|02108|076|CLEAR___|005|1013|"))
        self.assertTrue(record.endswith(b"|END\n"))
        body, integrity, end = record.rsplit(b"|", 2)
        self.assertEqual(integrity, f"{wg.checksum16(body + b'|'):04X}".encode())
        self.assertEqual(end, b"END\n")

    def test_location_record_exact_64_and_bounded(self):
        record = wg.build_location_record("000007", "85234", "Gilbert", "AZ", True)
        self.assertEqual(len(record), 64)
        self.assertEqual(record[:18], b"LC0100000785234\x01\x07\x02")
        self.assertEqual(record[18:25], b"GILBERT")
        self.assertEqual(record[34:36], b"AZ")
        self.assertEqual(int.from_bytes(record[57:59], "little"), sum(record[:57]) & 0xFFFF)
        self.assertEqual(record[59:], b"END!\n")

    def test_unresolved_location_is_truthful_and_bounded(self):
        record = wg.build_location_record("000008", "00000")
        self.assertEqual(record[15:18], bytes((0, 7, 2)))
        self.assertEqual(record[18:25], b"UNKNOWN")
        self.assertEqual(record[34:36], b"--")

    def test_location_normalization_bounds_city(self):
        self.assertEqual(wg.normalize_location("Saint-Mary's Very Long Place", "az"),
                         ("SAINT-MARY S VER", "AZ"))

    def test_history_record_is_bounded_and_rolls_to_latest_twelve(self):
        samples = [(60 + index, 1000 + index, index) for index in range(14)]
        record = wg.build_history_record("000007", "02108", samples, 1786038180)
        self.assertEqual(len(record), 64)
        self.assertEqual(record[:15], b"WXH100000702108")
        self.assertEqual(record[15], 12)
        self.assertEqual(record[21:24], bytes((162, 102, 2)))
        self.assertEqual(int.from_bytes(record[57:59], "little"), sum(record[:57]) & 0xFFFF)
        self.assertEqual(record[59:], b"END!\n")

    def test_condition_normalization(self):
        self.assertEqual(wg.normalize_condition("Light Rain"), "RAIN____")
        self.assertEqual(wg.normalize_condition("Mostly Cloudy"), "CLOUDY__")
        self.assertEqual(wg.normalize_condition("\u2603 unknown"), "UNKNOWN_")

    def test_zcta_parser_preserves_leading_zero(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "zcta.txt"
            path.write_text("GEOID|INTPTLAT|INTPTLONG\n02108|42.357|-71.064\n", encoding="ascii")
            self.assertEqual(wg.load_zcta(path)["02108"], (42.357, -71.064))

    def test_zcta_parser_rejects_malformed_source(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "bad.txt"
            path.write_text("BAD|LAT|LON\n", encoding="ascii")
            with self.assertRaises(ValueError):
                wg.load_zcta(path)

    def test_acquire_weather_converts_and_uses_discovery(self):
        replies = [
            {"properties": {
                "observationStations": "https://example/stations",
                "relativeLocation": {"properties": {"city": "Gilbert", "state": "AZ"}},
            }},
            {"features": [{"properties": {"stationIdentifier": "KXYZ"}}]},
            {"properties": {
                "temperature": {"value": 20.0, "unitCode": "wmoUnit:degC"},
                "windSpeed": {"value": 16.09344, "unitCode": "wmoUnit:km_h-1"},
                "barometricPressure": {"value": 101325.0, "unitCode": "wmoUnit:Pa"},
                "textDescription": "Mostly Cloudy",
                "timestamp": "2026-08-11T01:02:03+00:00",
            }},
        ]
        with patch.object(wg, "get_json", side_effect=replies):
            result = wg.acquire_weather(34.0, -118.0, "test-agent", 1.0)
        self.assertEqual(result[:4], (68, "CLOUDY__", 10, 1013))
        self.assertEqual(result[5], "KXYZ")
        self.assertEqual(result[6:], ("GILBERT", "AZ"))

    def test_required_null_is_not_fabricated(self):
        replies = [
            {"properties": {
                "observationStations": "https://example/stations",
                "relativeLocation": {"properties": {"city": "Los Angeles", "state": "CA"}},
            }},
            {"features": [{"properties": {"stationIdentifier": "KXYZ"}}]},
            {"properties": {
                "temperature": {"value": None, "unitCode": "wmoUnit:degC"},
                "windSpeed": {"value": 0.0, "unitCode": "wmoUnit:km_h-1"},
                "barometricPressure": {"value": 101325.0, "unitCode": "wmoUnit:Pa"},
                "textDescription": "Clear",
                "timestamp": "2026-08-11T01:02:03+00:00",
            }},
        ]
        with patch.object(wg, "get_json", side_effect=replies):
            with self.assertRaisesRegex(ValueError, "null"):
                wg.acquire_weather(34.0, -118.0, "test-agent", 1.0)

    def test_atomic_write(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "ncc_weather.dat"
            wg.atomic_write(path, b"first")
            wg.atomic_write(path, b"second")
            self.assertEqual(path.read_bytes(), b"second")

    def test_unexpected_units_are_rejected(self):
        props = {"temperature": {"value": 68.0, "unitCode": "wmoUnit:degF"}}
        with self.assertRaisesRegex(ValueError, "unexpected"):
            wg.qv_value(props, "temperature", "wmoUnit:degC")

    def test_identical_request_rewrite_has_new_signature(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / wg.REQUEST_NAME
            content = b"ZIP|000001|90210\n"
            path.write_bytes(content)
            first = wg.request_signature(path)
            path.write_bytes(content)
            os.utime(path, ns=(first[0] + 1_000_000, first[0] + 1_000_000))
            second = wg.request_signature(path)
            self.assertNotEqual(first, second)
            self.assertEqual(first[2], second[2])

    def test_publish_weather_preserves_exact_request_identity(self):
        with tempfile.TemporaryDirectory() as folder, patch.object(
                wg, "acquire_weather", return_value=(72, "CLEAR___", 8, 1013, 1786038180, "KXYZ", "GILBERT", "AZ")):
            path = Path(folder) / wg.WEATHER_NAME
            location_path = Path(folder) / wg.LOCATION_NAME
            record, location_record, station = wg.publish_weather(
                path, "000005", "90210", (34.0, -118.0), "agent", 1.0,
                location_path=location_path)
            self.assertEqual(station, "KXYZ")
            self.assertEqual(path.read_bytes(), record)
            self.assertIn(b"|000005|90210|", record)
            self.assertEqual(location_path.read_bytes(), location_record)
            self.assertEqual(location_record[18:25], b"GILBERT")


if __name__ == "__main__":
    unittest.main()
