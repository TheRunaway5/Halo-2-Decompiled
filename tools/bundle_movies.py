#!/usr/bin/env python3
"""Copy or verify the supplied upscaled release movies without transcoding."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import tempfile

ROOT = Path(__file__).resolve().parents[1]
MOVIES = {"intro_60.mp4", "credits_60.mp4"}


def read_manifest(path):
    manifest = json.loads(Path(path).read_text())
    if manifest.get("schema") != "halo2-upscaled-movies-v1" or set(manifest.get("files", {})) != MOVIES:
        raise ValueError("Invalid release movie manifest")
    return manifest


def verify_file(path, record):
    if not path.is_file() or path.stat().st_size != record["bytes"]:
        raise ValueError(f"Missing or wrong-sized release movie: {path}")
    with path.open("rb") as stream:
        digest = hashlib.file_digest(stream, "sha256").hexdigest()
    if digest != record["sha256"]:
        raise ValueError(f"Release movie checksum differs: {path}")


def verify(directory, manifest):
    for name, record in manifest["files"].items():
        verify_file(directory / name, record)


def bundle(source, output, manifest):
    source, output = Path(source).resolve(), Path(output).resolve()
    # Validate both inputs before changing any previously staged movie.
    verify(source, manifest)
    output.mkdir(parents=True, exist_ok=True)
    if source != output:
        with tempfile.TemporaryDirectory(prefix=".movies-", dir=output.parent) as temporary:
            stage = Path(temporary)
            for name in manifest["files"]:
                shutil.copyfile(source / name, stage / name)
            verify(stage, manifest)
            for name in manifest["files"]:
                (stage / name).replace(output / name)
    (output / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=ROOT / "resources/movies")
    parser.add_argument("--output", type=Path, default=ROOT / "resources/movies")
    parser.add_argument("--manifest", type=Path, default=ROOT / "resources/movie-manifest.json")
    parser.add_argument("--verify", action="store_true", help="Only verify --source")
    args = parser.parse_args()
    try:
        manifest = read_manifest(args.manifest)
        if args.verify:
            verify(args.source, manifest)
        else:
            bundle(args.source, args.output, manifest)
    except (OSError, ValueError, KeyError) as error:
        parser.exit(1, f"Release movies: {error}\n")
    print(f"Verified {len(manifest['files'])} release movies ({sum(r['bytes'] for r in manifest['files'].values())} bytes)")


if __name__ == "__main__":
    main()
