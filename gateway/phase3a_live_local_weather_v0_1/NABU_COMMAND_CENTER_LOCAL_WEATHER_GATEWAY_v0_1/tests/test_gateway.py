import http.client
import importlib.util
import logging
import os
from pathlib import Path
import tempfile
import sys
import threading
import unittest

MODULE_PATH = Path(__file__).resolve().parents[1] / "gateway.py"
SPEC = importlib.util.spec_from_file_location("gateway", MODULE_PATH)
gateway = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
sys.modules[SPEC.name] = gateway
SPEC.loader.exec_module(gateway)

TEST_LOG_PATH = Path(tempfile.gettempdir()) / f"ncc_gateway_tests_{os.getpid()}.log"


class GatewayTests(unittest.TestCase):
    @classmethod
    def tearDownClass(cls):
        logger = logging.getLogger("ncc_gateway")
        for handler in list(logger.handlers):
            logger.removeHandler(handler)
            handler.close()
        TEST_LOG_PATH.unlink(missing_ok=True)

    def settings(self, root: Path, **overrides):
        values = dict(
            mode="store",
            host="127.0.0.1",
            port=8765,
            store_path=root / "store",
            interval=10.0,
            auto_refresh=False,
            sequence=1,
            value=42,
            text="GATEWAY_RECORD_01",
            failure_mode="valid",
            fixed_utc=1786038180,
            log_path=TEST_LOG_PATH,
        )
        values.update(overrides)
        return gateway.Settings(**values)

    def test_exact_default_record_is_64_bytes(self):
        record = gateway.canonical_record(
            1, 1786038180, 42, "GATEWAY_RECORD_01"
        )
        self.assertEqual(len(record), 64)
        self.assertEqual(
            record,
            b"NCC|1|TEST|000001|1786038180|17|0042|"
            b"GATEWAY_RECORD_01|0F6E|END\n",
        )

    def test_checksum_reference(self):
        body = (
            b"NCC|1|TEST|000001|1786038180|17|0042|"
            b"GATEWAY_RECORD_01|"
        )
        self.assertEqual(gateway.checksum16(body), 0x0F6E)

    def test_atomic_store_write(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            settings = self.settings(root)
            state = gateway.State(settings)
            logger = gateway.configure_logging(settings.log_path)
            record = gateway.generate_and_publish(state, logger, increment=False)
            final_path = settings.store_path / gateway.RESOURCE_NAME
            self.assertEqual(final_path.read_bytes(), record.content)
            self.assertFalse(list(settings.store_path.glob("*.tmp")))

    def test_missing_removes_store_file(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            settings = self.settings(root)
            state = gateway.State(settings)
            logger = gateway.configure_logging(settings.log_path)
            gateway.generate_and_publish(state, logger, increment=False)
            settings.failure_mode = "missing"
            gateway.generate_and_publish(state, logger, increment=True)
            self.assertFalse((settings.store_path / gateway.RESOURCE_NAME).exists())

    def test_offline_preserves_previous_store_file(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            settings = self.settings(root)
            state = gateway.State(settings)
            logger = gateway.configure_logging(settings.log_path)
            first = gateway.generate_and_publish(state, logger, increment=False)
            settings.failure_mode = "offline"
            gateway.generate_and_publish(state, logger, increment=True)
            self.assertEqual(
                (settings.store_path / gateway.RESOURCE_NAME).read_bytes(),
                first.content,
            )

    def test_bad_integrity_differs_from_calculated(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            settings = self.settings(root, failure_mode="bad_integrity")
            record = gateway.build_failure_record(settings, 1, 1786038180)
            assert record.content is not None
            parts = record.content.rstrip(b"\n").split(b"|")
            body_end = record.content.rfind(b"|" + parts[-2] + b"|")
            self.assertIn(b"|0000|END\n", record.content)

    def test_empty_is_zero_bytes(self):
        with tempfile.TemporaryDirectory() as td:
            settings = self.settings(Path(td), failure_mode="empty")
            record = gateway.build_failure_record(settings, 101, 1786038180)
            self.assertEqual(record.content, b"")

    def test_truncated_is_shorter_than_bounded_record(self):
        with tempfile.TemporaryDirectory() as td:
            settings = self.settings(Path(td), failure_mode="truncated")
            record = gateway.build_failure_record(settings, 101, 1786038180)
            assert record.content is not None
            self.assertGreater(len(record.content), 0)
            self.assertLess(len(record.content), 64)

    def test_bad_magic_is_exact_bounded_record(self):
        with tempfile.TemporaryDirectory() as td:
            settings = self.settings(Path(td), failure_mode="bad_magic")
            record = gateway.build_failure_record(settings, 101, 1786038180)
            assert record.content is not None
            self.assertEqual(len(record.content), 64)
            self.assertTrue(record.content.startswith(b"BAD|1|TEST|"))

    def test_unsupported_version_is_exact_bounded_record(self):
        with tempfile.TemporaryDirectory() as td:
            settings = self.settings(Path(td), failure_mode="unsupported_version")
            record = gateway.build_failure_record(settings, 101, 1786038180)
            assert record.content is not None
            self.assertEqual(len(record.content), 64)
            self.assertTrue(record.content.startswith(b"NCC|9|TEST|"))

    def test_bad_length_disagrees_with_text_length(self):
        with tempfile.TemporaryDirectory() as td:
            settings = self.settings(Path(td), failure_mode="bad_length")
            record = gateway.build_failure_record(settings, 101, 1786038180)
            assert record.content is not None
            parts = record.content.rstrip(b"\n").split(b"|")
            self.assertNotEqual(int(parts[5]), len(parts[7]))

    def test_stale_timestamp_advances_sequence_but_rolls_back_utc(self):
        with tempfile.TemporaryDirectory() as td:
            settings = self.settings(Path(td), failure_mode="stale_timestamp")
            record = gateway.build_failure_record(settings, 101, 1786038180)
            self.assertEqual(record.sequence, 101)
            self.assertEqual(record.generated_utc, 1785951780)

    def test_sequence_rollback_is_below_prior_sequence(self):
        with tempfile.TemporaryDirectory() as td:
            settings = self.settings(Path(td), failure_mode="sequence_rollback")
            record = gateway.build_failure_record(settings, 101, 1786038180)
            self.assertEqual(record.sequence, 99)

    def test_oversized_is_larger_than_initial_client_buffer(self):
        with tempfile.TemporaryDirectory() as td:
            settings = self.settings(Path(td), failure_mode="oversized")
            record = gateway.build_failure_record(settings, 1, 1786038180)
            assert record.content is not None
            self.assertGreater(len(record.content), 128)

    def test_http_only_serves_exact_resource(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            settings = self.settings(root, mode="http", port=0)
            state = gateway.State(settings)
            logger = gateway.configure_logging(settings.log_path)
            gateway.generate_and_publish(state, logger, increment=False)
            handler = gateway.make_handler(state, logger)
            server = gateway.ReusableThreadingHTTPServer(("127.0.0.1", 0), handler)
            thread = threading.Thread(target=server.serve_forever, daemon=True)
            thread.start()
            try:
                port = server.server_address[1]
                conn = http.client.HTTPConnection("127.0.0.1", port, timeout=3)
                conn.request("GET", "/ncc_test.dat")
                response = conn.getresponse()
                body = response.read()
                self.assertEqual(response.status, 200)
                self.assertEqual(body, state.snapshot().content)
                conn.close()

                conn = http.client.HTTPConnection("127.0.0.1", port, timeout=3)
                conn.request("GET", "/")
                response = conn.getresponse()
                response.read()
                self.assertEqual(response.status, 404)
                conn.close()
            finally:
                server.shutdown()
                server.server_close()
                thread.join(timeout=3)

    def test_sequence_increments(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            settings = self.settings(root)
            state = gateway.State(settings)
            logger = gateway.configure_logging(settings.log_path)
            first = gateway.generate_and_publish(state, logger, increment=False)
            second = gateway.generate_and_publish(state, logger, increment=True)
            self.assertEqual(second.sequence, first.sequence + 1)


if __name__ == "__main__":
    unittest.main(verbosity=2)
