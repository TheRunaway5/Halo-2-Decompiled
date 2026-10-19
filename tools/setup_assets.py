#!/usr/bin/env python3
"""MCC-only onboarding, with retained helpers for explicit reference research."""
from __future__ import annotations

import argparse
from contextlib import ExitStack, contextmanager
import json
import mmap
import os
import select
import signal
from pathlib import Path, PurePosixPath
import shutil
import struct
import subprocess
import sys
import tempfile
import threading
import uuid
import zipfile

from audit_rendering import Cache, FormatError, file_sha
from disc import Disc, inspect_xbe
from export_vista_menu_ui import dds_image, expand_resource, mip_shapes, safe_asset_path
from prepare_vista_loading import prepare as prepare_loading

SCHEMA = "H2_ASSET_SOURCES_V1"
VISTA_BUILD = "11081.07.04.30.0934.main"
XBOX_XBE = "03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d"
XBOX_MAPS = (
    "00a_introduction", "01a_tutorial", "01b_spacestation", "03a_oldmombasa",
    "03b_newmombasa", "04a_gasgiant", "04b_floodlab", "05a_deltaapproach",
    "05b_deltatowers", "06a_sentinelwalls", "06b_floodzone", "07a_highcharity",
    "07b_forerunnership", "08a_deltacliffs", "08b_deltacontrol", "ascension",
    "beavercreek", "burial_mounds", "coagulation", "colossus", "cyclotron",
    "foundation", "headlong", "lockout", "midship", "waterworks", "zanzibar",
    "mainmenu", "shared", "single_player_shared",
)


class VistaUiCache(Cache):
    """Read only the inspected Vista directory, including shared-map slots.

    Vista metadata uses header+0x20 minus index, independently of its relative
    directory. Unused shared slots have a null datum/name. This directory
    reader does not decode geometry or execute shader programs.
    """
    def __init__(self, path):
        self.path = Path(path)
        self.file = self.path.open("rb")
        self.data = None
        try:
            self.data = mmap.mmap(self.file.fileno(), 0, access=mmap.ACCESS_READ)
            self.require(0, 2048)
            if (self.u32(0), self.u32(4), self.u32(2044)) != (0x68656164, 8, 0x666f6f74):
                raise FormatError("expected a Halo 2 Vista cache")
            if self.u32(8) != len(self.data) or self.string(300, 32) != VISTA_BUILD:
                raise FormatError("unsupported or truncated Halo 2 Vista cache")
            index = self.u32(16)
            self.require(index, 32)
            count = self.u32(index + 24)
            if not 0 < count <= 65536 or count != self.u32(716):
                raise FormatError("invalid Vista directory count")
            table = self.pointer(self.u32(index + 8), -index, count * 16)
            self.magic = self.u32(32) - index
            names, size, offsets = self.u32(720), self.u32(724), self.u32(728)
            self.require(names, size)
            self.require(offsets, count * 4)
            self.tags = {}
            for i in range(count):
                p = table + i * 16
                datum, address, length = self.u32(p + 4), self.u32(p + 8), self.u32(p + 12)
                if datum == 0xffffffff:
                    continue
                name = self.u32(offsets + i * 4)
                if name >= size:
                    raise FormatError("invalid Vista tag filename")
                if address:
                    self.pointer(address, self.magic, length)
                if datum in self.tags:
                    raise FormatError("duplicate Vista datum")
                self.tags[datum] = dict(datum=datum, address=address, size=length,
                    group=self.u32(p).to_bytes(4, "big").decode("ascii"),
                    name=self.string(names + name, size - name))
        except BaseException:
            self.close()
            raise


def regular(path):
    path = Path(path)
    if path.is_symlink() or not path.is_file():
        raise FormatError(f"missing regular source file: {path}")
    return path


def selected_member(name, gameplay=False, loading_only=False):
    """Select UI inputs, or all Vista maps for the checked conversion tool."""
    p = PurePosixPath(name)
    if p.is_absolute() or any(x in ("..", ".") for x in p.parts) or "\\" in name:
        raise FormatError("unsafe Vista archive path")
    if name in ("loading.bin", "halo2.exe"):
        return True
    if loading_only:
        return False
    return (gameplay and len(p.parts) == 2 and p.parts[0] == "maps" and
            p.suffix == ".map") or name in ("maps/mainmenu.map", "maps/shared.map") or (
        len(p.parts) == 3 and p.parts[:2] == ("maps", "fonts"))


def extract_vista(source, target, progress=print, *, gameplay=False, loading_only=False):
    source = source.resolve()
    target.mkdir(parents=True)
    if source.is_dir():
        # A previously extracted installer or installed game is also a donor.
        base = source if (source / "maps/mainmenu.map").is_file() else source.parent
        if (base / "maps/mainmenu.map").is_file() or (loading_only and (source / "loading.bin").is_file()):
            if loading_only and (source / "loading.bin").is_file():
                base = source
            selected = [] if loading_only else ([regular(p) for p in sorted((base / "maps").glob("*.map"))] if gameplay else
                        [regular(base / "maps" / name) for name in ("mainmenu.map", "shared.map")])
            if not loading_only:
                selected += [regular(p) for p in sorted((base / "maps/fonts").iterdir())]
            selected += [regular(base / name) for name in ("loading.bin", "halo2.exe") if (base / name).exists()]
            for path in selected:
                relative = path.relative_to(base)
                destination = target / relative
                destination.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(path, destination)
            return dict(path=str(source), kind="directory")
        source = regular(source / "h2pc_data.bin")
    else:
        regular(source)
        if source.suffix.lower() == ".exe":
            # Select the accompanying data archive; never execute an installer.
            source = regular(source.with_name("h2pc_data.bin"))
    if zipfile.is_zipfile(source):
        with zipfile.ZipFile(source) as archive:
            rows = archive.infolist()
            nested = [r for r in rows if r.filename == "h2pc_data.bin"]
            if nested:
                if len(nested) != 1 or nested[0].file_size > 8 * 1024**3:
                    raise FormatError("invalid bundled Vista data archive")
                progress("Reading the Vista installer data archive…")
                with tempfile.TemporaryDirectory(prefix="halo2-vista-") as temporary:
                    payload = Path(temporary) / "h2pc_data.bin"
                    with archive.open(nested[0]) as inp, payload.open("xb") as out:
                        shutil.copyfileobj(inp, out, 1024 * 1024)
                    extract_vista(payload, target / "nested", progress, gameplay=gameplay, loading_only=loading_only)
                    for child in (target / "nested").iterdir():
                        child.rename(target / child.name)
                    (target / "nested").rmdir()
                    return dict(path=str(source), kind="installer-zip", sha256=file_sha(source))
            seen = set()
            for row in rows:
                if row.is_dir() or not selected_member(row.filename, gameplay, loading_only):
                    continue
                if row.filename in seen or row.file_size > 512 * 1024**2 or ((row.external_attr >> 16) & 0xf000) == 0xa000:
                    raise FormatError("invalid or duplicate Vista archive member")
                seen.add(row.filename)
                destination = target / row.filename
                destination.parent.mkdir(parents=True, exist_ok=True)
                with archive.open(row) as inp, destination.open("xb") as out:
                    shutil.copyfileobj(inp, out, 1024 * 1024)
    else:
        program = shutil.which("7z") or shutil.which("7zz")
        if not program:
            raise FormatError("Reading this installer requires 7-Zip (7z or 7zz)")
        listing = subprocess.run([program, "l", "-slt", "--", str(source)],
            check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True).stdout
        names = []
        for record in listing.split("\n\n"):
            fields = dict(line.split(" = ", 1) for line in record.splitlines() if " = " in line)
            name = fields.get("Path", "")
            if name == str(source) or not name or fields.get("Folder") == "+":
                continue
            if selected_member(name, gameplay, loading_only):
                if int(fields.get("Size", "0")) > 512 * 1024**2 or "Symbolic Link" in fields:
                    raise FormatError("invalid Vista archive member")
                names.append(name)
        if len(set(names)) != len(names) or not names:
            raise FormatError("No supported Vista UI files in the installer")
        # Extract the exact allowlist in one pass through solid installer data.
        # -spd prevents filenames from being treated as wildcard expressions.
        subprocess.run([program, "x", "-y", "-spd", "-o" + str(target.resolve()),
                        "--", str(source), *names], check=True,
                       stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        for name in names:
            regular(target / name)
    if not loading_only:
        for name in ("mainmenu.map", "shared.map"):
            regular(target / "maps" / name)
    return dict(path=str(source), kind="archive", sha256=file_sha(source))


def effective_sequences(cache, at, frames):
    """The existing native UI uses explicit sprites or full-page fallback rows."""
    out = []
    for p in cache.block(at + 60, 60):
        sprites = []
        for s in cache.block(p + 52, 32):
            image = cache.u16(s)
            if image >= len(frames):
                raise FormatError("UI sprite outside bank")
            sprites.append((image, bytes(cache.data[s + 8:s + 32])))
        if not sprites:
            first, count = cache.u16(p + 32), cache.u16(p + 34)
            if first + count > len(frames):
                raise FormatError("UI sequence outside bank")
            sprites = [(i, struct.pack("<6f", 0, 1, 0, 1, 0, 0)) for i in range(first, first + count)]
        out.append(sprites)
    return out


def banks(cache, prefix="ui\\"):
    result = {}
    for tag in cache.tags.values():
        if tag["group"] != "bitm" or not tag["address"] or not tag["name"].startswith(prefix):
            continue
        _, at = cache.tag(tag["datum"], "bitm")
        frames = cache.block(at + 68, getattr(cache, "bitmap_stride", 116))
        if tag["name"] in result:
            raise FormatError("duplicate UI bitmap path")
        result[tag["name"]] = (frames, effective_sequences(cache, at, frames))
    return result


VISTA_CROSSHAIR_PREFIX = "ui\\hud\\bitmaps\\new_hud\\crosshairs\\"
# Inspected shared.map page records, not a tolerance around doubled sizes.
# These seven authored pages have extra pixels compared with twice Xbox's
# extent. Sprite correspondence is still exact; native quads use Xbox extents.
VISTA_CROSSHAIR_EXTENTS = {
    ("hud_reticles", 3): ((53, 45), (106, 91)),
    ("hud_reticles", 5): ((28, 50), (59, 100)),
    ("hud_reticles", 16): ((55, 55), (110, 111)),
    ("hud_reticles", 22): ((9, 9), (20, 20)),
    ("hud_reticles", 41): ((52, 55), (106, 111)),
    ("hud_weak_spot_reticles", 14): ((35, 35), (70, 71)),
    ("hud_weak_spot_reticles", 22): ((49, 54), (98, 109)),
}


def crosshair_extent(name, frame, original, sampled):
    if not name.startswith(VISTA_CROSSHAIR_PREFIX):
        return False
    entry = VISTA_CROSSHAIR_EXTENTS.get((name.removeprefix(VISTA_CROSSHAIR_PREFIX), frame))
    return entry == (original, sampled)


def compatible(xbox, original, vista, sampled, name=""):
    xf, xs = original
    vf, vs = sampled
    if not xf or len(xf) != len(vf) or xs != vs:
        return "bank or sprite correspondence"
    density = None
    for frame, (a, b) in enumerate(zip(xf, vf)):
        xw, xh, vw, vh = xbox.u16(a + 4), xbox.u16(a + 6), vista.u16(b + 4), vista.u16(b + 6)
        authored_crosshair = crosshair_extent(name, frame, (xw, xh), (vw, vh))
        if not xw or not xh or (not authored_crosshair and (vw, vh) not in ((xw, xh), (xw * 2, xh * 2))):
            return "page dimensions"
        scale = 2 if authored_crosshair else vw // xw
        if density is not None and density != scale:
            return "mixed page density"
        density = scale
        if vista.u16(b + 8) != 1 or vista.u16(b + 10) != 0 or vista.u16(b + 14) & 8:
            return "unsupported image storage"
    return None


def resource(cache, p):
    pointer, size = cache.u32(p + 28), cache.u32(p + 52)
    if not size or size > 128 * 1024**2 or cache.u32(p + 56) or cache.u32(p + 60):
        raise FormatError("unsupported split Vista UI resource")
    location = pointer >> 30
    owner = cache.path if not location else cache.path.parent / (
        "", "mainmenu.map", "shared.map", "single_player_shared.map")[location]
    with regular(owner).open("rb") as stream:
        stream.seek(pointer & 0x3fffffff)
        data = stream.read(size)
    if len(data) != size:
        raise FormatError("truncated Vista UI resource")
    return data


def export_pack(xbox, vista, output, manifest, prefix, names=None):
    original, sampled = banks(xbox, prefix), banks(vista, prefix)
    lines = ["H2UI_SAMPLING_PACK_V2"]
    included, excluded = [], {}
    for name, bank in sorted(original.items()):
        if names is not None and name not in names:
            continue
        if "framing" in name.lower() or name not in sampled:
            if name.startswith(VISTA_CROSSHAIR_PREFIX):
                raise FormatError("Missing Vista crosshair bank: " + name)
            excluded[name] = "frame seam" if "framing" in name.lower() else "absent from Vista donor"
            continue
        reason = compatible(xbox, bank, vista, sampled[name], name)
        if reason:
            if name.startswith(VISTA_CROSSHAIR_PREFIX):
                raise FormatError("Incompatible Vista crosshair bank: " + name + ": " + reason)
            excluded[name] = reason
            continue
        for frame, (a, b) in enumerate(zip(bank[0], sampled[name][0])):
            w, h, fmt, mips = vista.u16(b + 4), vista.u16(b + 6), vista.u16(b + 12), vista.u16(b + 20)
            levels = mip_shapes(w, h, mips, fmt)
            payload = expand_resource(resource(vista, b), sum(x["bytes"] for x in levels))
            dds, _ = dds_image(w, h, mips, fmt, payload)
            relative = Path("bitmaps") / manifest.removesuffix(".tsv") / safe_asset_path(name) / f"{frame:04d}.dds"
            destination = output / relative
            destination.parent.mkdir(parents=True, exist_ok=True)
            destination.write_bytes(dds)
            lines.append("\t".join((name, str(frame), str(len(bank[0])), str(xbox.u16(a + 4)),
                str(xbox.u16(a + 6)), relative.as_posix())))
        included.append(name)
    if not included:
        raise FormatError(f"No compatible Vista images for {manifest}")
    (output / manifest).write_text("\n".join(lines) + "\n")
    return dict(banks=len(included), frames=len(lines) - 1, included=included, retained_original=excluded)


def export_scoreboard_artwork(xbox, vista, output):
    """Vista4749F7/46BE60: solid cells plus the mulg-owned emblem pages.

    No mainmenu panel is substituted for the source's untextured672D24 cells.
    The five paint words below are literal supported-11081 executable words:
    79DED0,7A03E0,79C168,7BA848,7B6C78 respectively. Geometry stays Xbox-owned.
    """
    bindings = []
    palettes = {}
    names = {"ui\\global_bitmaps\\emblems\\background",
             "ui\\global_bitmaps\\emblems\\foreground"}
    for cache in (xbox, vista):
        globals_tag = next(t for t in cache.tags.values() if t["group"] == "matg")
        gp = cache.tag(globals_tag["datum"], "matg")[1]
        mp = cache.tag(cache.u32(gp + 0x16c), "mulg")[1]
        if cache.u32(mp + 8) != 1:
            raise FormatError("Scoreboard requires its actual single multiplayer runtime owner")
        runtime = cache.pointer(cache.u32(mp + 12), cache.magic, 1384)
        owners = [cache.tags[cache.u32(runtime + off)] for off in (0x54c, 0x554, 0x55c, 0x564)]
        bindings.append([t["name"] for t in owners])
        names.update(t["name"] for t in (owners[1], owners[3]))
        if cache is vista:
            universal = cache.pointer(cache.u32(mp + 4), cache.magic, 0x20)
            for name, at in (("palette", gp + 0x160), ("team_palette", universal + 0x10)):
                count = cache.u32(at)
                if not 0 < count <= 4096:
                    raise FormatError("Invalid Vista scoreboard color palette")
                base = cache.pointer(cache.u32(at + 4), cache.magic, count * 12)
                palettes[name] = [[cache.u32(base + i * 12 + j * 4) for j in range(3)] for i in range(count)]
    if bindings[0] != bindings[1]:
        raise FormatError("Vista scoreboard changes the supported image/shader owners")
    pack = export_pack(xbox, vista, output, "scoreboard.tsv", "ui\\", names)
    if set(pack["included"]) != names:
        raise FormatError("Incomplete Vista scoreboard artwork; Xbox fallback is forbidden")
    report = dict(schema="H2UI_SCOREBOARD_V1", **palettes,
        # header RGB, background alpha, text scale, text bias, inactive dim.
        style_words=[0x3e000000, 0x3e800000, 0x3e99999a, 0x3f333333, 0x3ecccccd],
        bindings=bindings[1], source_map_sha256=file_sha(vista.path),
        reference_executable_sha256="33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109")
    (output / "scoreboard.json").write_text(json.dumps(report, indent=2) + "\n")
    return pack


def export_hud_alignment(xbox, vista, output, included):
    """Keep Xbox geometry; sample Vista's authored fill relative to its outline.

    Only the inspected, unrotated shield pair is admitted. Relative offsets are
    normalized by the actual page density, never estimated from a screenshot.
    """
    name = "ui\\hud\\bitmaps\\new_hud\\meters\\shield_meter"
    if name not in included:
        return dict(rows=0)
    xb, vb = banks(xbox, name)[name], banks(vista, name)[name]
    rows, evidence = {}, []
    def pair(cache, tag_name):
        tag = next(t for t in cache.tags.values() if t["name"] == tag_name and t["group"] == "nhdt")
        _, at = cache.tag(tag["datum"], "nhdt")
        widgets = cache.block(at + 8, 100)
        fill = [q for q in widgets if cache.u16(q + 96) == 5]
        if len(fill) != 1:
            raise FormatError("ambiguous shield fill owner")
        fill = fill[0]
        mask = [q for q in widgets if cache.u32(q + 36) == cache.u32(fill + 36) and
                cache.u16(q + 96) == 0 and cache.data[q + 48] == cache.data[fill + 48] + 1]
        if len(mask) != 1:
            raise FormatError("ambiguous shield outline owner")
        mask = mask[0]
        for q in (fill, mask):
            if cache.u16(q + 28) != 2 or cache.u16(q + 30) or cache.u32(q + 88) or \
                    any(struct.unpack_from("<6f", cache.data, q + 64)):
                raise FormatError("unsupported shield artwork transform")
        return fill, mask
    for appearance, tag_name in enumerate(("ui\\hud\\masterchief", "ui\\hud\\dervish")):
        xf, xm = pair(xbox, tag_name)
        vf, vm = pair(vista, tag_name)
        for mode in range(3):
            xs, vs = xbox.data[xf + 48 + mode], vista.data[vf + 48 + mode]
            xi, vi = xb[1][xs][appearance][0], vb[1][vs][appearance][0]
            if xi != vi:
                raise FormatError("shield artwork frame correspondence changed")
            width, height = xbox.u16(xb[0][xi] + 4), xbox.u16(xb[0][xi] + 6)
            density = vista.u16(vb[0][vi] + 4) / width
            xfill, xmask = (struct.unpack_from("<2h", xbox.data, q + 52 + mode * 4) for q in (xf, xm))
            vfill, vmask = (struct.unpack_from("<2h", vista.data, q + 52 + mode * 4) for q in (vf, vm))
            shift = tuple(-((f - m) / density - (a - b)) / size
                          for f, m, a, b, size in zip(vfill, vmask, xfill, xmask, (width, height)))
            if xi in rows and rows[xi] != shift:
                raise FormatError("shield artwork alignment differs between split modes")
            rows[xi] = shift
            evidence.append(dict(owner=tag_name, mode=mode, frame=xi, density=density,
                xbox_fill=xfill, xbox_outline=xmask, vista_fill=vfill, vista_outline=vmask,
                uv_shift=shift, xbox_fill_record=xbox.data[xf:xf+100].hex(),
                xbox_outline_record=xbox.data[xm:xm+100].hex(),
                vista_fill_record=vista.data[vf:vf+100].hex(), vista_outline_record=vista.data[vm:vm+100].hex()))
    lines = ["H2UI_HUD_ALIGNMENT_V1"]
    for frame, shift in sorted(rows.items()):
        lines.append("\t".join((name, str(frame), *(format(v, ".9g") for v in shift))))
    (output / "hud-alignment.tsv").write_text("\n".join(lines) + "\n")
    report = dict(rows=len(rows), xbox_sha256=file_sha(xbox.path), vista_sha256=file_sha(vista.path), evidence=evidence)
    (output / "hud-alignment.json").write_text(json.dumps(report, indent=2) + "\n")
    return report


def validate_xbox(root):
    regular(root / "default.xbe")
    inspect_xbe(root / "default.xbe")
    if file_sha(root / "default.xbe") != XBOX_XBE:
        raise FormatError("Xbox source requires the supported original retail default.xbe")
    for name in XBOX_MAPS:
        cache = Cache(regular(root / "maps" / (name + ".map")))
        cache.close()
    table = regular(root / "maps/fonts/font_table.txt")
    names = table.read_text().splitlines()
    if len(names) != 11 or any(not n or "/" in n or "\\" in n or n in (".", "..") for n in names):
        raise FormatError("invalid Xbox font table")
    for name in names:
        regular(table.parent / name)
    for name in ("intro", "attract", "credits"):
        regular(root / "bink" / (name + "_60.bik"))


def inventory(root):
    return {p.relative_to(root).as_posix(): dict(bytes=p.stat().st_size, sha256=file_sha(p))
        for p in sorted(root.rglob("*")) if p.is_file()}


def setup_reference(root, vista_source, xbox_source, progress=print):
    root = root.resolve()
    imports = root / "assets/imported"
    imports.mkdir(parents=True, exist_ok=True)
    identifier = uuid.uuid4().hex
    stage = imports / (".staging-" + identifier)
    stage.mkdir()
    final = imports / identifier
    try:
        progress("Extracting Vista HUD, menu and match audio resources…")
        donor = stage / "vista-donor"
        # The native match opens the selected scenario's original Vista mulg
        # response bank and resolves its sound owners through shared.map.
        # Keeping only mainmenu.map lets the countdown finish, then fails at
        # world activation. Retain the supplied catalog without activating
        # Vista geometry or substituting its gameplay for the Xbox source.
        vista_source_record = extract_vista(Path(vista_source), donor, progress, gameplay=True)
        regular(donor / "maps/lockout.map")
        for path in sorted((donor / "maps").glob("*.map")):
            cache = VistaUiCache(path)
            cache.close()
        progress("Validating the original Xbox game files…")
        source = Path(xbox_source).resolve()
        if source.is_file():
            with Disc(source) as disc:
                disc.extract(stage / "xbox")
            xbox = stage / "xbox"
            xbox_source_record = dict(path=str(source), kind="iso", sha256=file_sha(source))
        else:
            xbox = source.parent if source.name == "maps" else source
            xbox_source_record = dict(path=str(xbox), kind="extracted-directory")
        validate_xbox(xbox)
        ui = stage / "vista"
        ui.mkdir()
        progress("Preparing original Vista loading textures and ambience…")
        loading = prepare_loading(donor, ui / "loading", progress)
        progress("Matching Vista artwork to the existing menu and HUD layouts…")
        with ExitStack() as stack:
            caches = []
            for cls, path in ((Cache, xbox / "maps/mainmenu.map"), (VistaUiCache, donor / "maps/mainmenu.map"),
                              (Cache, xbox / "maps/shared.map"), (VistaUiCache, donor / "maps/shared.map")):
                cache = cls(path)
                stack.callback(cache.close)
                caches.append(cache)
            menu = export_pack(caches[0], caches[1], ui, "pack.tsv", "ui\\")
            hud = export_pack(caches[2], caches[3], ui, "hud.tsv", "ui\\hud\\")
            if hud["banks"]:
                hud["alignment"] = export_hud_alignment(caches[2], caches[3], ui, hud["included"])
                hud["scoreboard"] = export_scoreboard_artwork(caches[2], caches[3], ui)
        donor_maps = {path.name: file_sha(path) for path in sorted((donor / "maps").glob("*.map"))}
        # Move the complete extracted catalog into the committed asset root.
        # Both settings and match audio resolve owners relative to maps/.
        (donor / "maps").rename(ui / "maps")
        shutil.copytree(ui / "maps/fonts", ui / "fonts")
        shutil.rmtree(donor)
        progress("Recording asset provenance…")
        ui_files = inventory(ui)
        xbox_files = {"maps/" + n + ".map": dict(bytes=(xbox / "maps" / (n + ".map")).stat().st_size,
            sha256=file_sha(xbox / "maps" / (n + ".map"))) for n in XBOX_MAPS}
        xbox_files["default.xbe"] = dict(bytes=(xbox / "default.xbe").stat().st_size, sha256=XBOX_XBE)
        # References to an existing extracted directory remain read-only.
        xbox_final = final / "xbox" if xbox == stage / "xbox" else xbox
        report = dict(schema=SCHEMA, complete=True,
            xbox=dict(root=str(xbox_final), source=xbox_source_record, files=xbox_files),
            vista=dict(root=str(final / "vista"), source=vista_source_record, donor_maps=donor_maps,
                       files=ui_files, menu=menu, hud=hud, loading=loading),
            policy=dict(gameplay="xbox", world_models_textures="xbox", ui_pixels_fonts="vista",
                match_announcer_audio="vista",
                ui_layout="existing", shaders="independently implemented Vulkan GLSL; Xbox behavior reference",
                installer_execution=False, vista_shader_binary_import=False))
        (stage / "provenance.json").write_text(json.dumps(report, indent=2) + "\n")
        stage.rename(final)
        # The source registry is the commit point. Interrupted imports never
        # replace or delete a prior working extraction or registry.
        registry = root / "assets/asset-sources.tsv"
        temporary = registry.with_name(".asset-sources-" + identifier)
        paths = (str(xbox_final), str(final / "vista"))
        if any(any(c in path for c in "\t\r\n\0") for path in paths):
            raise FormatError("source paths cannot contain control characters")
        temporary.write_text(f"{SCHEMA}\nxbox\t{paths[0]}\nvista\t{paths[1]}\n")
        os.replace(temporary, registry)
        bundled_ui = root / "resources/ui"
        if (bundled_ui / "bundle.json").is_file():
            from bundle_ui_assets import install
            install(root, bundled_ui, only_unselected=True)
        progress(f"Ready: {menu['banks']} menu image banks and {hud['banks']} HUD image banks.")
        return report
    finally:
        if stage.exists():
            shutil.rmtree(stage)


def repair_loading(root, progress=print):
    """Backfill an older import from its recorded donor, without copying maps/disc."""
    root = Path(root).resolve()
    lines = (root / "assets/asset-sources.tsv").read_text().splitlines()
    if not lines or lines[0] != SCHEMA:
        raise FormatError("Invalid game-file source registry")
    sources = dict(line.split("\t") for line in lines[1:])
    vista = (root / sources["vista"]).resolve()
    # Only our own private import has provenance that can be updated safely.
    if vista.parent.parent != (root / "assets/imported").resolve() or vista.name != "vista":
        raise FormatError("Select the Vista installer again to prepare its loading resources")
    provenance = vista.parent / "provenance.json"
    report = json.loads(provenance.read_text())
    source = Path(report["vista"]["source"]["path"])
    if not source.exists():
        raise FormatError(f"The saved Vista installer is unavailable: {source}")
    with tempfile.TemporaryDirectory(prefix=".loading-", dir=vista) as temporary:
        stage = Path(temporary)
        extract_vista(source, stage / "donor", progress, loading_only=True)
        loading = prepare_loading(stage / "donor", stage / "loading", progress)
        entries = {"loading/" + name: value for name, value in inventory(stage / "loading").items()}
        destination = vista / "loading"
        backup = None
        if destination.exists():
            backup = vista / ("loading.previous-" + uuid.uuid4().hex)
            destination.rename(backup)
        try:
            (stage / "loading").rename(destination)
        except BaseException:
            if backup:
                backup.rename(destination)
            raise
        report["vista"]["loading"] = loading
        report["vista"]["files"].update(entries)
        pending = stage / "provenance.json"
        pending.write_text(json.dumps(report, indent=2) + "\n")
        os.replace(pending, provenance)
    progress("Original Vista loading resources are ready.")


@contextmanager
def _gui_owner(enabled):
    """An EOF-only parent pipe cancels the CLI in its main-thread scope."""
    if not enabled:
        yield
        return
    fd = sys.stdin.fileno()
    stop = threading.Event()
    def watch():
        try:
            while not stop.is_set():
                if select.select([fd], [], [], .05)[0]:
                    # The GUI writes no input. EOF or unexpected input means
                    # ownership ended; all cache cleanup stays in main.
                    os.read(fd, 1)
                    break
        except OSError:
            pass
        if not stop.is_set():
            os.kill(os.getpid(), signal.SIGTERM)
    if select.select([fd], [], [], 0)[0]:
        raise SystemExit(143)
    reader = threading.Thread(target=watch, daemon=True)
    reader.start()
    try:
        yield
    finally:
        stop.set()
        reader.join()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--interactive", action="store_true")
    parser.add_argument("--gui-owner", action="store_true", help=argparse.SUPPRESS)
    parser.add_argument("--mcc-root", type=Path, help="MCC installation or halo2 folder; otherwise scan installed game libraries")
    parser.add_argument("--use-cache", type=Path, help="verify and select an existing private MCC import")
    parser.add_argument("--prepare-mcc-gameplay", action="store_true",
        help="prepare and select the private MCC cache; does not declare incomplete conversion playable")
    args = parser.parse_args()
    if args.gui_owner and args.interactive:
        parser.error("--gui-owner is reserved for the noninteractive setup child")
    if args.use_cache and (args.interactive or args.mcc_root):
        parser.error("--use-cache takes a completed private MCC import by itself")
    if args.interactive:
        try:
            from setup_assets_ui import run
            return run(args.root, args.mcc_root)
        except (ImportError, RuntimeError) as error:
            print(f"Cannot open MCC setup: {error}", file=sys.stderr)
            return 1
    try:
        from mcc_movies import _cancellation
        import mcc_onboarding
        with _cancellation(), _gui_owner(args.gui_owner):
            if args.use_cache:
                mcc_onboarding.select(args.root, args.use_cache)
            else:
                mcc_onboarding.setup(args.root, args.mcc_root)
        return 0
    except (OSError, ValueError, KeyError) as error:
        print(f"MCC import failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
