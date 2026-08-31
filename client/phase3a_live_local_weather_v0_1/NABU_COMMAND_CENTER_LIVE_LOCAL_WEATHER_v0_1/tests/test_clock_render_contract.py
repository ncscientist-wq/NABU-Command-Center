import unittest
from pathlib import Path


CLIENT = Path(__file__).resolve().parents[1] / "main.c"


class ClockRenderContractTest(unittest.TestCase):
    def test_full_clock_field_is_cleared_before_complete_redraw(self):
        source = CLIENT.read_text(encoding="utf-8")
        start = source.index("static void clock_draw_value(void)")
        end = source.index("static void clock_frame_isr(void)", start)
        body = source[start:end]
        clear = "for(y=13;y<20;++y) undraw(86,y,174,y);"
        redraw = "micro_text(86,13,value);"
        self.assertIn(clear, body)
        self.assertIn(redraw, body)
        self.assertLess(body.index(clear), body.index(redraw))
        self.assertNotIn("draw_dashboard", body)
        self.assertNotIn("static char drawn", body)


if __name__ == "__main__":
    unittest.main()
