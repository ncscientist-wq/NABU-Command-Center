import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import earthquake_gateway as eq


class EarthquakeGatewayTests(unittest.TestCase):
    def test_bounded_record_and_selection(self):
        now = 1_800_000_000_000
        def feature(event_id, mag, lon, lat, depth, place, age=60000):
            return {"id": event_id, "properties": {"mag": mag, "place": place, "time": now-age},
                    "geometry": {"coordinates": [lon, lat, depth]}}
        feed = {"features": [
            feature("local1", 3.2, -80.2, 25.8, 12, "10 km S of Miami, Florida"),
            feature("local2", 4.1, -66.1, 18.4, 20, "Puerto Rico"),
            feature("global1", 6.4, 140.1, 36.2, 33, "Honshu, Japan"),
            feature("global2", 5.9, -150.0, 61.0, 45, "Alaska"),
        ]}
        local, global_events = eq.select_events(feed, (25.76, -80.19), now)
        record = eq.build_record(7, local, global_events)
        self.assertEqual((len(local), len(global_events)), (2, 2))
        self.assertEqual(len(record), 64)
        self.assertEqual(record[:4], b"EQ01")
        self.assertEqual(int.from_bytes(record[60:62], "little"), sum(record[:60]) & 0xFFFF)
        self.assertEqual(record[63], 10)


if __name__ == "__main__":
    unittest.main()
