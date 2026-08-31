import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from zip_request_receiver import parse_request


class ZipRequestTests(unittest.TestCase):
    def test_required_zip_examples(self):
        for sequence, zip_code in (("000001", "90210"), ("000002", "10001"), ("000003", "02108")):
            raw = f"ZIP|{sequence}|{zip_code}\n".encode("ascii")
            self.assertEqual(len(raw), 17)
            self.assertEqual(parse_request(raw), (sequence, zip_code))

    def test_rejects_malformed_requests(self):
        malformed = (
            b"ZIP|000001|9021\n",
            b"ZIP|000001|ABCDE\n",
            b"ZIP|000001|90210",
            b"ZIP|000001|90210\r\n",
        )
        for raw in malformed:
            with self.assertRaises(ValueError):
                parse_request(raw)


if __name__ == "__main__":
    unittest.main()
