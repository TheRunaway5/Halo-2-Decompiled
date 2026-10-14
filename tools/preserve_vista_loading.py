#!/usr/bin/env python3
"""Keep the selected, measured Vista match-loading bank inside an MCC import.

MCC's map-cache loading images are different assets from Vista's dedicated
loading.bin bank. Preserve the owned Vista export explicitly; never fabricate
the missing higher-resolution source images by enlarging MCC bitmaps.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import shutil
import struct
import sys
import uuid

from audit_rendering import FormatError, file_sha
from prepare_vista_loading import SHAPES, PAYLOAD_SHA256
from export_vista_loading_audio import BANK_SHA


def selected(root):
    root = Path(root).resolve()
    registry = root / "assets/loading-source.tsv"
    if registry.exists():
        rows = registry.read_text(encoding="utf-8").splitlines()
        if len(rows) != 2 or rows[0] != "H2_LOADING_SOURCE_V1" or not rows[1].startswith("vista\t"):
            raise FormatError("Invalid selected match-loading source")
        path = rows[1].split("\t")
        if len(path) != 2 or not path[1] or any(c in path[1] for c in "\r\n\0"):
            raise FormatError("Invalid selected match-loading path")
        result = Path(path[1])
        return result if result.is_absolute() else root / result
    assets = root / "assets/asset-sources.tsv"
    if assets.is_file():
        for row in assets.read_text(encoding="utf-8").splitlines()[1:]:
            fields = row.split("\t")
            if len(fields) == 2 and fields[0] == "vista":
                path = Path(fields[1])
                result = (path if path.is_absolute() else root / path) / "loading"
                if result.is_dir():
                    return result
    legacy = root / "assets/menu-ui/vista/loading"
    if legacy.is_dir():
        return legacy
    raise FormatError("The chosen Vista match-loading export is missing; prepare the owned Vista loading.bin/halo2.exe bank first")


def validate(source):
    source = Path(source).resolve()
    files = {}
    manifest = source / "loading-textures.tsv"
    if manifest.is_symlink() or not manifest.is_file() or manifest.stat().st_size > 16384:
        raise FormatError("Missing regular Vista loading texture manifest")
    rows = manifest.read_text(encoding="utf-8").splitlines()
    if len(rows) != 13 or rows[0] != "H2V_LOADING_TEXTURES_V1":
        raise FormatError("Incomplete Vista match-loading manifest")
    seen = set()
    for row in rows[1:]:
        fields = row.split("\t")
        if len(fields) != 7:
            raise FormatError("Invalid Vista loading row")
        index, sw, sh, width, height, fmt = map(int, fields[:6])
        if index not in range(12) or index in seen:
            raise FormatError("Invalid or repeated Vista loading texture index")
        seen.add(index)
        expected_width, expected_height, bits = SHAPES[index]
        tw, th = 1 << (expected_width - 1).bit_length(), 1 << (expected_height - 1).bit_length()
        name = f"loading-{index:02d}.dds"
        if (sw, sh, width, height, fmt, fields[6]) != (
                expected_width, expected_height, tw, th, 11 if bits == 32 else 10, name):
            raise FormatError("Vista loading texture metadata differs from the measured original bank")
        path = source / name
        if path.is_symlink() or not path.is_file() or path.stat().st_size != 128 + tw * th * 4:
            raise FormatError("Missing or truncated Vista loading image")
        data = path.read_bytes()
        if data[:4] != b"DDS " or struct.unpack_from("<II", data, 12) != (th, tw) or (
                struct.unpack_from("<I", data, 4)[0] != 124 or
                struct.unpack_from("<I", data, 20)[0] != tw * 4 or
                struct.unpack_from("<I", data, 28)[0] > 1 or
                struct.unpack_from("<8I", data, 76) != (
                    32, 0x41 if bits == 32 else 0x40, 0, 32,
                    0xff0000, 0xff00, 0xff, 0xff000000 if bits == 32 else 0) or
                struct.unpack_from("<II", data, 108) != (0x1000, 0)):
            raise FormatError("Invalid original Vista loading DDS header")
        if hashlib.sha256(data[128:]).hexdigest() != PAYLOAD_SHA256[index]:
            raise FormatError("Vista loading image differs from measured original D3DX pixels")
        files[name] = dict(bytes=len(data), sha256=hashlib.sha256(data).hexdigest())
    audio = source / "loading-tone.xbox-adpcm"
    if audio.is_symlink() or not audio.is_file() or audio.stat().st_size != 18396 or file_sha(audio) != BANK_SHA:
        raise FormatError("Vista loading ambience differs from the original bank")
    for name in ("loading-tone.xbox-adpcm", "loading-textures.tsv"):
        files[name] = dict(bytes=(source / name).stat().st_size, sha256=file_sha(source / name))
    return files


def preserve(pack, source):
    pack, source = Path(pack).resolve(), Path(source).resolve()
    files = validate(source)
    destination = pack / "loading"
    if destination.exists():
        if validate(destination) != files:
            raise FormatError("Existing MCC-import loading bank differs from the chosen original")
    else:
        stage = pack / (".loading-staging-" + uuid.uuid4().hex)
        stage.mkdir()
        try:
            for name in files:
                shutil.copyfile(source / name, stage / name)
            if validate(stage) != files:
                raise FormatError("Copied Vista loading bank differs")
            stage.rename(destination)
        finally:
            if stage.exists():
                shutil.rmtree(stage)
    report = dict(schema="H2_MCC_PRESERVED_VISTA_LOADING_V1", source=str(source),
        assets="Original measured Vista match-loading export", files=files,
        shaders="Existing native loading renderer", artwork_reconstructed=False,
        complete=True)
    (destination / "source.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return report


def activate(root, directory):
    root, directory = Path(root).resolve(), Path(directory).resolve()
    validate(directory)
    value = str(directory)
    if any(c in value for c in "\t\r\n\0"):
        raise FormatError("Unsupported loading source path")
    registry = root / "assets/loading-source.tsv"
    registry.parent.mkdir(parents=True,exist_ok=True)
    temporary = registry.with_name(".loading-source-" + uuid.uuid4().hex)
    try:
        temporary.write_text("H2_LOADING_SOURCE_V1\nvista\t" + value + "\n", encoding="utf-8")
        temporary.replace(registry)
    finally:
        temporary.unlink(missing_ok=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--pack", type=Path, required=True)
    parser.add_argument("--source", type=Path)
    parser.add_argument("--activate", action="store_true")
    args = parser.parse_args()
    try:
        report = preserve(args.pack, args.source if args.source else selected(args.root))
        if args.activate:
            activate(args.root, args.pack / "loading")
        print(f"Preserved all twelve original Vista loading images and ambience: {args.pack / 'loading'}")
        return 0
    except (OSError, ValueError) as error:
        print(f"Vista match-loading preservation failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
