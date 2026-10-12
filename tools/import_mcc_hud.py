#!/usr/bin/env python3
"""Import original MCC Halo 2 Classic HUD resources and fonts, read only.

The v13 cache/texture readers follow the locally retained Reclaimer reader and
Assembly Halo2MCC schemas. No reconstruction, resampling or image generation.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import tempfile
import uuid
import zlib

from audit_rendering import Cache, FormatError, file_sha
from export_vista_menu_ui import dds_image, mip_shapes, safe_asset_path, sequences
from inspect_pc_combat_tag_sample import PcCache
from setup_assets import banks, effective_sequences, export_hud_alignment

ROOT = Path(__file__).resolve().parents[1]
HUD = "ui\\hud\\"
EMBLEMS = "ui\\global_bitmaps\\emblems\\"
PACK = "H2UI_AUTHORED_SAMPLING_PACK_V3"
MARKER = "H2_MCC_CLASSIC_HUD_V1\nradar-size\t128\n"


def digest(data):
    return hashlib.sha256(data).hexdigest()


class MccCache(PcCache):
    """Bounded adapter for the existing read-only v13 directory reader."""
    bitmap_stride = 168
    def __init__(self, path):
        super().__init__(path)
        if not self.mcc:
            raise FormatError("Expected MCC Halo 2 Classic cache version 13")
        self.by_index = self.tags
        self.tags = {int(t["datum"], 16): dict(t, datum=int(t["datum"], 16),
            address=(t["offset"] + self.magic) if t["offset"] is not None else 0)
            for t in self.by_index.values()}

    u32 = PcCache.u
    string = Cache.string

    def u16(self, at):
        self.require(at, 2)
        return struct.unpack_from("<H", self.data, at)[0]

    def require(self, at, size):
        if not 0 <= at <= at + size <= len(self.data):
            raise FormatError("MCC metadata range outside decoded cache")

    def pointer(self, pointer, magic, size):
        at = pointer - magic
        self.require(at, size)
        return at

    def block(self, at, stride):
        count, pointer = self.u32(at), self.u32(at + 4)
        if count > 65536:
            raise FormatError("Excessive MCC metadata block")
        if not count:
            return []
        base = self.pointer(pointer, self.magic, count * stride)
        return [base + i * stride for i in range(count)]

    def tag(self, datum, group):
        tag = self.tags[datum]
        if tag["group"] != group or tag["offset"] is None:
            raise FormatError("Unresolved MCC tag owner")
        return tag, tag["offset"]


def mcc_directory(path):
    path = path.expanduser().resolve()
    for candidate in (path, path / "halo2", path.parent):
        if (candidate / "h2_maps_win64_dx11/textures.dat").is_file() and (candidate / "h2_fonts/font_table.txt").is_file():
            return candidate
    raise FormatError("Select the MCC installation or its halo2 directory")


def discover():
    libraries = [Path.home() / ".local/share/Steam", Path.home() / ".steam/steam",
                 Path("C:/Program Files (x86)/Steam")]
    import re
    for library in list(libraries):
        config = library / "steamapps/libraryfolders.vdf"
        if config.is_file():
            libraries.extend(Path(p.replace("\\\\", "\\")) for p in
                re.findall(r'"path"\s+"([^"\n]+)"', config.read_text()))
    for library in libraries:
        candidate = library / "steamapps/common/Halo The Master Chief Collection"
        try:
            return mcc_directory(candidate)
        except FormatError:
            pass
    raise FormatError("MCC was not found; pass --mcc-root with the installed game directory")


def bitmap_banks(cache):
    result = {}
    for tag in cache.tags.values():
        if tag["group"] != "bitm" or tag["offset"] is None or not tag["name"].startswith((HUD, EMBLEMS)):
            continue
        at = tag["offset"]
        frames = cache.block(at + 68, 168)  # Assembly Halo2MCC bitm.xml
        result[tag["name"]] = dict(tag=tag, frames=frames,
            sequences=sequences(cache, at, len(frames)),
            effective=effective_sequences(cache, at, frames))
    return result


def texture_resource(stream, cache, at, expected):
    pointers = [cache.u32(at + 56 + i * 4) for i in range(3)]
    sizes = [cache.u32(at + 80 + i * 4) for i in range(3)]
    if not sizes[0] or any(sizes[1:]) or any(pointers[1:]) or sizes[0] > 128 * 1024**2:
        raise FormatError("Unsupported split or missing MCC HUD texture")
    address = pointers[0] & 0x3fffffff
    stream.seek(address)
    header = stream.read(4)
    if len(header) != 4:
        raise FormatError("Truncated MCC texture segment count")
    count, = struct.unpack("<i", header)
    if not 0 < count <= 4096:
        raise FormatError("Excessive MCC texture segment count")
    header += stream.read(count * 4)
    if len(header) != (count + 1) * 4:
        raise FormatError("Truncated MCC texture segment table")
    segments = struct.unpack_from(f"<{count}i", header, 4)
    if any(s == 0 for s in segments) or len(header) + sum(abs(s) for s in segments) != sizes[0]:
        raise FormatError("MCC texture segment extent differs from bitmap record")
    result, stored = bytearray(), bytearray(header)
    for size in segments:
        block = stream.read(abs(size))
        if len(block) != abs(size):
            raise FormatError("Truncated MCC texture segment")
        stored.extend(block)
        if size < 0:
            decoded = block
        else:
            inflater = zlib.decompressobj()
            decoded = inflater.decompress(block, expected - len(result) + 1)
            if not inflater.eof or inflater.unused_data or inflater.unconsumed_tail or inflater.flush():
                raise FormatError("Invalid or excessive MCC texture zlib segment")
        result.extend(decoded)
        if len(result) > expected:
            raise FormatError("Excessive decoded MCC texture")
    if len(result) != expected:
        raise FormatError("MCC texture does not match full authored mip extent")
    return bytes(result), dict(address=address, pointers=pointers, stored_sizes=sizes,
        segments=segments, stored_sha256=digest(stored), payload_sha256=digest(result))


def export_bank(cache, bank, stream, output):
    name = bank["tag"]["name"]
    frames = []
    for ordinal, at in enumerate(bank["frames"]):
        width, height, depth, kind, fmt, flags = struct.unpack_from("<6H", cache.data, at + 4)
        if depth != 1 or kind != 0 or flags & 8:
            raise FormatError(f"Unsupported MCC HUD storage: {name}:{ordinal}")
        mips = cache.u16(at + 20)
        levels = mip_shapes(width, height, mips, fmt)
        payload, resource = texture_resource(stream, cache, at, sum(m["bytes"] for m in levels))
        dds, _ = dds_image(width, height, mips, fmt, payload)
        relative = Path("bitmaps") / safe_asset_path(name) / f"{ordinal:04d}.dds"
        destination = output / relative
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_bytes(dds)
        if digest(destination.read_bytes()) != digest(dds):
            raise FormatError("MCC DDS readback failed")
        frames.append(dict(frame=ordinal, width=width, height=height, format=fmt,
            additional_mips=mips, flags=flags, record_hex=cache.data[at:at + 168].hex(),
            dds=relative.as_posix(), dds_sha256=digest(dds), resource=resource))
    return dict(name=name, source_map=cache.path.name, datum=f'{bank["tag"]["datum"]:08x}',
        frames=frames, sequences=bank["sequences"])


def signature(cache, bank):
    # Ignore transient handles/addresses, retain authored records and resource
    # identities. Identical textures.dat locations denote identical source bytes.
    return dict(sequences=bank["sequences"], frames=[bytes(cache.data[p+4:p+28]).hex() +
        bytes(cache.data[p+56:p+68]).hex() + bytes(cache.data[p+80:p+92]).hex() for p in bank["frames"]])


def emit_pack(xbox, donor_banks, exports, output, filename, prefix):
    lines, mappings = [PACK], []
    for name, (frames, seqs) in sorted(banks(xbox, prefix).items()):
        if name not in donor_banks:
            raise FormatError("MCC is missing original HUD bank: " + name)
        donor = donor_banks[name]
        # The MCC backpack adds a final silenced-SMG page/sequence. All existing
        # 19 page owners and UV/registration bytes must match exactly.
        backpack = name == HUD + "bitmaps\\new_hud\\backpack_weapons"
        if not (len(donor["frames"]) == len(frames) or
                backpack and len(frames) == 19 and len(donor["frames"]) == 20):
            raise FormatError("MCC changes an original HUD bank count: " + name)
        if donor["effective"][:len(seqs)] != seqs or (not backpack and len(donor["effective"]) != len(seqs)):
            raise FormatError("MCC changes original HUD sprite ownership: " + name)
        for ordinal, at in enumerate(frames):
            frame = exports[name]["frames"][ordinal]
            logical = [xbox.u16(at + 4), xbox.u16(at + 6)]
            lines.append("\t".join((name, str(ordinal), str(len(frames)),
                *(str(n) for n in logical), frame["dds"])))
            mappings.append(dict(name=name, frame=ordinal, logical=logical,
                physical=[frame["width"], frame["height"]], source_frame=ordinal))
    (output / filename).write_text("\n".join(lines) + "\n")
    return mappings


def import_fonts(source, output):
    target = output / "fonts"
    target.mkdir()
    files = {}
    for path in sorted(source.iterdir()):
        if not path.is_file():
            raise FormatError("Unexpected directory in MCC font bank")
        data = path.read_bytes()
        if path.name.startswith("font_table"):
            names = data.decode("utf-8").splitlines()
            if len(names) != 12 or any(not n or Path(n).name != n or "/" in n or "\\" in n for n in names):
                raise FormatError("Invalid MCC font table")
            if any(not (source / name).is_file() for name in names):
                raise FormatError("Missing MCC font table owner")
        elif len(data) < 0x40400 or struct.unpack_from("<I", data, 512)[0] != 0xf0000001:
            raise FormatError("Unsupported MCC Classic font")
        shutil.copyfile(path, target / path.name)
        if (target / path.name).read_bytes() != data:
            raise FormatError("MCC font copy changed bytes")
        files[path.name] = dict(bytes=len(data), sha256=digest(data))
    return files


def export(mcc, xbox_maps, output, progress=print, menu_assets=None):
    maps = mcc / "h2_maps_win64_dx11"
    progress("Reading MCC Classic shared HUD tags and textures.dat…")
    shared = MccCache(maps / "shared.map")
    donor = bitmap_banks(shared)
    output.mkdir(parents=True)
    with (maps / "textures.dat").open("rb") as stream:
        exports = {name: export_bank(shared, bank, stream, output) for name, bank in sorted(donor.items())}
    signatures = {name: signature(shared, bank) for name, bank in donor.items()}
    scanned, additional = [], []
    progress("Checking every MCC Classic map for additional or changed HUD artwork…")
    for path in sorted(maps.glob("*.map")):
        cache = shared if path.name == "shared.map" else MccCache(path)
        local = bitmap_banks(cache)
        scanned.append(dict(file=path.name, sha256=cache.provenance["sha256"], local_banks=len(local)))
        for name, bank in local.items():
            sig = signature(cache, bank)
            if name in signatures:
                if sig != signatures[name]:
                    raise FormatError(f"Map-specific MCC HUD variant needs explicit mapping: {path.name}: {name}")
            else:
                with (maps / "textures.dat").open("rb") as stream:
                    exports[name] = export_bank(cache, bank, stream, output)
                signatures[name] = sig
                additional.append(name)
        if cache is not shared:
            del cache
    xbox = Cache(xbox_maps / "shared.map")
    try:
        hud = emit_pack(xbox, donor, exports, output, "hud.tsv", HUD)
        emblem = emit_pack(xbox, donor, exports, output, "scoreboard.tsv", EMBLEMS)
        # Scoreboard emblem/death owners also reference HUD pages. The native
        # library selects only the banks actually owned by its source shader.
        scoreboard = (output / "scoreboard.tsv").read_text()
        scoreboard += "\n".join((output / "hud.tsv").read_text().splitlines()[1:]) + "\n"
        (output / "scoreboard.tsv").write_text(scoreboard)
        # The shield widget layout has the same schema in MCC; import its actual
        # fill-to-outline registration, normalized to the original logical page.
        alignment = export_hud_alignment(xbox, shared, output, [r["name"] for r in hud])
    finally:
        xbox.close()
    fonts = import_fonts(mcc / "h2_fonts", output)
    (output / "hud-source.tsv").write_text(MARKER)
    if menu_assets:
        menu_assets = menu_assets.resolve()
        if not (menu_assets / "maps/01a_tutorial.map").is_file() or any(c in str(menu_assets) for c in "\t\r\n\0"):
            raise FormatError("Invalid separately selected pause-menu source")
        (output / "menu-ui-source.tsv").write_text("H2_HUD_MENU_SOURCE_V1\nvista\t" + str(menu_assets) + "\n")
    sources = ["tools/reference/reclaimer/Reclaimer.Blam/Blam/Halo2/BitmapTag.cs",
        "tools/reference/reclaimer/Reclaimer.Blam/Blam/Halo2/DataPointer.cs",
        "tools/reference/reclaimer/Reclaimer.Blam/Blam/Halo2/Config/CacheFile.config.cs",
        "analysis/multiplayer/pc-gameplay-tag-comparison-20261011/schemas/Halo2MCC/bitm.xml"]
    report = dict(schema="halo2-mcc-classic-hud-import-v1", complete=True,
        importer_sha256=file_sha(Path(__file__)),
        source=str(mcc), maps=scanned, menu_assets=str(menu_assets) if menu_assets else None,
        textures=dict(path=str(maps / "textures.dat"), bytes=(maps / "textures.dat").stat().st_size,
                      sha256=file_sha(maps / "textures.dat")),
        reader_sources={p: file_sha(ROOT / p) for p in sources},
        bitmaps=list(exports.values()), hud_mapping=hud, scoreboard_mapping=emblem,
        additional_banks=additional, fonts=fonts, alignment=alignment,
        summary=dict(maps=len(scanned), imported_banks=len(exports),
            imported_pages=sum(len(e["frames"]) for e in exports.values()),
            active_hud_banks=len({r["name"] for r in hud}), active_hud_pages=len(hud),
            font_files=sum(not n.startswith("font_table") for n in fonts),
            font_tables=sum(n.startswith("font_table") for n in fonts)),
        policy=dict(pixels="original MCC payloads", fonts="byte-identical MCC files and tables",
            artwork_resampling=False, image_generation=False, logical_layout="original Xbox",
            gameplay="unchanged", extra_mcc_pages="retained in source bank; selected only by existing owners"))
    (output / "manifest.json").write_text(json.dumps(report, indent=2) + "\n")
    return report


def activate(root, output):
    path = str(output.resolve())
    if any(c in path for c in "\t\r\n\0"):
        raise FormatError("HUD source path contains control characters")
    registry = root / "assets/hud-source.tsv"
    temporary = registry.with_name(".hud-source-" + uuid.uuid4().hex)
    temporary.write_text("H2_HUD_SOURCE_V1\nmcc\t" + path + "\n")
    os.replace(temporary, registry)


def selected_xbox_maps(root):
    for line in (root / "assets/asset-sources.tsv").read_text().splitlines()[1:]:
        if line.startswith("xbox\t"):
            path = Path(line.split("\t")[1])
            return (path if path.is_absolute() else root / path) / "maps"
    raise FormatError("Import the Xbox game files first or pass --xbox-maps")


def selected_menu_assets(root):
    registry = root / "assets/asset-sources.tsv"
    if registry.is_file():
        for line in registry.read_text().splitlines()[1:]:
            if line.startswith("vista\t"):
                path = Path(line.split("\t")[1])
                return path if path.is_absolute() else root / path
    return None


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--mcc-root", type=Path)
    parser.add_argument("--xbox-maps", type=Path)
    parser.add_argument("--menu-assets", type=Path, help="Separate existing Vista pause-menu import; auto-selected from asset setup")
    parser.add_argument("--output", type=Path)
    parser.add_argument("--activate", action="store_true", help="Use MCC HUD/fonts in both ordinary launchers")
    args = parser.parse_args()
    root = args.root.resolve()
    output = (args.output or root / "assets/imported" / ("mcc-hud-" + uuid.uuid4().hex)).resolve()
    if output.exists():
        parser.error("Output already exists; choose a new directory to preserve the existing import")
    mcc = mcc_directory(args.mcc_root) if args.mcc_root else discover()
    output.parent.mkdir(parents=True, exist_ok=True)
    stage = Path(tempfile.mkdtemp(prefix=".mcc-hud-", dir=output.parent))
    try:
        report = export(mcc, args.xbox_maps or selected_xbox_maps(root), stage / "pack",
                        menu_assets=args.menu_assets or selected_menu_assets(root))
        (stage / "pack").rename(output)
        if args.activate:
            activate(root, output)
        print(json.dumps(report["summary"], indent=2))
        print("Imported: " + str(output))
    finally:
        shutil.rmtree(stage)


if __name__ == "__main__":
    main()
