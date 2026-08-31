import unittest
from pathlib import Path


CLIENT = Path(__file__).resolve().parents[1] / "main.c"


class SatelliteRendererContractTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        source = CLIENT.read_text(encoding="utf-8")
        plot_start = source.index("static void draw_satellite_plot(void)")
        rail_start = source.index("static void draw_satellite_rail(void)", plot_start)
        detail_start = source.index("static void draw_satellite_detail(void)", rail_start)
        cls.plot = source[plot_start:rail_start]
        cls.rail = source[rail_start:detail_start]
        cls.detail = source[detail_start:source.index("static void draw_weather_plot(void)", detail_start)]

    def test_source_is_live_and_generic_mock_rail_is_not_used(self):
        self.assertIn('"SOURCE LIVE"', self.rail)
        self.assertNotIn('"MOCK"', self.rail)
        self.assertNotIn("draw_telemetry_rail", self.detail)

    def test_orbital_plot_contains_no_telemetry_text(self):
        for marker in ("satellite_position_text", "satellite_motion_text", '"LAT----', '"ALT----'):
            self.assertNotIn(marker, self.plot)

    def test_right_column_clears_before_live_values(self):
        clear = "for(y=43;y<158;++y) undraw(180,y,SAFE_RIGHT,y);"
        first_value = 'micro_text(180,43,has_satellite?"STATUS LIVE":"STATUS WAIT");'
        self.assertIn(clear, self.rail)
        self.assertIn(first_value, self.rail)
        self.assertLess(self.rail.index(clear), self.rail.index(first_value))
        for label in ('"TARGET "', '"NORAD "', '"LAT "', '"LON "', '"ALT "', '"VEL "', '"VIS "'):
            self.assertIn(label, self.rail)
        self.assertNotIn('"PROFILE"', self.rail)
        self.assertNotIn('"SEV"', self.rail)


if __name__ == "__main__":
    unittest.main()
