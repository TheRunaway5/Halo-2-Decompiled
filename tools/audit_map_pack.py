#!/usr/bin/env python3
"""Verify the extracted local Map Pack and inventory its original update payloads.

This is a data audit. It neither installs Xbox executables nor changes native
gameplay, and does not interpret an update as the final-ever Halo 2 release.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct
import zlib

from disc import FormatError, bounds, inspect_xbe


def sha256(path: Path) -> str:
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def level_catalog(path: Path) -> list[dict]:
    """Decode fields copied by original 19BD00 / update 19F2E0.

    Record sizes and CRC semantics are checked against the untouched Xbox
    validators by tools/oracle_map_pack_levels.py. These records describe
    map-menu entries and their previews; they do not contain weapon tuning.
    """
    data = path.read_bytes()
    bounds(0, 8, len(data))
    version = struct.unpack_from("<I", data)[0]
    if version == 2:
        count, start, size, extra = 1, 0, 0x2D3FC, 0
    elif version == 3:
        count, start, size, extra = struct.unpack_from("<I", data, 4)[0], 8, 0x2D400, 4
    else:
        raise FormatError("unsupported original level-catalog version")
    if not count or count > 64 or len(data) != start + count * size:
        raise FormatError("invalid level-catalog count/length")
    result = []
    for index in range(count):
        offset = start + index * size
        record = bytearray(data[offset:offset + size])
        record_version, checksum, kind = struct.unpack_from("<III", record)
        if record_version != version or kind != 1:
            raise FormatError("invalid original multiplayer catalog record")
        struct.pack_into("<I", record, 4, 0)
        if (zlib.crc32(record) ^ 0xFFFFFFFF) != checksum:
            raise FormatError("original level-catalog CRC mismatch")
        struct.pack_into("<I", record, 4, checksum)
        def wide(at: int, length: int) -> str:
            text = record[at:at + length].decode("utf-16-le")
            if "\0" not in text:
                raise FormatError("unterminated original catalog text")
            return text.split("\0", 1)[0]
        scenario = bytes(record[12:12 + 256]).split(b"\0", 1)[0].decode("ascii")
        preview = record[0xC6C + extra:]
        if len(preview) != 220 * 207 * 4:
            raise FormatError("invalid original catalog preview size")
        result.append(dict(index=index, offset=offset, version=version, checksum=f"{checksum:08x}",
                           scenario=scenario, map_id=struct.unpack_from("<I", record, 0x10C + extra)[0],
                           titles_by_slot=[wide(0x114 + extra + slot * 64, 64) for slot in range(9)],
                           descriptions_by_slot=[wide(0x354 + extra + slot * 256, 256) for slot in range(9)],
                           preview=dict(offset=offset + 0xC6C + extra, width=220, height=207,
                                        bytes=len(preview), sha256=hashlib.sha256(preview).hexdigest())))
    return result


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pack", type=Path, default=Path("assets/multiplayer-map-pack"))
    parser.add_argument("--base", type=Path, default=Path("assets/disc"))
    parser.add_argument("--output", type=Path, default=Path("analysis/multiplayer/map-pack-audit.json"))
    args = parser.parse_args()
    root = args.pack.resolve()
    manifest = json.loads((root / "disc-manifest.json").read_text())
    for entry in manifest["files"]:
        path = root / entry["path"]
        if not path.resolve().is_relative_to(root):
            raise FormatError("manifest path escapes extracted disc")
        if path.stat().st_size != entry["size"] or sha256(path) != entry["sha256"]:
            raise FormatError("extracted file differs from disc manifest: " + entry["path"])
    base_manifest = json.loads((args.base / "disc-manifest.json").read_text())
    base_executable = next(item for item in base_manifest["files"] if item["path"] == "default.xbe")
    if sha256(args.base / "default.xbe") != base_executable["sha256"]:
        raise FormatError("base executable differs from original disc manifest")

    updates = []
    for path in sorted((root / "demos/update/Updates").glob("default.xbe.*")):
        info = inspect_xbe(path)
        data = path.read_bytes()
        base = struct.unpack_from("<I", data, 0x104)[0]
        certificate = struct.unpack_from("<I", data, 0x118)[0] - base
        bounds(certificate, 0xB0, len(data))
        version = struct.unpack_from("<I", data, certificate + 0xAC)[0]
        info.update(path=str(path.relative_to(root)), certificate_version=f"0x{version:08x}")
        updates.append(info)
    if len(updates) != 8 or any(info["title_id"] != "4D530064" for info in updates):
        raise FormatError("expected the eight original Halo 2 update executables")
    section_sets = {tuple((s["name"], s["address"], s["file_size"], s["sha256"])
                         for s in update["sections"]) for update in updates}

    maps = []
    for path in sorted((root / "demos/dlc").glob("*/*.map")):
        with path.open("rb") as stream:
            header = stream.read(2048)
        if len(header) != 2048 or struct.unpack_from("<II", header) != (0x68656164, 8):
            raise FormatError("expected original Xbox version-8 map: " + str(path))
        if struct.unpack_from("<I", header, 8)[0] != path.stat().st_size or \
                struct.unpack_from("<I", header, 2044)[0] != 0x666F6F74:
            raise FormatError("invalid map length/footer: " + str(path))
        def text(offset: int, size: int) -> str:
            field = header[offset:offset + size]
            if b"\0" not in field:
                raise FormatError("unterminated map header field")
            return field.split(b"\0", 1)[0].decode("ascii")
        maps.append(dict(path=str(path.relative_to(root)), size=path.stat().st_size,
                         sha256=sha256(path), name=text(408, 32), build=text(288, 32),
                         scenario=text(444, 256)))
    if len(maps) != 9:
        raise FormatError("expected the nine original Map Pack maps")

    patches = [dict(path=str(path.relative_to(root)), size=path.stat().st_size,
                    sha256=sha256(path), role="Original map-menu catalog and preview records",
                    records=level_catalog(path))
               for path in sorted((root / "demos/dlc").glob("*/*.lvl"))]
    confirmation_path = root / "demos/update/Updates/Confirmation.utf.01"
    confirmation = confirmation_path.read_bytes().decode("utf-16")
    if "'1.1'" not in confirmation:
        raise FormatError("disc installer no longer identifies update 1.1")
    result = dict(source_iso=manifest["source"], source_iso_sha256=manifest["source_sha256"],
                  verified_files=len(manifest["files"]),
                  verified_bytes=sum(entry["size"] for entry in manifest["files"]),
                  base_executable_unchanged=True, base_executable_sha256=base_executable["sha256"],
                  update_version="1.1", version_authority=str(confirmation_path.relative_to(root)),
                  installer_confirmation=confirmation, update_sections_identical=len(section_sets) == 1,
                  gameplay_update_authority="demos/update/Updates/default.xbe.03; base certificate variant 03, identical update code across all eight variants",
                  updates=updates, maps=maps, level_payloads=patches,
                  limits=["Update 1.1 is identified by the shipped installer; final-ever balance is not established.",
                          "No Xbox code executed, update installed, or native gameplay tuning changed.",
                          "Map header validation does not establish complete native DLC-map support."])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + "\n")
    print(f"Verified {result['verified_files']} extracted files ({result['verified_bytes']:,} bytes), "
          f"{len(maps)} maps and {len(updates)} update executables; installer version 1.1.")
    print("Original base executable unchanged; all update sections identical:", result["update_sections_identical"])
    print(args.output)


if __name__ == "__main__":
    main()
