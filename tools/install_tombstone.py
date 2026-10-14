#!/usr/bin/env python3
"""Privately install the verified, user-owned revised Xbox Tombstone cache.

No game data is bundled or downloaded. The original cache name highplains2 is
retained for ONLINE capability discovery and loading; highplains is an older
revision and is never substituted.
"""
import argparse
import hashlib
import os
from pathlib import Path
import shutil
import tempfile


TOMBSTONE_SHA256 = "b14a0debdb2e279c7bd7c7cda2b3e2a5cf69ca85f457923c1cfb9de6ca0b1bb1"


def digest(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def selected_maps(root):
    lines = (root / "assets/asset-sources.tsv").read_text().splitlines()
    if not lines or lines[0] != "H2_ASSET_SOURCES_V1":
        raise ValueError("Prepare the Xbox/Vista game files first")
    sources = dict(line.split("\t") for line in lines[1:])
    return (root / sources["xbox"] / "maps").resolve(strict=True)


def install(source, maps):
    source, maps = source.resolve(strict=True), maps.resolve(strict=True)
    if digest(source) != TOMBSTONE_SHA256:
        raise ValueError("Expected the revised Xbox highplains2.map; cache hash differs")
    if not all((maps / name).is_file() for name in ("mainmenu.map", "shared.map")):
        raise ValueError("Destination must be the prepared Xbox maps directory")
    target = maps / "highplains2.map"
    if target.exists() or target.is_symlink():
        if digest(target) != TOMBSTONE_SHA256:
            raise ValueError("Destination already contains a different Tombstone cache")
        return target
    # Publish a complete checked copy atomically, without replacing existing files.
    with tempfile.TemporaryDirectory(prefix=".tombstone-", dir=maps) as temporary:
        staged = Path(temporary) / target.name
        shutil.copyfile(source, staged)
        staged.chmod(0o644)
        if digest(staged) != TOMBSTONE_SHA256:
            raise ValueError("Copied Tombstone cache failed verification")
        os.link(staged, target)
    return target


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path, help="Owned revised Xbox highplains2.map")
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--maps", type=Path, help="Explicit prepared Xbox maps directory")
    args = parser.parse_args()
    try:
        target = install(args.source, args.maps or selected_maps(args.root.resolve()))
    except (OSError, ValueError, KeyError) as error:
        parser.exit(1, f"Tombstone installation failed: {error}\n")
    print(f"Tombstone ready for ONLINE: {target}\nSHA-256: {TOMBSTONE_SHA256}")


if __name__ == "__main__":
    main()
