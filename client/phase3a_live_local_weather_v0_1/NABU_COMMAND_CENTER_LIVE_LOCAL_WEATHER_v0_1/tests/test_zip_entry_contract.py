import unittest
from pathlib import Path


CLIENT = Path(__file__).resolve().parents[1] / "main.c"


def enter_keys(keys):
    entered = []
    submitted = None
    for key in keys:
        if key == "BACKSPACE":
            if entered:
                entered.pop()
        elif key == "ENTER":
            if len(entered) == 5:
                submitted = "".join(entered)
        elif len(key) == 1 and key.isdigit() and len(entered) < 5:
            entered.append(key)
    return "".join(entered), submitted


class ZipEntryContractTest(unittest.TestCase):
    def test_digits_backspace_empty_and_corrected_submit(self):
        source = CLIENT.read_text(encoding="utf-8")
        self.assertIn("#define KEY_BACKSPACE 0x08", source)
        self.assertIn("#define KEY_DELETE 0x7F", source)
        deletion = "if ((key==KEY_BACKSPACE) || (key==KEY_DELETE))"
        self.assertIn(deletion, source)
        self.assertLess(source.index(deletion), source.index("else if ((key>='0') && (key<='9')"))
        self.assertIn("if(count>0)", source)
        self.assertIn("--count; entered[count]=0;", source)
        self.assertIn("static void zip_draw_cell", source)
        self.assertIn("undraw(x,(unsigned char)(91+row)", source)
        self.assertIn("zip_draw_cell(count,0);", source)
        self.assertIn("zip_draw_cell(count,(char)key);", source)
        remaining, submitted = enter_keys([
            "BACKSPACE", "8", "5", "2", "3", "5", "BACKSPACE", "4", "ENTER"
        ])
        self.assertEqual(remaining, "85234")
        self.assertEqual(submitted, "85234")


if __name__ == "__main__":
    unittest.main()
