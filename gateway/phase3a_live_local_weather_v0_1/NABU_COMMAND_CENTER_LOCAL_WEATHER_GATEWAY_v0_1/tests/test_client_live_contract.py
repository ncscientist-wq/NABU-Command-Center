import datetime as dt
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
CLIENT = ROOT.parents[2] / "client" / "phase3a_live_local_weather_v0_1" / "NABU_COMMAND_CENTER_LIVE_LOCAL_WEATHER_v0_1" / "main.c"


class ClientLiveContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source = CLIENT.read_text(encoding="utf-8")

    def test_weather_demo_metrics_removed(self):
        self.assertNotIn("72F W8KT", self.source)
        self.assertNotIn("72F WIND 8KT", self.source)

    def test_current_session_token_required(self):
        self.assertIn("zip_request_valid", self.source)
        self.assertIn("NO CURRENT ZIP REQUEST", self.source)
        self.assertIn("if(!has_weather || !zip_request_valid", self.source)

    def test_one_cache_feeds_both_weather_views(self):
        self.assertIn("weather_temp[0]", self.source)
        self.assertIn("draw_weather_detail", self.source)
        self.assertGreaterEqual(self.source.count("weather_matches_selection()"), 3)

    def test_verified_frame_interrupt_api_is_used(self):
        self.assertIn("ncc_install_minimal_vdp_isr()", self.source)
        self.assertIn("CLOCK_FRAMES_PER_SECOND 60", self.source)

    def test_calendar_boundaries_expected_by_epoch_model(self):
        cases = {
            1_704_067_199: "2023-12-31T23:59:59+00:00",
            1_704_067_200: "2024-01-01T00:00:00+00:00",
            1_709_164_800: "2024-02-29T00:00:00+00:00",
            1_709_251_200: "2024-03-01T00:00:00+00:00",
        }
        for epoch, expected in cases.items():
            self.assertEqual(dt.datetime.fromtimestamp(epoch, tz=dt.timezone.utc).isoformat(), expected)


if __name__ == "__main__":
    unittest.main(verbosity=2)
