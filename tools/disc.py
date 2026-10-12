#!/usr/bin/env python3
"""Read a local Xbox XDVDFS disc and inspect XBE metadata; no game code is executed."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct

SECTOR = 2048
MAGIC = b"MICROSOFT*XBOX*MEDIA"


class FormatError(ValueError):
    pass


def bounds(offset: int, length: int, size: int) -> None:
    if offset < 0 or length < 0 or offset > size or length > size - offset:
        raise FormatError(f"range {offset:#x}+{length:#x} outside {size:#x} bytes")


class Disc:
    def __init__(self, path: Path):
        self.path = path
        self.size = path.stat().st_size
        self.file = path.open("rb")
        try:
            for base in (0, 0xFD90000, 0x2080000):
                if base + 33 * SECTOR > self.size:
                    continue
                header = self.read(base + 32 * SECTOR, SECTOR)
                if header[:20] == MAGIC and header[-20:] == MAGIC:
                    self.base = base
                    self.root_sector, self.root_size = struct.unpack_from("<II", header, 20)
                    bounds(base + self.root_sector * SECTOR, self.root_size, self.size)
                    break
            else:
                raise FormatError("no XDVDFS volume descriptor at a supported partition offset")
        except BaseException:
            self.file.close()
            raise

    def __enter__(self):
        return self

    def __exit__(self, *_):
        self.file.close()

    def read(self, offset: int, length: int) -> bytes:
        bounds(offset, length, self.size)
        self.file.seek(offset)
        data = self.file.read(length)
        if len(data) != length:
            raise FormatError("short disc read")
        return data

    def entries(self) -> list[dict]:
        entries: list[dict] = []
        directories: set[tuple[int, int]] = set()
        names: set[str] = set()

        def directory(sector: int, size: int, parent: str, depth: int):
            if depth > 64 or size > 16 * 1024 * 1024:
                raise FormatError("directory nesting or size limit exceeded")
            if not size:
                return
            key = (sector, size)
            if key in directories:
                raise FormatError("directory cycle or alias")
            directories.add(key)
            data = self.read(self.base + sector * SECTOR, size)
            seen: set[int] = set()
            stack = [0]
            while stack:
                offset = stack.pop()
                if offset in seen:
                    raise FormatError("directory tree cycle")
                seen.add(offset)
                bounds(offset, 14, size)
                left, right, first, length, flags, name_size = struct.unpack_from("<HHIIBB", data, offset)
                bounds(offset + 14, name_size, size)
                raw_name = data[offset + 14:offset + 14 + name_size]
                name = raw_name.decode("ascii", errors="strict")
                if not name or name in (".", "..") or any(c in name for c in "/\\\x00:"):
                    raise FormatError("unsafe disc filename")
                path = f"{parent}/{name}" if parent else name
                if path.casefold() in names:
                    raise FormatError("duplicate disc path")
                names.add(path.casefold())
                bounds(self.base + first * SECTOR, length, self.size)
                if flags & 0x10:
                    directory(first, length, path, depth + 1)
                else:
                    entries.append({"path": path, "sector": first, "size": length,
                                    "offset": self.base + first * SECTOR})
                for child in (right, left):
                    if child:
                        stack.append(child * 4)
        directory(self.root_sector, self.root_size, "", 0)
        return sorted(entries, key=lambda item: item["path"].casefold())

    def extract(self, target: Path) -> dict:
        target.mkdir(parents=True, exist_ok=True)
        target = target.resolve()
        entries = self.entries()
        if (target / "disc-manifest.json").exists():
            raise FileExistsError(f"{target}/disc-manifest.json already exists; use a new output directory")
        # Validate every destination before writing the first file.
        for item in entries:
            dest = target / item["path"]
            if not dest.resolve().is_relative_to(target) or dest.exists() or dest.is_symlink():
                raise FormatError(f"unsafe or existing extraction destination: {dest}")
            if any(p.is_symlink() for p in dest.parents if p != target and p.is_relative_to(target)):
                raise FormatError("extraction destination contains a symlink")
        for item in entries:
            dest = target / item["path"]
            dest.parent.mkdir(parents=True, exist_ok=True)
            digest = hashlib.sha256()
            self.file.seek(item["offset"])
            remaining = item["size"]
            # Exclusive creation avoids silently replacing an existing asset.
            with dest.open("xb") as out:
                while remaining:
                    chunk = self.file.read(min(1024 * 1024, remaining))
                    if not chunk:
                        raise FormatError("short read while extracting")
                    out.write(chunk)
                    digest.update(chunk)
                    remaining -= len(chunk)
            item["sha256"] = digest.hexdigest()
            print(f"{item['path']} ({item['size']:,} bytes)")
        self.file.seek(0)
        iso_digest = hashlib.file_digest(self.file, "sha256").hexdigest()
        manifest = {"format": "XDVDFS", "source": self.path.name, "source_size": self.size,
                    "source_sha256": iso_digest, "partition_offset": self.base, "files": entries}
        (target / "disc-manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
        return manifest


def inspect_xbe(path: Path) -> dict:
    data = path.read_bytes()
    bounds(0, 0x178, len(data))
    if data[:4] != b"XBEH":
        raise FormatError("not an Xbox executable")

    def u32(offset):
        bounds(offset, 4, len(data))
        return struct.unpack_from("<I", data, offset)[0]

    base = u32(0x104)
    sections = []
    count, table = u32(0x11C), u32(0x120) - base
    if count > 4096:
        raise FormatError("excessive XBE section count")
    bounds(table, count * 56, len(data))
    for i in range(count):
        offset = table + i * 56
        flags, va, virtual_size, raw, raw_size, name_va = struct.unpack_from("<6I", data, offset)
        bounds(raw, raw_size, len(data))
        name_offset = name_va - base
        bounds(name_offset, 1, len(data))
        end = data.find(b"\0", name_offset, min(name_offset + 256, len(data)))
        if end < 0:
            raise FormatError("unterminated XBE section name")
        sections.append({"name": data[name_offset:end].decode("ascii"), "flags": flags,
                         "address": va, "virtual_size": virtual_size,
                         "file_offset": raw, "file_size": raw_size,
                         "executable": bool(flags & 4),
                         "sha256": hashlib.sha256(data[raw:raw + raw_size]).hexdigest()})
    entry = None
    kind = None
    for candidate, ep_key in (("retail", 0xA8FC57AB), ("debug", 0x94859D4B)):
        address = u32(0x128) ^ ep_key
        if any(s["executable"] and s["address"] <= address < s["address"] + s["virtual_size"] for s in sections):
            entry, kind = address, candidate
            break
    if entry is None:
        raise FormatError("XBE entry point is outside executable sections")
    cert = u32(0x118) - base
    bounds(cert, 0x5C, len(data))
    title = data[cert + 12:cert + 92].decode("utf-16-le").split("\0", 1)[0]
    libraries = []
    lib_count, lib_table = u32(0x160), u32(0x164) - base
    if lib_count > 4096:
        raise FormatError("excessive XBE library count")
    bounds(lib_table, lib_count * 16, len(data))
    for i in range(lib_count):
        off = lib_table + i * 16
        name = data[off:off + 8].split(b"\0", 1)[0].decode("ascii")
        major, minor, build, flags = struct.unpack_from("<4H", data, off + 8)
        libraries.append({"name": name, "version": f"{major}.{minor}.{build}", "flags": flags})
    return {"path": path.name, "sha256": hashlib.sha256(data).hexdigest(), "title": title,
            "title_id": f"{u32(cert + 8):08X}", "kind": kind, "base_address": base,
            "entry_point": entry, "image_size": u32(0x10C), "sections": sections,
            "libraries": libraries}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    ls = sub.add_parser("list")
    ls.add_argument("iso", type=Path)
    extract = sub.add_parser("extract")
    extract.add_argument("iso", type=Path)
    extract.add_argument("--output", type=Path, default=Path("assets/disc"))
    xbe = sub.add_parser("xbe")
    xbe.add_argument("file", type=Path)
    xbe.add_argument("--output", type=Path)
    args = parser.parse_args()
    try:
        if args.command == "xbe":
            result = json.dumps(inspect_xbe(args.file), indent=2) + "\n"
            if args.output:
                args.output.parent.mkdir(parents=True, exist_ok=True)
                args.output.write_text(result)
            else:
                print(result, end="")
        else:
            with Disc(args.iso) as disc:
                if args.command == "list":
                    for item in disc.entries():
                        print(f"{item['size']:>12}  {item['path']}")
                else:
                    disc.extract(args.output)
    except (OSError, ValueError, UnicodeError) as error:
        parser.exit(1, f"error: {error}\n")


if __name__ == "__main__":
    main()
