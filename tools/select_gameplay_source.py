#!/usr/bin/env python3
"""Select a verified, user-owned Xbox update for the native launchers.

The original executable remains external. This writes only an ignored local
path registry; the native runtime independently checks its supported image.
"""
import argparse
import hashlib
import os
from pathlib import Path
import tempfile

SUPPORTED = {
    "aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377": "Map Pack 1.1",
    "21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd": "Title Update 1.5",
}


def select(root, source):
    source = source.resolve(strict=True)
    if any(c in str(source) for c in "\t\r\n\0"):
        raise ValueError("Executable path contains a control character")
    with source.open("rb") as stream:
        pin = hashlib.file_digest(stream, "sha256").hexdigest()
    if pin not in SUPPORTED:
        raise ValueError("Unsupported update executable SHA-256: " + pin)
    registry = root.resolve() / "assets/gameplay-source.tsv"
    registry.parent.mkdir(parents=True, exist_ok=True)
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(mode="w", encoding="utf-8", dir=registry.parent,
                                         prefix=".gameplay-source-", delete=False) as stream:
            temporary = Path(stream.name)
            stream.write(f"H2_GAMEPLAY_SOURCE_V1\nxbe\t{source}\n")
        os.replace(temporary, registry)
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)
    print(f"Selected {SUPPORTED[pin]}: {source}\nSHA-256: {pin}\nRegistry: {registry}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path, help="Original update default.xbe")
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()
    try:
        select(args.root, args.source)
    except (OSError, ValueError) as error:
        parser.exit(1, f"Gameplay source selection failed: {error}\n")


if __name__ == "__main__":
    main()
