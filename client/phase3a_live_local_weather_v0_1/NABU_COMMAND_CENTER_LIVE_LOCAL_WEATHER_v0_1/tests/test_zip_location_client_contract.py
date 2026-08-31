import unittest
from pathlib import Path


CLIENT = Path(__file__).resolve().parents[1] / "main.c"


class ZipLocationClientContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source = CLIENT.read_text(encoding="utf-8")

    def test_location_uses_existing_bounded_store_transport(self):
        self.assertIn('#define LOCATION_NAME "ncc_location.dat"', self.source)
        self.assertIn("protected_file_read(handle,(unsigned char *)record_buffer,0,0,RECORD_LENGTH)", self.source)
        self.assertIn("refresh_weather(); refresh_location();", self.source)

    def test_parser_validates_identity_before_cache_commit(self):
        self.assertIn("validate_location_record", self.source)
        self.assertIn("if(received!=calculated) return 0;", self.source)
        self.assertIn("if(!text_equal(selected_zip,zip_code)||!zip_request_valid) return 0;", self.source)
        self.assertIn("if(token[i]!=zip_request[4+i]) return 0;", self.source)
        commit = "location_resolved=status; has_location=1; format_location_labels();"
        self.assertIn(commit, self.source)
        self.assertGreater(self.source.index(commit), self.source.index("if(received!=calculated) return 0;"))

    def test_city_and_state_are_fixed_bounded_fields(self):
        self.assertIn("static char location_city[17]", self.source)
        self.assertIn("static char location_state[3]", self.source)
        self.assertIn("city_length>16", self.source)
        self.assertIn("state_length!=2", self.source)

    def test_unknown_is_truthful_without_hardcoded_profiles(self):
        self.assertIn('text_copy_bounded(location_label,"UNKNOWN"', self.source)
        self.assertNotIn("profile_zip[", self.source)
        self.assertNotIn("profile_city[", self.source)
        self.assertNotIn("profile_region[", self.source)

    def test_weather_detail_uses_resolved_location(self):
        self.assertIn("const char *place=profile_name();", self.source)
        self.assertIn('text_copy_bounded(title,"WEATHER "', self.source)


if __name__ == "__main__":
    unittest.main()
