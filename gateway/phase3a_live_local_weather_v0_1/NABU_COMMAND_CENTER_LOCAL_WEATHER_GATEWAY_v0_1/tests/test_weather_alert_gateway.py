import unittest

import weather_alert_gateway as alerts


class WeatherAlertGatewayTests(unittest.TestCase):
    def test_active_alert_fixed_record(self):
        state = alerts.normalize({"features": [{"properties": {
            "severity": "Severe", "headline": "Severe Thunderstorm Warning"
        }}]})
        record = alerts.build_record(7, "000123", "33101", state, 1787216400)
        self.assertEqual(len(record), 64)
        self.assertEqual(record[:4], b"WA01")
        self.assertEqual(record[4:15], b"00012333101")
        self.assertEqual(record[16:19], bytes((3, 1, len(state["text"]))))
        self.assertEqual(record[61:63], (sum(record[:61]) & 0xffff).to_bytes(2, "little"))
        self.assertEqual(record[63], 10)

    def test_no_alert_truthful_message(self):
        state = alerts.normalize({"features": []})
        self.assertEqual(state["count"], 0)
        self.assertEqual(state["text"], "NO ACTIVE NOAA/NWS ALERTS")


if __name__ == "__main__":
    unittest.main()
