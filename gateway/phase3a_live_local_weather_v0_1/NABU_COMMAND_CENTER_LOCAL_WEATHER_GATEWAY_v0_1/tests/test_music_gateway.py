import tempfile
import unittest
from pathlib import Path
from unittest.mock import Mock

import mido

import music_gateway as mg


def fixture_midi() -> bytes:
    with tempfile.TemporaryDirectory() as folder:
        path = Path(folder) / "fixture.mid"
        midi = mido.MidiFile(type=1, ticks_per_beat=480)
        track = mido.MidiTrack(); midi.tracks.append(track)
        track.append(mido.Message("note_on", note=60, velocity=80, time=0))
        track.append(mido.Message("note_on", note=67, velocity=72, time=0))
        track.append(mido.Message("note_off", note=60, velocity=0, time=480))
        track.append(mido.Message("note_off", note=67, velocity=0, time=0))
        midi.save(path)
        return path.read_bytes()


def timing_fixture_midi() -> bytes:
    with tempfile.TemporaryDirectory() as folder:
        path = Path(folder) / "timing.mid"
        midi = mido.MidiFile(type=1, ticks_per_beat=480)
        track = mido.MidiTrack(); midi.tracks.append(track)
        track.extend([
            mido.MetaMessage("set_tempo", tempo=500_000, time=0),
            mido.Message("note_on", note=60, velocity=80, channel=0, time=0),
            mido.Message("note_off", note=60, velocity=0, channel=0, time=480),
            mido.Message("note_on", note=64, velocity=80, channel=0, time=240),
            mido.Message("note_on", note=67, velocity=70, channel=1, time=0),
            mido.Message("note_on", note=72, velocity=100, channel=2, time=240),
            mido.Message("note_on", note=64, velocity=0, channel=0, time=240),
            mido.Message("note_off", note=72, velocity=0, channel=2, time=240),
            mido.Message("note_off", note=67, velocity=0, channel=1, time=240),
            mido.MetaMessage("set_tempo", tempo=1_000_000, time=0),
            mido.Message("note_on", note=55, velocity=64, channel=0, time=480),
            mido.Message("note_off", note=55, velocity=0, channel=0, time=480),
        ])
        midi.save(path)
        return path.read_bytes()


class MusicGatewayTests(unittest.TestCase):
    def test_valid_deterministic_conversion_and_bounds(self):
        body = fixture_midi()
        first = mg.convert_midi(body)
        self.assertEqual(first, mg.convert_midi(body))
        self.assertTrue(mg.validate_record(first))
        self.assertEqual(len(first), 512)

    def test_controlled_timing_tempo_voice_and_release_timeline(self):
        events, stats = mg.conversion_events(timing_fixture_midi())
        self.assertEqual(events, [
            (30, 60, 0, 0xA0),
            (15, 0, 0, 0x00),
            (15, 64, 67, 0xA8),
            (15, 72, 64, 0xCA),
            (15, 72, 67, 0xC8),
            (15, 67, 0, 0x80),
            (60, 0, 0, 0x00),
            (60, 55, 0, 0x80),
            (1, 0, 0, 0x00),
        ])
        self.assertEqual(stats["ticks_per_beat"], 480)
        self.assertEqual(stats["duration_frames"], 226)
        self.assertAlmostEqual(stats["duration_seconds"], 226 / 60)
        self.assertTrue(stats["final_silent"])

    def test_pitch_matches_nabutracker_oracle(self):
        expected = {36: 3420, 48: 1710, 60: 855, 69: 508, 72: 427, 84: 213, 96: 106}
        self.assertEqual({note: mg.ay_period(note) for note in expected}, expected)

    def test_malformed_and_unsupported_are_rejected(self):
        with self.assertRaises(ValueError): mg.convert_midi(b"not midi")
        body = bytearray(fixture_midi()); body[9] = 2
        with self.assertRaises(Exception): mg.convert_midi(bytes(body))

    def test_cache_reuse_and_failed_source_preserves_cache(self):
        body = fixture_midi()
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder); store = root / "store"; cache = root / "cache"
            acquire = Mock(return_value=body)
            first = mg.publish(store, cache, acquire=acquire)
            saved = (store / mg.MUSIC_NAME).read_bytes()
            cached = (cache / f"{mg.CACHE_STEM}.mu01").read_bytes()
            self.assertEqual(first["license"], "Public Domain")
            self.assertTrue((cache / f"{mg.CACHE_STEM}.json").is_file())
            second = mg.publish(store, cache, acquire=Mock(side_effect=OSError("offline")))
            self.assertEqual(second["acquisition_status"], "CACHE")
            self.assertEqual((cache / f"{mg.CACHE_STEM}.mu01").read_bytes(), cached)
            self.assertTrue(mg.validate_record((store / mg.MUSIC_NAME).read_bytes()))
            self.assertNotEqual((store / mg.MUSIC_NAME).read_bytes()[7], saved[7])

    def test_invalid_partial_cannot_replace_valid_cache(self):
        body = fixture_midi()
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder); store = root / "store"; cache = root / "cache"
            mg.publish(store, cache, acquire=Mock(return_value=body))
            saved = (store / mg.MUSIC_NAME).read_bytes()
            (cache / f"{mg.CACHE_STEM}.mu01").unlink()
            (cache / f"{mg.CACHE_STEM}.mid").unlink()
            with self.assertRaises(ValueError):
                mg.publish(store, cache, acquire=Mock(return_value=b"MThd"))
            self.assertEqual((store / mg.MUSIC_NAME).read_bytes(), saved)

    def test_old_converter_cache_is_rebuilt_from_cached_midi(self):
        body = fixture_midi()
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder); store = root / "store"; cache = root / "cache"
            mg.publish(store, cache, acquire=Mock(return_value=body))
            metadata = cache / f"{mg.CACHE_STEM}.json"
            text = metadata.read_text(encoding="utf-8").replace(mg.CONVERTER_VERSION, "MU01-LV1")
            metadata.write_text(text, encoding="utf-8")
            acquire = Mock(side_effect=OSError("must not download"))
            result = mg.publish(store, cache, acquire=acquire)
            acquire.assert_not_called()
            self.assertEqual(result["converter_version"], mg.CONVERTER_VERSION)
            self.assertTrue(mg.validate_record((store / mg.MUSIC_NAME).read_bytes()))


if __name__ == "__main__": unittest.main()
