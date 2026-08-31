import re
import unittest
from pathlib import Path

SOURCE = Path(__file__).resolve().parents[1] / "main.c"


class MusicServiceContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.text = SOURCE.read_text(encoding="utf-8")

    def test_bounded_foreground_player_and_no_new_interrupt(self):
        self.assertIn("static unsigned char music_buffer[MUSIC_RECORD_SIZE];", self.text)
        self.assertIn("#define MUSIC_RECORD_SIZE 512", self.text)
        service = self.text.split("static void service_music(void)", 1)[1].split("static void start_cue", 1)[0]
        self.assertNotRegex(service, r"rn_file|malloc|calloc|realloc|intrinsic_|interrupt")
        self.assertIn("now=clock_frame_counter", service)
        self.assertIn("elapsed=(unsigned char)(now-music_frame_mark)", service)
        self.assertIn("processed<MUSIC_MAX_EVENTS", service)
        self.assertIn("elapsed=(unsigned char)(elapsed-music_wait)", service)
        self.assertNotIn("*180U", service)

    def test_channels_and_safe_mixer_ownership(self):
        event = self.text.split("static void music_event", 1)[1].split("static void service_music", 1)[0]
        self.assertIn("set_psg(9", event)
        self.assertIn("set_psg(10", event)
        self.assertNotIn("set_psg(8", event)
        self.assertIn("get_psg(7)", event)
        cue = self.text.split("static void ay_note", 1)[1].split("static void weather_diag_set", 1)[0]
        self.assertIn("set_psg(8, 10)", cue)
        self.assertIn("music_silence(); music_loop_pending=0", self.text)

    def test_controls_banner_and_refresh_contract(self):
        self.assertRegex(self.text, r"(?s)key=='b'.*?music_enabled")
        self.assertNotRegex(self.text, r"(?s)key=='c'.*?music")
        refresh = self.text.split("static unsigned char service_global_refresh", 1)[1].split("static unsigned char validate_weather_history", 1)[0]
        self.assertIn("refresh_music();", refresh)
        loader = self.text.split("static void refresh_music", 1)[1].split("static void gate0_endurance", 1)[0]
        self.assertIn("if(music_valid) return", loader)
        self.assertIn('"MUSIC ON MUTOPIA"', self.text)
        self.assertIn('"MUSIC ON CACHE"', self.text)

    def test_existing_architecture_unchanged(self):
        self.assertEqual(self.text.count("int main(void)"), 1)
        self.assertEqual(self.text.count("static void service_scheduler(void)\n{"), 1)
        self.assertNotIn("MUSIC_EVENT_COUNT", self.text)


if __name__ == "__main__": unittest.main()
