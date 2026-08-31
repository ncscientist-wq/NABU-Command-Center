import struct
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

import airspace_gateway as ag


FIXTURE = {"ac": [
    {"hex": "abc123", "flight": " FAR2 ", "lat": 33.50, "lon": -111.70, "alt_baro": 12000, "gs": 240, "track": 90},
    {"hex": "def456", "flight": "", "lat": 33.31, "lon": -111.81, "alt_baro": 5000, "gs": 120, "track": 270},
    {"hex": "badpos", "flight": "BAD", "alt_baro": 1},
]}


class StopAfterTwo:
    def __init__(self): self.calls = 0
    def is_set(self): return self.calls >= 2
    def wait(self, _): self.calls += 1


class AirspaceGatewayTests(unittest.TestCase):
    def test_dynamic_map_scale_quadrants_and_nearby_declutter(self):
        center = (33.30, -111.80)
        nearby = {"ac": [
            {"hex": "100001", "lat": 33.30, "lon": -111.7800},
            {"hex": "100002", "lat": 33.30, "lon": -111.8400},
            {"hex": "100003", "lat": 33.3667, "lon": -111.80},
        ]}
        tracks = ag.reduce_aircraft(nearby, center)
        self.assertEqual(len({(item["x"], item["y"]) for item in tracks}), 3)
        self.assertEqual([item["distance"] for item in tracks], sorted(item["distance"] for item in tracks))
        self.assertGreaterEqual(tracks[0]["display_radius"], ag.MIN_DISPLAY_RADIUS_NM)
        self.assertLessEqual(tracks[0]["display_radius"], ag.AIRSPACE_RADIUS_NM)
        self.assertGreater(tracks[0]["x"], ag.PLOT_CENTER_X) # east
        self.assertLess(tracks[1]["x"], ag.PLOT_CENTER_X)    # west
        self.assertGreater(tracks[0]["y"], tracks[1]["y"])
        self.assertGreater(tracks[1]["y"], tracks[2]["y"])  # near is lower than far

        one = ag.reduce_aircraft({"ac": [{"hex": "200001", "lat": 33.3083, "lon": -111.80}]}, center)
        self.assertEqual(one[0]["display_radius"], ag.MIN_DISPLAY_RADIUS_NM)
        self.assertGreater(one[0]["y"], 130)
        self.assertEqual(ag.reduce_aircraft({"ac": []}, center), [])

    def test_fixture_mapping_selection_zero_and_contract(self):
        aircraft = ag.reduce_aircraft(FIXTURE, (33.30, -111.80))
        self.assertEqual([item["icao"] for item in aircraft], ["DEF456", "ABC123"])
        self.assertEqual(aircraft[0]["call"], "DEF456")
        record = ag.build_record(aircraft, 7)
        self.assertEqual(len(record), 64)
        self.assertEqual(record[:4], b"AS01")
        self.assertEqual(record[7], 2)
        self.assertLessEqual(record[7], ag.MAX_AIRCRAFT)
        self.assertEqual(struct.unpack_from("<H", record, 60)[0], sum(record[:60]) & 0xFFFF)
        self.assertEqual(record[62:], b"E\n")
        zero = ag.build_record([], 8)
        self.assertEqual((zero[5], zero[7]), (ag.STATE_LIVE, 0))

    def test_failure_publishes_stale_with_last_valid_slots(self):
        responses = [("ADSBLOL", FIXTURE), RuntimeError("timeout")]
        def fetch(*_args, **_kwargs):
            result = responses.pop(0)
            if isinstance(result, Exception): raise result
            return result
        writes = []
        with tempfile.TemporaryDirectory() as folder, \
             patch.object(ag, "current_profile_center", return_value=("85234", 33.30, -111.80)), \
             patch.object(ag, "fetch_aircraft", side_effect=fetch), \
             patch.object(ag, "_atomic_write", side_effect=lambda _path, data: writes.append(data)):
            ag.run(Path(folder), Path("weather.ini"), StopAfterTwo())
        self.assertEqual(len(writes), 2)
        self.assertEqual(writes[0][5], ag.STATE_LIVE)
        self.assertEqual(writes[1][5], ag.STATE_STALE)
        self.assertEqual(writes[0][7], writes[1][7])
        self.assertEqual(writes[0][8:59], writes[1][8:59])


if __name__ == "__main__":
    unittest.main()
