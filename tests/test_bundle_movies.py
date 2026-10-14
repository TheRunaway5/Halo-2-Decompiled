import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("bundle_movies", ROOT / "tools/bundle_movies.py")
movies = importlib.util.module_from_spec(spec)
spec.loader.exec_module(movies)


class ReleaseMoviesTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.source = self.root / "original Videos"
        self.source.mkdir()
        self.manifest = {"schema": "halo2-upscaled-movies-v1", "files": {}}
        for name in sorted(movies.MOVIES):
            payload = (name + " fixture").encode()
            (self.source / name).write_bytes(payload)
            self.manifest["files"][name] = {
                "bytes": len(payload), "sha256": hashlib.sha256(payload).hexdigest()}
        self.output = self.root / "release/resources/movies"

    def test_bundle_survives_removal_of_original_source(self):
        movies.bundle(self.source, self.output, self.manifest)
        self.source.rename(self.root / "moved Videos")
        movies.verify(self.output, self.manifest)
        self.assertEqual(json.loads((self.output / "manifest.json").read_text()), self.manifest)
        # A later release can use the local bundle as its source.
        destination = self.root / "next release/resources/movies"
        movies.bundle(self.output, destination, self.manifest)
        movies.verify(destination, self.manifest)

    def test_changed_input_preserves_existing_bundle(self):
        movies.bundle(self.source, self.output, self.manifest)
        previous = {p.name: p.read_bytes() for p in self.output.iterdir()}
        path = self.source / "intro_60.mp4"
        path.write_bytes(b"!" * path.stat().st_size)
        with self.assertRaisesRegex(ValueError, "checksum differs"):
            movies.bundle(self.source, self.output, self.manifest)
        self.assertEqual({p.name: p.read_bytes() for p in self.output.iterdir()}, previous)

    def test_missing_credits_stops_staging(self):
        (self.source / "credits_60.mp4").unlink()
        with self.assertRaisesRegex(ValueError, "Missing"):
            movies.bundle(self.source, self.output, self.manifest)
        self.assertFalse(self.output.exists())

    def test_bundled_movie_tampering_is_detected(self):
        movies.bundle(self.source, self.output, self.manifest)
        (self.output / "credits_60.mp4").write_bytes(b"modified")
        with self.assertRaises(ValueError):
            movies.verify(self.output, self.manifest)

    def test_manifest_cannot_introduce_other_files(self):
        self.manifest["files"]["../other.mp4"] = {}
        path = self.root / "manifest.json"
        path.write_text(json.dumps(self.manifest))
        with self.assertRaisesRegex(ValueError, "Invalid"):
            movies.read_manifest(path)


if __name__ == "__main__":
    unittest.main()
