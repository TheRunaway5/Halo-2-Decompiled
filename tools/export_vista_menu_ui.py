#!/usr/bin/env python3
"""Export the supplied Halo 2 PC mainmenu's 2D UI images without importing tags.

The supported container is the independently inspected 11081 PC version-8 map.
DDS payloads retain the original image format and every serialized mip. Only
RFC1950 wrapping is removed; no bitmap pixels, layouts, fonts, shaders, geometry,
or camera values are resampled or substituted. PNG sheets are previews only.
Opt-in lobby metadata uses a separate versioned sidecar and retains source
records/locale ownership without changing the existing texture-pack format.
"""
import argparse
from collections import Counter
import hashlib
import io
import json
import mmap
from pathlib import Path, PureWindowsPath
import struct
import sys
import zlib

from audit_rendering import Cache, FormatError, file_sha, sha


class VistaCache(Cache):
    """Checked PC header/relative metadata mapping, separate from Xbox Cache."""
    def __init__(self, path):
        self.path = Path(path)
        self.file = self.path.open("rb")
        self.data = None
        try:
            self.data = mmap.mmap(self.file.fileno(), 0, access=mmap.ACCESS_READ)
            self.require(0, 2048)
            if (self.u32(0), self.u32(4), self.u32(2044)) != (0x68656164, 8, 0x666f6f74):
                raise FormatError("expected PC version-8 cache")
            self.build = self.string(0x12c, 32)
            if self.u32(8) != len(self.data) or self.build != "11081.07.04.30.0934.main":
                raise FormatError("unsupported PC build or truncated map")
            self.index = self.u32(16)
            self.require(self.index, 32)
            # src/ui/vista/vista_cache.cpp: directory table pointers are index-relative;
            # resident metadata additionally subtracts the header's VA bias.
            # Multiplayer caches use 0x1400000 here, unlike mainmenu/solo.
            self.magic = self.u32(32)-self.index
            count = self.u32(self.index + 24)
            if not 0 < count <= 65536 or count != self.u32(0x2cc):
                raise FormatError("invalid PC tag count")
            table = self.pointer(self.u32(self.index + 8), -self.index, count * 16)
            names, size, offsets = self.u32(0x2d0), self.u32(0x2d4), self.u32(0x2d8)
            self.require(names, size)
            self.require(offsets, count * 4)
            self.tags = {}
            for i in range(count):
                at = table + i * 16
                datum, address, length = self.u32(at + 4), self.u32(at + 8), self.u32(at + 12)
                # Shared/MP directories retain vacant slots with no filename.
                if datum==0xffffffff:
                    continue
                if datum&65535 != i:
                    raise FormatError("PC tag datum disagrees with directory slot")
                name = self.u32(offsets + i * 4)
                if name >= size:
                    raise FormatError("tag filename outside PC string table")
                if datum in self.tags:
                    raise FormatError("duplicate PC tag datum")
                group=self.u32(at).to_bytes(4, "big").decode("ascii")
                filename=self.string(names + name, size - name)
                if address:
                    self.pointer(address, self.magic, length)
                self.tags[datum] = dict(datum=datum,
                    group=group,name=filename,address=address,size=length)
            self.scenario = self.u32(self.index + 12)
            self.passes, self.templates, self.vertex_shaders, self.shaders, self.bitmaps = {}, {}, {}, {}, {}
        except Exception:
            self.close()
            raise

# DDS_PIXELFORMAT flags/masks for the formats actually present in this map.
# Format names and bitmap field offsets are independently retained in the
# supplied mods/plugins/bitm.xml. RGB/alpha masks describe little-endian words.
FORMATS = {
    1: ("Y8", 8, (0x20000, 0, 8, 0xff, 0, 0, 0)),
    3: ("A8Y8", 16, (0x20001, 0, 16, 0xff, 0, 0, 0xff00)),
    9: ("A4R4G4B4", 16, (0x41, 0, 16, 0xf00, 0xf0, 0xf, 0xf000)),
    10: ("X8R8G8B8", 32, (0x40, 0, 32, 0xff0000, 0xff00, 0xff, 0)),
    11: ("A8R8G8B8", 32, (0x41, 0, 32, 0xff0000, 0xff00, 0xff, 0xff000000)),
    14: ("DXT1", 4, (4, int.from_bytes(b"DXT1", "little"), 0, 0, 0, 0, 0)),
    15: ("DXT3", 8, (4, int.from_bytes(b"DXT3", "little"), 0, 0, 0, 0, 0)),
}


def mip_shapes(width, height, additional, fmt):
    if fmt not in FORMATS or not 0 < width <= 16384 or not 0 < height <= 16384:
        raise FormatError("unsupported UI image shape/format")
    if not 0 <= additional <= max(width, height).bit_length() - 1:
        raise FormatError("invalid UI mip count")
    rows, offset = [], 0
    for level in range(additional + 1):
        w, h = max(1, width >> level), max(1, height >> level)
        if fmt in (14, 15):
            size = ((w + 3) // 4) * ((h + 3) // 4) * (8 if fmt == 14 else 16)
        else:
            size = w * h * (FORMATS[fmt][1] // 8)
        rows.append(dict(level=level, width=w, height=h, offset=offset, bytes=size))
        offset += size
    return rows


def dds_image(width, height, additional, fmt, payload):
    levels = mip_shapes(width, height, additional, fmt)
    if len(payload) != sum(row["bytes"] for row in levels):
        raise FormatError("expanded resource does not match full mip extent")
    compressed = fmt in (14, 15)
    flags = 0x1007 | (0x80000 if compressed else 8) | (0x20000 if additional else 0)
    pitch = levels[0]["bytes"] if compressed else width * (FORMATS[fmt][1] // 8)
    caps = 0x1000 | (0x400008 if additional else 0)
    header = [124, flags, height, width, pitch, 0, additional + 1] + [0] * 11
    header += [32] + list(FORMATS[fmt][2]) + [caps, 0, 0, 0, 0]
    if len(header) != 31:
        raise AssertionError("DDS header word count")
    return b"DDS " + struct.pack("<31I", *header) + payload, levels


def expand_resource(blob, expected):
    # Bounded inflate also rejects unused/trailing input and concatenated streams.
    inflater = zlib.decompressobj()
    result = inflater.decompress(blob, expected + 1)
    if len(result) != expected or not inflater.eof or inflater.unused_data or inflater.unconsumed_tail:
        raise FormatError("invalid RFC1950 resource or incorrect expanded size")
    if inflater.flush():
        raise FormatError("unexpected trailing expanded pixels")
    return result


def safe_asset_path(name, require_ui=True):
    value = PureWindowsPath(name)
    if value.is_absolute() or value.drive or not value.parts or (require_ui and value.parts[0] != "ui"):
        raise FormatError("UI tag path must be relative")
    if any(p in ("", ".", "..") or any(c in p for c in '<>:"|?*') for p in value.parts):
        raise FormatError("unsafe UI tag path")
    return Path(*value.parts)


def write_stable(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.exists() and path.read_bytes() != data:
        raise FormatError(f"refusing to overwrite different existing export: {path}")
    if not path.exists():
        path.write_bytes(data)
    if path.read_bytes() != data:
        raise FormatError("export readback differs")


def sequences(cache, at, frame_count):
    result = []
    for ordinal, p in enumerate(cache.block(at + 0x3c, 0x3c)):
        first, count = cache.u16(p + 32), cache.u16(p + 34)
        if count and (first >= frame_count or count > frame_count - first):
            raise FormatError("sequence image range exceeds bitmap frames")
        sprites = []
        for s in cache.block(p + 52, 32):
            image = cache.u16(s)
            if image >= frame_count:
                raise FormatError("sprite image index exceeds bitmap frames")
            values = [cache.u32(s + i) for i in (8, 12, 16, 20, 24, 28)]
            sprites.append(dict(image=image, uv_and_registration_words=[f"{v:08x}" for v in values],
                uv_and_registration=[struct.unpack("<f", struct.pack("<I", v))[0] for v in values],
                raw_record_hex=cache.data[s:s + 32].hex()))
        result.append(dict(ordinal=ordinal, name=cache.string(p, 32), first_image=first,
            image_count=count, raw_record_hex=cache.data[p:p + 60].hex(), sprites=sprites))
    return result


def export_lobby_metadata(cache, output):
    """Retain PC lobby metadata and source preview DDS in a separate format.

    This opt-in sidecar does not change the texture-pack schema or assembly.
    It records raw authored records, owner-local strings and complete map slots;
    none of this selects a world cache, rewrites geometry, or enables a menu.
    """
    def record(at, size):
        cache.require(at, size)
        data = bytes(cache.data[at:at + size])
        return dict(file_offset=at, bytes=size, sha256=sha(data), raw_hex=data.hex())

    def tag(datum, group):
        if datum == 0xffffffff:
            return None
        t, at = cache.tag(datum, group)
        return dict(datum=f"{datum:08x}", group=group, name=t["name"],
                    address=t["address"], metadata=record(at, t["size"]))

    def word(at):
        return struct.unpack("<i", struct.pack("<I", cache.u32(at)))[0]

    def wide(at, count):
        cache.require(at, count * 2)
        return bytes(cache.data[at:at + count * 2]).decode("utf-16-le").split("\0", 1)[0]

    globals_tag, gp = cache.tag(cache.u32(cache.index + 16), "matg")
    scenario_tag, sp = cache.tag(cache.u32(cache.index + 12), "scnr")
    level_headers = cache.block(gp + 0x178, 24)
    path_headers = cache.block(gp + 0x170, 8)
    paths = [dict(source=record(p, 0x108), campaign_id=word(p), map_id=word(p + 4),
                  scenario_path=cache.string(p + 8, 256))
             for p in (cache.block(path_headers[0], 0x108) if path_headers else [])]
    preview_datums, previews = [], {}

    def preview_ref(datum):
        if datum != 0xffffffff and datum not in previews:
            t, at = cache.tag(datum, "bitm")
            frames = []
            images = cache.block(at + 0x44, 0x74)
            for ordinal, p in enumerate(images):
                width, height = cache.u16(p + 4), cache.u16(p + 6)
                fmt, mips = cache.u16(p + 12), cache.u16(p + 20)
                if not 0 < width <= 4096 or not 0 < height <= 4096 or cache.u16(p + 10) or cache.data[p + 8] != 1 or cache.u16(p + 14) & 8:
                    raise FormatError("invalid source-local Vista lobby preview")
                pointers = [cache.u32(p + 28 + i * 4) for i in range(3)]
                sizes = [cache.u32(p + 52 + i * 4) for i in range(3)]
                if pointers[1:] != [0, 0] or sizes[1:] != [0, 0] or pointers[0] >> 30 or not sizes[0]:
                    raise FormatError("external/split Vista lobby preview")
                cache.require(pointers[0], sizes[0])
                compressed = bytes(cache.data[pointers[0]:pointers[0] + sizes[0]])
                levels = mip_shapes(width, height, mips, fmt)
                payload = expand_resource(compressed, sum(level["bytes"] for level in levels))
                dds, levels = dds_image(width, height, mips, fmt, payload)
                destination = Path("bitmaps") / safe_asset_path(t["name"]) / f"frame-{ordinal:04d}.dds"
                write_stable(output / destination, dds)
                frames.append(dict(ordinal=ordinal, source=record(p, 0x74), width=width, height=height,
                    format=fmt, additional_mips=mips, resource_pointers=pointers, resource_sizes=sizes,
                    compressed_sha256=sha(compressed), expanded_bytes=len(payload), expanded_sha256=sha(payload),
                    dds=destination.as_posix(), dds_bytes=len(dds), dds_sha256=sha(dds), levels=levels))
            preview_datums.append(datum)
            previews[datum] = dict(owner=tag(datum, "bitm"), frames=frames, sequences=sequences(cache, at, len(images)))
        return datum

    campaign, multiplayer = [], []
    for p in cache.block(level_headers[0] + 8, 0xb50) if level_headers else []:
        cid, mid = word(p), word(p + 4)
        campaign.append(dict(source=record(p, 0xb50), campaign_id=cid, map_id=mid,
            bitmap=preview_ref(cache.u32(p + 12)), english_name=wide(p + 0x10, 32),
            english_description=wide(p + 0x250, 128), scenario_path=next((r["scenario_path"] for r in paths
                if r["campaign_id"] == cid and r["map_id"] == mid), "")))
    for p in cache.block(level_headers[0] + 16, 0xc64) if level_headers else []:
        multiplayer.append(dict(source=record(p, 0xc64), campaign_id=-1, map_id=word(p),
            bitmap=preview_ref(cache.u32(p + 8)), english_name=wide(p + 12, 32),
            english_description=wide(p + 0x24c, 128), scenario_path=cache.string(p + 0xb4c, 256),
            sort_order=word(p + 0xc4c), flags=cache.data[p + 0xc50],
            team_counts=list(cache.data[p + 0xc54:p + 0xc64])))

    loc = gp + 0x190
    count, size, indices, strings_at = [cache.u32(loc + i * 4) for i in range(4)]
    if count > 65536 or size > 16 * 1024 * 1024:
        raise FormatError("excessive Vista lobby locale")
    cache.require(indices, count * 8); cache.require(strings_at, size)
    locale_entries = []
    for i in range(count):
        p = indices + i * 8; offset = cache.u32(p + 4)
        if offset >= size:
            raise FormatError("invalid Vista lobby locale offset")
        text = cache.string(strings_at + offset, size - offset)
        locale_entries.append(dict(string=cache.u32(p), text=text, utf16le_hex=text.encode("utf-16-le").hex()))
    locales, parameters, screens = [], [], []
    for t in cache.tags.values():
        if t["group"] != "unic":
            continue
        _, p = cache.tag(t["datum"], "unic")
        first, n = cache.u16(p + 16), cache.u16(p + 18)
        if first > len(locale_entries) or n > len(locale_entries) - first:
            raise FormatError("invalid Vista lobby source-local unic range")
        locales.append(dict(owner=tag(t["datum"], "unic"), first=first, count=n))
    locale_owners = {int(v["owner"]["datum"], 16) for v in locales}

    def require_locale(datum):
        if datum != 0xffffffff and datum not in locale_owners:
            raise FormatError("missing Vista lobby source-local unic owner")

    for t in cache.tags.values():
        if t["group"] != "sily":
            continue
        _, p = cache.tag(t["datum"], "sily")
        owner = cache.u32(p + 12); require_locale(owner)
        parameters.append(dict(owner=tag(t["datum"], "sily"), parameter=cache.u32(p),
            default_value=word(p + 4), locale=owner, title=cache.u32(p + 16),
            header=cache.u32(p + 20), description=cache.u32(p + 24),
            choices=[dict(source=record(q, 12), value=word(q + 4), string=cache.u32(q + 8)) for q in cache.block(p + 28, 12)]))
    scenario_type = struct.unpack("<h", struct.pack("<H", cache.u16(sp + 0x10)))[0]
    ui_globals, game_settings = None, None
    shared_ui_globals = quick_settings_screen = variant_locale = 0xffffffff
    groups, variants = [], []
    interface_address = cache.u32(gp + 0x114)
    if cache.u32(gp + 0x110) and interface_address != 0xffffffff and 0 <= scenario_type <= 2:
        interface = cache.pointer(interface_address, cache.magic, 0x98)
        datum = cache.u32(interface + (0x8c if scenario_type == 0 else 0x94 if scenario_type == 1 else 0x84))
        if datum != 0xffffffff:
            ui_globals = tag(datum, "wgtz"); _, p = cache.tag(datum, "wgtz")
            shared_ui_globals, goof = cache.u32(p + 4), cache.u32(p + 20)
            if goof != 0xffffffff:
                game_settings = tag(goof, "goof"); _, q = cache.tag(goof, "goof")
                quick_settings_screen, variant_locale = cache.u32(q + 12), cache.u32(q + 36)
                require_locale(variant_locale)
                parameter_owners = {int(v["owner"]["datum"], 16) for v in parameters}
                for r in cache.block(q + 24, 24):
                    refs = [dict(source=record(s, 8), datum=cache.u32(s + 4)) for s in cache.block(r + 8, 8)]
                    if any(v["datum"] != 0xffffffff and v["datum"] not in parameter_owners for v in refs):
                        raise FormatError("missing Vista lobby source-local sily owner")
                    groups.append(dict(source=record(r, 24), key=word(r), parameters=refs))
                for r in cache.block(q + 40, 20):
                    variants.append(dict(source=record(r, 20), name=cache.u32(r), engine=cache.u32(r + 4),
                        values=[dict(source=record(s, 8), parameter=cache.u32(s), value=word(s + 4)) for s in cache.block(r + 8, 8)]))
    for t in cache.tags.values():
        if t["group"] != "wgit":
            continue
        _, p = cache.tag(t["datum"], "wgit"); owner = cache.u32(p + 28); require_locale(owner)
        screens.append(dict(owner=tag(t["datum"], "wgit"), id=cache.u16(p + 4), locale=owner,
            values=[dict(source=record(q, 12), name=cache.u32(q), strings=[dict(source=record(r, 4), string=cache.u32(r))
                    for r in cache.block(q + 4, 4)]) for q in cache.block(p + 48, 12)]))
    report = dict(schema="halo2-vista-lobby-metadata-v1", complete=True,
        source_map=dict(path=str(cache.path.resolve()), sha256=sha(cache.data), bytes=len(cache.data), build=cache.build),
        exporter_sha256=file_sha(Path(__file__)), globals=tag(globals_tag["datum"], "matg"),
        scenario=tag(scenario_tag["datum"], "scnr"), scenario_type=scenario_type,
        level_headers=[record(p, 24) for p in level_headers], path_headers=[record(p, 8) for p in path_headers],
        campaign=campaign, multiplayer=multiplayer, paths=paths, preview_datums=preview_datums,
        previews=[previews[d] for d in preview_datums], locale_entries=locale_entries, locales=locales,
        ui_globals=ui_globals, shared_ui_globals=shared_ui_globals, game_settings=game_settings,
        quick_settings_screen=quick_settings_screen, variant_locale=variant_locale,
        parameters=parameters, groups=groups, variants=variants, screens=screens,
        limits=["Retained PC records/preview DDS only; no visible menu-route or original final-pixel proof.",
                "Every NONE slot and ordered source list retained; display filtering belongs to a separate owner.",
                "No Xbox backdrop, geometry, camera, world shader or map-file selection changes.",
                "Per-unic ownership is retained; no cross-owner flattened string table."])
    encoded = (json.dumps(report, indent=2, allow_nan=False) + "\n").encode()
    write_stable(output / "lobby-metadata.json", encoded)
    return dict(path="lobby-metadata.json", sha256=sha(encoded), bytes=len(encoded), campaign_records=len(campaign),
                multiplayer_records=len(multiplayer), preview_banks=len(previews), parameters=len(parameters),
                groups=len(groups), variants=len(variants), locales=len(locales), screens=len(screens))


def preview(width, height, fmt, payload, dds):
    from PIL import Image
    count = width * height
    if fmt == 11:
        return Image.frombytes("RGBA", (width, height), payload[:count * 4], "raw", "BGRA")
    if fmt == 10:
        return Image.frombytes("RGB", (width, height), payload[:count * 4], "raw", "BGRX").convert("RGBA")
    if fmt == 1:
        return Image.frombytes("L", (width, height), payload[:count]).convert("RGBA")
    if fmt == 3:
        return Image.frombytes("LA", (width, height), payload[:count * 2]).convert("RGBA")
    if fmt == 9:
        rgba = bytearray(count * 4)
        for i, (v,) in enumerate(struct.iter_unpack("<H", payload[:count * 2])):
            rgba[i * 4:i * 4 + 4] = bytes(((v >> 8 & 15) * 17, (v >> 4 & 15) * 17,
                                         (v & 15) * 17, (v >> 12) * 17))
        return Image.frombytes("RGBA", (width, height), bytes(rgba))
    # Contact-sheet rendering is explicitly not a source BC endpoint parity test.
    return Image.open(io.BytesIO(dds)).convert("RGBA")


def emit_pack(output, manifest, approved_path, inventory_path):
    approved = json.loads(approved_path.read_text())
    if approved["vista_cache_sha256"] != manifest["source_map"]["sha256"]:
        raise FormatError("approved source map differs from exported map")
    lines = inventory_path.read_text().splitlines()
    if not lines or lines[0] != "H2UI_LOGICAL_IMAGES_V1":
        raise FormatError("invalid target image inventory")
    targets = {}
    for line in lines[1:]:
        if not line:
            continue
        fields = line.split("\t")
        if len(fields) != 5:
            raise FormatError("invalid target image inventory row")
        name, frame, count, width, height = fields
        safe_asset_path(name, require_ui=False)
        frame, count, width, height = map(int, (frame, count, width, height))
        if not 0 <= frame < count <= 65536 or not 0 < width <= 16384 or not 0 < height <= 16384:
            raise FormatError("invalid target image bank or extent")
        bank = targets.setdefault(name, {})
        if frame in bank:
            raise FormatError("duplicate target image inventory row")
        bank[frame] = (count, width, height)
    for bank in targets.values():
        counts = {row[0] for row in bank.values()}
        if len(counts) != 1 or set(bank) != set(range(next(iter(counts)))):
            raise FormatError("incomplete target image bank")
    sources = {row["name"]: row for row in manifest["bitmaps"]}
    pack, included, absent = ["H2UI_TEXTURE_PACK_V1"], [], []
    seen = set()
    for pair in approved["approved_pairs"]:
        name = pair["name"]
        if name in seen or "framing" in name.lower():
            raise FormatError("duplicate or forbidden framing approval")
        seen.add(name)
        if name not in targets:
            absent.append(name)
            continue
        if name not in sources:
            raise FormatError("approved bitmap absent from rip")
        frames, pages, bank = sources[name]["frames"], pair["pages"], targets[name]
        if len(frames) != len(pages) or len(bank) != len(pages):
            raise FormatError("approved entire bank count does not match source/target")
        numerator, denominator = pair["density"]
        if (numerator, denominator) not in ((1, 1), (2, 1)):
            raise FormatError("unsupported approved page density")
        for ordinal, (frame, page) in enumerate(zip(frames, pages)):
            if page["xbox_page"] != ordinal or page["vista_page"] != ordinal:
                raise FormatError("unproved approved page remapping")
            if list(bank[ordinal][1:]) != page["xbox_logical_extent"]:
                raise FormatError("actual target logical extent differs from approval")
            if [frame["width"], frame["height"]] != page["vista_sampled_extent"]:
                raise FormatError("exported sampled extent differs from approval")
            if any(v * denominator != x * numerator for v, x in
                   zip(page["vista_sampled_extent"], page["xbox_logical_extent"])):
                raise FormatError("approved density does not match image extents")
            relative = Path(frame["dds"])
            if relative.is_absolute() or ".." in relative.parts:
                raise FormatError("unsafe relative DDS path")
            dds = (output / relative).read_bytes()
            if len(dds) != frame["dds_bytes"] or sha(dds) != frame["dds_sha256"]:
                raise FormatError("exported DDS changed before pack assembly")
            pack.append("\t".join((name, str(ordinal), str(len(frames)),
                str(bank[ordinal][1]), str(bank[ordinal][2]), relative.as_posix())))
        included.append(name)
    encoded = ("\n".join(pack) + "\n").encode()
    write_stable(output / "pack.tsv", encoded)
    report = dict(complete=True, schema="halo2-vista-ui-pack-assembly-v1",
        exporter_sha256=file_sha(Path(__file__)),
        source_manifest_sha256=file_sha(output / "manifest.json"),
        approved_pairs=dict(path=str(approved_path.resolve()), sha256=file_sha(approved_path)),
        target_inventory=dict(path=str(inventory_path.resolve()), sha256=file_sha(inventory_path)),
        source_xbox_cache_sha256=approved["xbox_cache_sha256"],
        source_vista_cache_sha256=approved["vista_cache_sha256"],
        pack=dict(path="pack.tsv", sha256=sha(encoded), bytes=len(encoded),
            bitmap_banks=len(included), frames=len(pack) - 1),
        included_names=included, approved_but_absent_from_actual_target=absent,
        limits=["Opt-in texture-only candidates; no original art equivalence claim.",
            "Only existing actual target banks, exact original logical extents, and approved identity mapping.",
            "No framing/fonts/layout/flow/model/world/camera imports."])
    write_stable(output / "pack-manifest.json", (json.dumps(report, indent=2) + "\n").encode())
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--map", required=True, type=Path, help="PC mainmenu.map build11081")
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--approved-pairs", type=Path, help="independently approved bank correspondence JSON")
    parser.add_argument("--target-inventory", type=Path, help="actual Xbox FrontendContent image inventory TSV")
    parser.add_argument("--pack-only", action="store_true", help="assemble optional pack from existing rip; do not rewrite rip")
    parser.add_argument("--lobby-metadata", action="store_true", help="also emit the source-local PC lobby metadata sidecar")
    parser.add_argument("--lobby-metadata-only", action="store_true", help="emit only PC lobby metadata and its source preview DDS")
    args = parser.parse_args()
    if bool(args.approved_pairs) != bool(args.target_inventory):
        parser.error("pack emission requires both --approved-pairs and --target-inventory")
    if args.pack_only and (args.lobby_metadata or args.lobby_metadata_only):
        parser.error("lobby metadata export cannot be combined with --pack-only")
    if args.lobby_metadata_only:
        if args.approved_pairs:
            parser.error("--lobby-metadata-only does not assemble a texture pack")
        args.output.mkdir(parents=True, exist_ok=True)
        cache = VistaCache(args.map)
        try:
            print(json.dumps(export_lobby_metadata(cache, args.output), indent=2))
        finally:
            cache.close()
        return
    if args.pack_only:
        if not args.approved_pairs:
            parser.error("--pack-only requires --approved-pairs and --target-inventory")
        manifest = json.loads((args.output / "manifest.json").read_text())
        if manifest["schema"] != "halo2-vista-ui-dds-v1" or not manifest["complete"]:
            raise FormatError("invalid existing rip manifest")
        if file_sha(args.map) != manifest["source_map"]["sha256"]:
            raise FormatError("--map differs from existing rip source")
        report = emit_pack(args.output, manifest, args.approved_pairs, args.target_inventory)
        print(json.dumps(report["pack"], indent=2))
        return
    from PIL import Image, ImageDraw
    args.output.mkdir(parents=True, exist_ok=True)
    cache = VistaCache(args.map)
    rows, thumbnails, format_counts, raw_total, expanded_total = [], [], Counter(), 0, 0
    try:
        tags = sorted((t for t in cache.tags.values() if t["group"] == "bitm" and
            t["name"].startswith("ui\\")), key=lambda t:t["name"])
        for tag in tags:
            _, at = cache.tag(tag["datum"], "bitm")
            images = cache.block(at + 0x44, 0x74)
            frames = []
            for frame, p in enumerate(images):
                width, height = cache.u16(p + 4), cache.u16(p + 6)
                depth = cache.data[p + 8]
                kind, fmt, flags = cache.u16(p + 10), cache.u16(p + 12), cache.u16(p + 14)
                additional, low_detail = cache.u16(p + 20), cache.u16(p + 22)
                if kind != 0 or depth != 1 or flags & 8:
                    raise FormatError(f"unsupported non-2D/swizzled UI image: {tag['name']}:{frame}")
                pointers = [cache.u32(p + 28 + i * 4) for i in range(3)]
                sizes = [cache.u32(p + 52 + i * 4) for i in range(3)]
                if not sizes[0] or sizes[1:] != [0, 0] or pointers[0] >> 30:
                    raise FormatError("unsupported missing/split/external image resource")
                cache.require(pointers[0], sizes[0])
                source = bytes(cache.data[pointers[0]:pointers[0] + sizes[0]])
                levels = mip_shapes(width, height, additional, fmt)
                payload = expand_resource(source, sum(m["bytes"] for m in levels))
                dds, levels = dds_image(width, height, additional, fmt, payload)
                destination = Path("bitmaps") / safe_asset_path(tag["name"]) / f"frame-{frame:04d}.dds"
                write_stable(args.output / destination, dds)
                for level in levels:
                    start = level["offset"]
                    level["sha256"] = sha(payload[start:start + level["bytes"]])
                registration = [struct.unpack("<h", struct.pack("<H", cache.u16(p + x)))[0] for x in (16, 18)]
                record = dict(frame=frame, width=width, height=height, depth=depth, type=kind,
                    format=fmt, format_name=FORMATS[fmt][0], flags=flags,
                    additional_mips=additional, low_detail_mips=low_detail, registration=registration,
                    pixels_offset=cache.u32(p + 24), resource_pointers=pointers, resource_sizes=sizes,
                    raw_image_record_hex=cache.data[p:p + 116].hex(),
                    source_resource_sha256=sha(source), expanded_payload_bytes=len(payload),
                    expanded_payload_sha256=sha(payload), levels=levels,
                    dds=str(destination), dds_bytes=len(dds), dds_sha256=sha(dds))
                frames.append(record)
                rgba = preview(width, height, fmt, payload, dds)
                rgba.thumbnail((120, 96), Image.Resampling.LANCZOS)
                thumbnails.append((tag["name"], frame, rgba))
                raw_total += len(source)
                expanded_total += len(payload)
                format_counts[FORMATS[fmt][0]] += 1
            rows.append(dict(name=tag["name"], datum=f"{tag['datum']:08x}",
                raw_tag_header_hex=cache.data[at:at + 76].hex(),
                sequences=sequences(cache, at, len(images)), frames=frames))
        sheets = []
        for start in range(0, len(thumbnails), 256):
            batch = thumbnails[start:start + 256]
            sheet = Image.new("RGB", (16 * 128, ((len(batch) + 15) // 16) * 128), (24, 28, 35))
            draw = ImageDraw.Draw(sheet)
            for i, (name, frame, thumb) in enumerate(batch):
                x, y = (i % 16) * 128, (i // 16) * 128
                for cy in range(y, y + 96, 8):
                    for cx in range(x, x + 128, 8):
                        draw.rectangle((cx, cy, cx + 7, cy + 7), fill=(65, 65, 65) if
                            ((cx - x) // 8 + (cy - y) // 8) % 2 else (105, 105, 105))
                sheet.paste(thumb, (x + (128 - thumb.width) // 2, y + (96 - thumb.height) // 2), thumb)
                short = name.rsplit("\\", 1)[-1]
                draw.text((x + 2, y + 98), short[:20], fill="white")
                draw.text((x + 2, y + 110), f"#{start + i:04d} frame {frame}", fill=(185, 200, 220))
            stream = io.BytesIO()
            sheet.save(stream, format="PNG")
            path = Path(f"contact-sheet-{start // 256 + 1:02d}.png")
            write_stable(args.output / path, stream.getvalue())
            sheets.append(dict(path=str(path), bytes=len(stream.getvalue()), sha256=sha(stream.getvalue()),
                first_frame=start, frame_count=len(batch)))
        report = dict(schema="halo2-vista-ui-dds-v1", complete=True,
            source_map=dict(path=str(args.map.resolve()), bytes=len(cache.data), sha256=sha(cache.data),
                build=cache.build, metadata_index=cache.index, tag_count=len(cache.tags)),
            exporter=dict(path=str(Path(__file__).resolve()), sha256=file_sha(Path(__file__)),
                python=sys.version, zlib=zlib.ZLIB_VERSION),
            command=[str(Path(__file__).resolve()), "--map", str(args.map.resolve()),
                "--output", str(args.output.resolve())],
            summary=dict(bitmap_tags=len(rows), image_frames=len(thumbnails),
                formats=dict(format_counts), compressed_source_bytes=raw_total,
                expanded_native_payload_bytes=expanded_total,
                mip_levels=sum(len(f["levels"]) for r in rows for f in r["frames"])),
            bitmaps=rows, contact_sheets=sheets,
            scope="2D UI bitm resources only; includes HUD UI bitmaps serialized in mainmenu.",
            validation=["All source ranges and tag references checked within exact map allocation.",
                "Every RFC1950 stream has exact expected full mip bytes, end marker, and no trailing input.",
                "Every DDS payload after its128-byte header equals the unchanged expanded source resource.",
                "Every emitted DDS reread equals its generated bytes; per-mip/output SHA256 retained.",
                "All sequence ranges/sprite frame indices validated; original records and float words retained."],
            limits=["No runtime overlay, sampler, visual parity, or frame remapping proof.",
                "No wgit/wigl/layout/font/3D geometry/material/shader/camera imports.",
                "Original PC image extents must not replace Xbox logical geometry automatically.",
                "PNG contact sheets are resized previews; Pillow BC decoding is not source endpoint proof.",
                "The supplied installer is a Project Cartographer distribution, not established pristine retail provenance."])
        encoded = (json.dumps(report, indent=2, allow_nan=False) + "\n").encode()
        write_stable(args.output / "manifest.json", encoded)
        if args.lobby_metadata:
            export_lobby_metadata(cache, args.output)
        if args.approved_pairs:
            emit_pack(args.output, report, args.approved_pairs, args.target_inventory)
        print(json.dumps(dict(complete=True, manifest=str((args.output / "manifest.json").resolve()),
            manifest_sha256=sha(encoded), **report["summary"]), indent=2))
    finally:
        cache.close()


if __name__ == "__main__":
    main()
