import unittest

import space_weather_gateway as sw


class SpaceWeatherGatewayTests(unittest.TestCase):
    def test_normalize_and_fixed_record(self):
        kp = [{"time_tag": "2026-08-20T09:00:00", "Kp": value} for value in (1.0, 2.0, 3.2)]
        wind = [["time_tag", "speed"], ["2026-08-20 08:59:00.000", "440"], ["2026-08-20 09:00:00.000", "552"]]
        state = sw.normalize(kp, wind, [{"bt": 6, "bz_gsm": -2}], [{"flux": 126}])
        record = sw.build_record(7, state)
        self.assertEqual(len(record), 64)
        self.assertEqual(record[:4], b"SW01")
        self.assertEqual(record[5], 32)
        self.assertEqual(int.from_bytes(record[6:8], "little"), 552)
        self.assertEqual(record[60:62], (sum(record[:60]) & 0xFFFF).to_bytes(2, "little"))
        self.assertEqual(record[62:], b"E\n")


if __name__ == "__main__":
    unittest.main()
