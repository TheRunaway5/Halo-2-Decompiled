"""Exercise playback-cache staging with Ninja's precreated output directories."""
import json
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import prepare_movie_playback as playback


class PlaybackStagingTests(unittest.TestCase):
    def setUp(self):
        self.owner = tempfile.TemporaryDirectory(prefix='movie-stage-', dir=os.environ.get('HALO2_TEST_SCRATCH'))
        self.addCleanup(self.owner.cleanup)
        self.root = Path(self.owner.name)
        resources = self.root / 'resources'
        resources.mkdir()
        self.source = self.root / 'original.mp4'
        self.source.write_bytes(b'original-frame-owner fixture')
        expected = {'bytes': self.source.stat().st_size, 'sha256': playback.sha(self.source)}
        (resources / 'movie-manifest.json').write_text(json.dumps({'files': {'intro_60.mp4': expected}}))
        self.bank = self.root / 'verified bank'
        self.bank.mkdir()
        (self.bank / playback.VIDEO).write_bytes(b'pixel-identical-video fixture; no decoder runs')
        row = {'schema': playback.SCHEMA, 'role': 'intro', 'file': playback.VIDEO,
               'source': expected, 'all_yuv_frames_identical': True, 'copied_audio_packets': True,
               'pixel_format': 'yuv420p', 'frame_rate': '60000/1001',
               'bytes': (self.bank / playback.VIDEO).stat().st_size,
               'sha256': playback.sha(self.bank / playback.VIDEO)}
        (self.bank / 'manifest.json').write_text(json.dumps(row))
        self.output = self.root / 'build/movie-playback'
        self.patch = patch.object(playback, 'ROOT', self.root)
        self.patch.start()
        self.addCleanup(self.patch.stop)

    def test_ninja_created_empty_directory_is_filled_and_verified(self):
        self.output.mkdir(parents=True)
        playback.stage(self.bank, self.output, self.source)
        self.assertEqual(playback.verify(self.output, self.source), playback.verify(self.bank, self.source))
        self.assertEqual((self.output / playback.VIDEO).read_bytes(), (self.bank / playback.VIDEO).read_bytes())
        before = {p.name: p.stat().st_mtime_ns for p in self.output.iterdir()}
        playback.stage(self.bank, self.output, self.source)
        self.assertEqual(before, {p.name: p.stat().st_mtime_ns for p in self.output.iterdir()})
        self.assertFalse(list(self.output.parent.glob('halo2-playback-stage-*')))

    def test_nonempty_partial_bank_is_preserved(self):
        self.output.mkdir(parents=True)
        (self.output / 'existing-asset').write_bytes(b'preserve this owner')
        with self.assertRaises(ValueError):
            playback.stage(self.bank, self.output, self.source)
        self.assertEqual(list(p.name for p in self.output.iterdir()), ['existing-asset'])
        self.assertEqual((self.output / 'existing-asset').read_bytes(), b'preserve this owner')


if __name__ == '__main__':
    unittest.main()
