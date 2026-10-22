"""Bounded readers and installation discovery for user-owned MCC Classic data.

Format authority: retained Reclaimer Halo2 CacheFile.config.cs,
MccCompressedCacheStream.cs, HeaderAddressTranslator.cs, DataPointer.cs and
Assembly fd14ef3c46135e9422966c6a64cd926fb0bd23fb Halo2MCC overrides.
Raw v13 chunks are verified against the installed MCC chunk-table extents;
the retained Reclaimer reader's two-byte raw-header skip does not apply.
This reader does not interpret MCC shaders or pretend v13 metadata is Xbox v8.
"""
from __future__ import annotations

import os
from pathlib import Path
import re
import struct
import zlib

from audit_rendering import FormatError

HEADER_SIZE = 896
MAX_CACHE = 1024 * 1024 * 1024
MAX_RESOURCE = 256 * 1024 * 1024


def directory(path):
    path = Path(path).expanduser().resolve()
    for candidate in (path, path / "halo2", path.parent):
        maps = candidate / "h2_maps_win64_dx11"
        if all((maps / n).is_file() for n in ("textures.dat", "shared.map", "mainmenu.map")) and (
                candidate / "h2_fonts/font_table.txt").is_file():
            return candidate
    raise FormatError("Select the MCC installation, halo2, or h2_maps_win64_dx11 directory")


def discover(*, home=None, environment=None, extra_libraries=()):
    """Search Steam's configured libraries, including mounted external libraries.

    No whole-disk crawl, Proton prefix recursion, network drive scan or registry
    mutation. An explicit source path always takes precedence in the caller.
    """
    home = Path.home() if home is None else Path(home)
    env = os.environ if environment is None else environment
    direct = [Path(env[k]) for k in ("HALO2_MCC_ROOT",) if env.get(k)]
    libraries = [home / ".local/share/Steam", home / ".steam/steam",
                 home / ".steam/root", home / ".var/app/com.valvesoftware.Steam/.local/share/Steam",
                 home / "Library/Application Support/Steam", *map(Path, extra_libraries)]
    for key in ("PROGRAMFILES(X86)", "ProgramFiles(x86)", "PROGRAMFILES", "ProgramFiles", "LOCALAPPDATA"):
        if env.get(key):
            libraries.append(Path(env[key]) / "Steam")
    if os.name == "nt":
        import winreg
        for hive, key, value in ((winreg.HKEY_CURRENT_USER, r"Software\Valve\Steam", "SteamPath"),
                                 (winreg.HKEY_LOCAL_MACHINE, r"SOFTWARE\WOW6432Node\Valve\Steam", "InstallPath")):
            try:
                with winreg.OpenKey(hive, key) as registry:
                    libraries.append(Path(winreg.QueryValueEx(registry, value)[0]))
            except OSError:
                pass
        for drive in "CDEFGHIJKLMNOPQRSTUVWXYZ":
            direct.extend((Path(drive + ":/XboxGames/Halo The Master Chief Collection/Content"),
                           Path(drive + ":/XboxGames/Halo The Master Chief Collection")))
    visited = set()
    for library in libraries:
        canonical = library.expanduser().resolve()
        if canonical in visited:
            continue
        visited.add(canonical)
        for config in (canonical / "steamapps/libraryfolders.vdf", canonical / "config/libraryfolders.vdf"):
            if config.is_file() and config.stat().st_size <= 1024 * 1024:
                contents = config.read_text(encoding="utf-8", errors="replace")
                # Current path keys and legacy numbered library entries.
                paths = re.findall(r'"(?:path|[0-9]+)"\s+"([^"\r\n]+)"', contents)
                libraries.extend(Path(p.replace("\\\\", "\\")) for p in paths)
        # The manifest supplies nonstandard installation directory names.
        manifest = canonical / "steamapps/appmanifest_976730.acf"
        names = ["Halo The Master Chief Collection"]
        if manifest.is_file() and manifest.stat().st_size <= 1024 * 1024:
            match = re.search(r'"installdir"\s+"([^"\r\n]+)"', manifest.read_text(encoding="utf-8", errors="replace"))
            if match and Path(match[1]).name == match[1] and match[1] not in (".", "..") and "\\" not in match[1]:
                names.insert(0, match[1])
        direct.extend(canonical / "steamapps/common" / name for name in names)
    for candidate in direct:
        try:
            return directory(candidate)
        except (FormatError, OSError):
            pass
    raise FormatError("MCC Halo 2 Classic was not found in installed game libraries; pass --mcc-root PATH")


def inflate(block, limit):
    if limit <= 0:
        raise FormatError("MCC decompression exceeds its declared extent")
    try:
        decoder = zlib.decompressobj()
        result = decoder.decompress(block, limit + 1)
        if len(result) > limit or not decoder.eof or decoder.unused_data or decoder.unconsumed_tail:
            raise FormatError("Invalid, excessive or trailing MCC zlib data")
        return result
    except zlib.error as error:
        raise FormatError(f"Invalid MCC zlib stream: {error}") from error


def require(data, at, size):
    if at < 0 or size < 0 or at > len(data) or size > len(data) - at:
        raise FormatError("MCC access outside file extent")


def decode_cache(raw):
    require(raw, 0, HEADER_SIZE)
    if struct.unpack_from("<II", raw) != (0x68656164, 13):
        raise FormatError("Expected MCC Halo 2 Classic version-13 cache")
    extent = struct.unpack_from("<I", raw, 8)[0]
    if not HEADER_SIZE < extent <= MAX_CACHE:
        raise FormatError("Invalid or excessive decoded MCC cache size")
    flags = struct.unpack_from("<H", raw, 28)[0]
    if not flags & 1:
        if len(raw) != extent:
            raise FormatError("Truncated uncompressed MCC cache")
        return bytearray(raw)
    chunk_size, _, table, count = struct.unpack_from("<4I", raw, 776)
    if not 0 < chunk_size <= MAX_CACHE or not 0 < count <= 65536:
        raise FormatError("Invalid MCC compressed chunk count/size")
    require(raw, table, count * 8)
    decoded = bytearray(raw[:HEADER_SIZE])
    for i in range(count):
        size, address = struct.unpack_from("<ii", raw, table + i * 8)
        if address < HEADER_SIZE or not size:
            raise FormatError("Invalid MCC compressed chunk location")
        remaining = min(chunk_size, extent - len(decoded))
        if size < 0:
            require(raw, address, -size)
            block = raw[address:address - size]
            if not 0 < len(block) <= remaining:
                raise FormatError("Excessive MCC uncompressed chunk")
        else:
            require(raw, address, size)
            block = inflate(raw[address:address + size], remaining)
        decoded.extend(block)
    if len(decoded) != extent:
        raise FormatError("MCC decoded size differs from cache header")
    return decoded


class MccCache:
    bitmap_stride = 168

    def __init__(self, path):
        self.path = Path(path)
        if self.path.is_symlink() or not self.path.is_file() or self.path.stat().st_size > MAX_CACHE:
            raise FormatError(f"Missing regular or excessive MCC cache: {self.path}")
        self.data = decode_cache(self.path.read_bytes())
        self.index = self.u32(16)
        self.require(self.index, 32)
        self.magic = self.u32(720) - self.index
        header_magic = self.u32(self.index) - self.index - 32
        count = self.u32(self.index + 24)
        if not 0 < count <= 65536 or count != self.u32(32):
            raise FormatError("MCC directory count differs from header")
        table = self.pointer(self.u32(self.index + 8), header_magic, count * 16)
        names, size, offsets = self.u32(36), self.u32(40), self.u32(44)
        self.require(names, size)
        self.require(offsets, count * 4)
        self.tags = {}
        for i in range(count):
            at = table + i * 16
            group, datum, address, length = struct.unpack_from("<4I", self.data, at)
            if datum == 0xffffffff:
                continue
            if datum & 65535 != i:
                raise FormatError("MCC salted datum disagrees with directory slot")
            name = self.u32(offsets + i * 4)
            if name >= size:
                raise FormatError("MCC filename outside name table")
            try:
                group_name = group.to_bytes(4, "big").decode("ascii")
            except UnicodeError as error:
                raise FormatError("Invalid MCC tag group") from error
            offset = self.pointer(address, self.magic, length) if address else None
            self.tags[datum] = dict(datum=datum, group=group_name,
                name=self.string(names + name, size - name), address=address, size=length, offset=offset)

    def require(self, at, size):
        require(self.data, at, size)

    def u32(self, at):
        self.require(at, 4)
        return struct.unpack_from("<I", self.data, at)[0]

    def u16(self, at):
        self.require(at, 2)
        return struct.unpack_from("<H", self.data, at)[0]

    def string(self, at, size):
        self.require(at, size)
        end = self.data.find(b"\0", at, at + size)
        if end < 0:
            raise FormatError("Unterminated MCC string")
        try:
            return self.data[at:end].decode("utf-8")
        except UnicodeError as error:
            raise FormatError("Invalid MCC string encoding") from error

    def pointer(self, address, magic, size):
        at = address - magic
        self.require(at, size)
        return at

    def block(self, at, stride):
        count, pointer = self.u32(at), self.u32(at + 4)
        if count > 65536 or stride <= 0:
            raise FormatError("Excessive MCC block")
        if not count:
            return []
        base = self.pointer(pointer, self.magic, count * stride)
        return [base + i * stride for i in range(count)]

    def tag(self, datum, group):
        tag = self.tags.get(datum)
        if not tag or tag["group"] != group or tag["offset"] is None:
            raise FormatError(f"Unresolved MCC {group} tag {datum:08x}")
        return tag, tag["offset"]

    def close(self):
        self.data.clear()


def texture_resource(stream, pointer, stored_size, *, decoded_limit=MAX_RESOURCE):
    """Decode any bitmap partition without resampling or assuming pixel layout."""
    address = pointer & 0x3fffffff
    limit = min(MAX_RESOURCE, decoded_limit)
    if limit <= 0:
        raise FormatError("MCC decoded bitmap budget exhausted")
    stream.seek(0, 2)
    extent = stream.tell()
    if stored_size < 8 or stored_size > MAX_RESOURCE or address + stored_size > extent:
        raise FormatError("MCC texture partition outside textures.dat")
    stream.seek(address)
    data = stream.read(stored_size)
    if len(data) != stored_size:
        raise FormatError("Short MCC texture partition read")
    count = struct.unpack_from("<i", data)[0]
    if not 0 < count <= 4096:
        raise FormatError("Invalid MCC texture segment count")
    require(data, 4, count * 4)
    segments = struct.unpack_from(f"<{count}i", data, 4)
    at = 4 + count * 4
    if any(s == 0 for s in segments) or at + sum(abs(s) for s in segments) != stored_size:
        raise FormatError("MCC texture segment sizes differ from partition extent")
    result = bytearray()
    for size in segments:
        block = data[at:at + abs(size)]
        remaining = limit - len(result)
        if size < 0 and len(block) > remaining:
            raise FormatError("Excessive decoded MCC texture partition")
        result.extend(block if size < 0 else inflate(block, remaining))
        at += abs(size)
    return bytes(result)
