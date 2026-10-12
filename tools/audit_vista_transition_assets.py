#!/usr/bin/env python3
"""Pin actual supplied PC PCR16/185 and loading209 2D definitions/resources.

This independently reads the source map; it does not call the native importer.
It proves retained assets, not a live route, PC emblem pass or visual parity.
"""
from pathlib import Path
import argparse
import hashlib
import json
import struct

from export_vista_menu_ui import VistaCache, expand_resource, mip_shapes

ROOT = Path(__file__).resolve().parents[1]
MAP_SHA = "eb259c207da26adc1820ae0757d23efb4ce1d2c2018956635d1094c8364f6b44"


def sha(data):
    return hashlib.sha256(data).hexdigest()


def float_word(value):
    return struct.unpack("<I", struct.pack("<f", value))[0]


class Inventory:
    def __init__(self, cache):
        self.c = cache
        self.images, self.groups = set(), set()
        self.shaders = {}
        matg = next(t for t in cache.tags.values() if t["group"] == "matg")
        self.matg = cache.tag(matg["datum"], "matg")[1]
        wigl = next(t for t in cache.tags.values() if t["group"] == "wigl")
        self.wigl = cache.tag(wigl["datum"], "wigl")[1]
        at = self.matg + 0x190
        count, size, indices, strings = [cache.u32(at + i) for i in (0, 4, 8, 12)]
        self.locale = [(cache.u32(indices + i * 8), cache.string(
            strings + cache.u32(indices + i * 8 + 4), size - cache.u32(indices + i * 8 + 4)))
            for i in range(count)]

    def signed(self, at):
        return struct.unpack("<h", struct.pack("<H", self.c.u16(at)))[0]

    def point(self, at):
        return [float_word(self.signed(at + i)) for i in (0, 2)]

    def rect(self, at):
        return [float_word(self.signed(at + i)) for i in (0, 2, 4, 6)]

    def words(self, at, count):
        return [self.c.u32(at + 4 * i) for i in range(count)]

    def image_ref(self, datum):
        if datum != 0xFFFFFFFF:
            self.c.tag(datum, "bitm")
            self.images.add(datum)
        return datum

    def strings(self, datum):
        at = self.c.tag(datum, "unic")[1]
        first, count = self.c.u16(at + 16), self.c.u16(at + 18)
        assert first + count <= len(self.locale)
        return {str(key): value for key, value in self.locale[first:first + count]}

    def text(self, at):
        c = self.c
        return dict(flags=c.u32(at), animation=c.u16(at + 4), delay=c.u16(at + 6),
            font=self.signed(at + 10), color=self.words(at + 12, 4), bounds=self.rect(at + 28),
            string=c.u32(at + 36), layer=self.signed(at + 40))

    def bitmap(self, at):
        c = self.c
        return dict(flags=c.u32(at), animation=c.u16(at + 4), delay=c.u16(at + 6),
            blend=c.u16(at + 8), frame=self.signed(at + 10), position=self.point(at + 12),
            scroll=self.words(at + 16, 2), datum=self.image_ref(c.u32(at + 28)),
            layer=self.signed(at + 32), fps=c.u32(at + 36), placeholder=c.u32(at + 44),
            scale=self.words(at + 48, 2))

    def grid(self, at):
        c = self.c
        skin = c.u32(at + 8)
        if skin != 0xFFFFFFFF:
            c.tag(skin, "skin")
            self.groups.add(skin)
        return dict(skin=skin, position=self.point(at + 12), order=c.data[at + 16],
            count=c.data[at + 17], rows=c.data[at + 18], columns=c.data[at + 19],
            step=[float_word(self.signed(at + i)) for i in (22, 20)])

    def emblem(self, at):
        c = self.c
        shader = c.u32(at + 24)
        if shader != 0xFFFFFFFF:
            tag, p = c.tag(shader, "shad")
            definitions = []
            for q in c.block(p + 32, 124):
                stem, t = c.tag(c.u32(q), "stem")
                passes = []
                for pp in c.block(t + 88, 40):
                    for entry in c.block(pp + 16, 10):
                        pass_tag, _ = c.tag(c.u32(entry + 4), "spas")
                        passes.append(dict(datum=pass_tag["datum"], name=pass_tag["name"]))
                definitions.append(dict(template=stem["datum"], template_name=stem["name"],
                    bitmaps=[self.image_ref(c.u32(ref)) for ref in c.block(q + 4, 12)], passes=passes))
            self.shaders[str(shader)] = dict(name=tag["name"], definitions=definitions,
                rendered=False, reason="PC spas native emblem pass remains unresolved")
        return dict(flags=c.u32(at), animation=c.u16(at + 4), delay=c.u16(at + 6),
            layer=self.signed(at + 8), page=self.signed(at + 10),
            bitmap=self.image_ref(c.u32(at + 16)), shader=shader, bounds=self.rect(at + 28))

    def track(self, milliseconds, at):
        return dict(milliseconds=milliseconds,
            keys=[dict(alpha=self.c.u32(q + 4), position=self.words(q + 8, 3))
                  for q in self.c.block(at, 20)])

    def animation(self, at):
        return dict(intro=self.track(self.c.u32(at + 4), at + 8),
            outro=self.track(self.c.u32(at + 16), at + 20),
            ambient=self.track(self.c.u32(at + 28), at + 36), loop=self.signed(at + 32))

    def list(self, at):
        c = self.c
        return dict(flags=c.u32(at), skin=c.u16(at + 4), visible=c.u16(at + 6),
            position=self.point(at + 8), animation=c.u16(at + 12), delay=c.u16(at + 14),
            labels=[c.u32(q + 16) for q in c.block(at + 16, 20)])

    def pane(self, at):
        c = self.c
        assert c.u32(at + 4) == 0 and c.u32(at + 44) == 0
        return dict(animation=c.u16(at + 2), buttons=[], scenes=[],
            lists=[self.list(q) for q in c.block(at + 12, 24)],
            texts=[self.text(q) for q in c.block(at + 28, 44)],
            bitmaps=[self.bitmap(q) for q in c.block(at + 36, 56)],
            emblems=[self.emblem(q) for q in c.block(at + 60, 36)],
            grids=[self.grid(q) for q in c.block(at + 68, 24)])

    def screen(self, datum):
        c = self.c
        tag, at = c.tag(datum, "wgit")
        return dict(name=tag["name"], id=c.u16(at + 4), flags=c.u32(at), button_key=c.u16(at + 6),
            color=self.words(at + 8, 4), strings=self.strings(c.u32(at + 28)),
            shape=c.u16(at + 40), header=c.u32(at + 44),
            panes=[self.pane(q) for q in c.block(at + 32, 76)],
            bitmap_resources=[self.image_ref(c.u32(q + 4)) for q in c.block(at + 56, 8)])

    def skin(self, datum):
        c = self.c
        tag, at = c.tag(datum, "skin")
        return dict(name=tag["name"], arrows=self.image_ref(c.u32(at + 8)),
            arrow_up=self.point(at + 12), arrow_down=self.point(at + 16),
            animations=[self.track(c.u32(q + 4), q + 8) for q in c.block(at + 20, 16)],
            texts=[self.text(q) for q in c.block(at + 28, 44)],
            bitmaps=[self.bitmap(q) for q in c.block(at + 36, 56)],
            emblems=[self.emblem(q) for q in c.block(at + 44, 36)],
            grids=[self.grid(q) for q in c.block(at + 52, 24)])

    def image(self, datum):
        c = self.c
        tag, at = c.tag(datum, "bitm")
        frames = []
        for q in c.block(at + 0x44, 0x74):
            width, height, fmt, additional = [c.u16(q + i) for i in (4, 6, 12, 20)]
            assert c.u16(q + 10) == 0 and c.data[q + 8] == 1 and not c.u16(q + 14) & 8
            source_mips = mip_shapes(width, height, additional, fmt)
            expanded = expand_resource(c.data[c.u32(q + 28):c.u32(q + 28) + c.u32(q + 52)],
                sum(row["bytes"] for row in source_mips))
            prepared, mips = bytearray(), []
            for level in source_mips:
                begin, size = level["offset"], level["bytes"]
                raw = expanded[begin:begin + size]
                pixels = bytearray()
                if fmt in (14, 15):
                    pixels.extend(raw)
                elif fmt == 1:
                    for y in raw:
                        pixels.extend((y, y, y, 255))
                elif fmt == 3:
                    for y, a in struct.iter_unpack("BB", raw):
                        pixels.extend((y, y, y, a))
                elif fmt == 9:
                    for word, in struct.iter_unpack("<H", raw):
                        pixels.extend(((word >> 8 & 15) * 17, (word >> 4 & 15) * 17,
                            (word & 15) * 17, (word >> 12) * 17))
                else:
                    assert fmt in (10, 11)
                    for b, g, r, a in struct.iter_unpack("BBBB", raw):
                        pixels.extend((r, g, b, a if fmt == 11 else 255))
                mips.append(dict(width=level["width"], height=level["height"], offset=len(prepared),
                    size=len(pixels), depth=1))
                prepared.extend(pixels)
            frames.append(dict(width=width, height=height, format=fmt if fmt in (14, 15) else 11,
                layers=1, depth=0, mips=mips, pixels_bytes=len(prepared), pixels_sha256=sha(prepared)))
        sequences = []
        for q in c.block(at + 0x3c, 60):
            sprites = []
            for s in c.block(q + 52, 32):
                sprites.append(dict(image=c.u16(s), uv=self.words(s + 8, 4), registration=self.words(s + 24, 2)))
            if not sprites:
                for index in range(c.u16(q + 32), c.u16(q + 32) + c.u16(q + 34)):
                    sprites.append(dict(image=index, uv=[0, 0x3F800000, 0, 0x3F800000], registration=[0, 0]))
            sequences.append(sprites)
        return dict(name=tag["name"], images=frames, sequences=sequences)

    def result(self):
        c, g = self.c, self.wigl
        # PC5B4DB6: matg+16C -> mulg universal[0]+10 RGB block.
        mulg = c.u32(self.matg + 0x16C)
        mulg_at = c.tag(mulg, "mulg")[1]
        universal = c.pointer(c.u32(mulg_at + 4), c.magic, 0x18)
        # PC60BB1D ->60C1A9: actual primary scenario type selects the
        # interface wgtz; its +4 reference owns wigl+158 signed ranges.
        scenario_at = c.tag(c.scenario, "scnr")[1]
        scenario_type = self.signed(scenario_at + 0x10)
        assert 0 <= scenario_type <= 2 and c.u32(self.matg + 0x110)
        interface = c.pointer(c.u32(self.matg + 0x114), c.magic, 0x98)
        wgtz = c.u32(interface + (0x8C, 0x94, 0x84)[scenario_type])
        wgtz_at = c.tag(wgtz, "wgtz")[1]
        rank_wigl = c.u32(wgtz_at + 4)
        rank_at = c.tag(rank_wigl, "wigl")[1]
        pcr_appearance = dict(team_palette=[self.words(at, 3) for at in c.block(universal + 0x10, 12)],
            rank_owner_present=True,
            rank_ranges=[[self.signed(at), self.signed(at + 2)] for at in c.block(rank_at + 0x158, 4)],
            source_owners=dict(scenario=c.scenario, scenario_type=scenario_type, mulg=mulg,
                wgtz=wgtz, rank_wigl=rank_wigl))
        definitions = {str(d): self.screen(d) for d in (0xE5B80396, 0xE5BE039C, 0xE6230401)}
        indices = sorted({row["skin"] for screen in definitions.values()
            for pane in screen["panes"] for row in pane["lists"]})
        refs = c.block(g + 0x138, 8)
        roots = {str(index): c.u32(refs[index] + 4) for index in indices}
        groups = {str(d): self.skin(d) for d in roots.values()}
        while self.groups:
            d = self.groups.pop()
            if str(d) not in groups:
                groups[str(d)] = self.skin(d)
        return dict(schema="H2V_TRANSITION_UI_ASSETS_V1", source_map_sha256=sha(c.data),
            generator_sha256=sha(Path(__file__).read_bytes()),
            definitions=definitions, root_skin_datums=roots, skin_groups=groups,
            images={str(d): self.image(d) for d in sorted(self.images)}, emblem_shaders=self.shaders,
            palette=[self.words(at, 3) for at in c.block(self.matg + 0x160, 12)],
            pcr_appearance=pcr_appearance,
            header_fonts=[self.signed(g + 0x160 + i * 2) for i in range(4)],
            header_bounds=[self.rect(g + 0x178 + i * 8) for i in range(8)],
            color=self.words(g + 0x168, 4), overlay=self.words(g + 0x6C, 4),
            typewriter=[self.signed(g + 0x48), c.u16(g + 0x4A), c.u32(g + 0x4C)],
            animations=[self.animation(at) for at in c.block(g + 0x120, 44)],
            limits=["Assets and exact retained fields only; not a live route or full visual parity.",
                "No PC3D scene, Xbox world/camera/material mutation or substitute emblem pass.",
                "Source is the supplied Project Cartographer Vista distribution."])


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--map", type=Path, default=ROOT / "Halo 2 Vista installer/maps/mainmenu.map")
    p.add_argument("--fixture", type=Path, default=ROOT / "tests/fixtures/vista_transition_assets.json")
    args = p.parse_args()
    c = VistaCache(args.map)
    try:
        assert sha(c.data) == MAP_SHA, "supplied PC map SHA256 changed"
        result = Inventory(c).result()
    finally:
        c.close()
    args.fixture.parent.mkdir(parents=True, exist_ok=True)
    args.fixture.write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(dict(fixture_sha256=sha(args.fixture.read_bytes()), screens=len(result["definitions"]),
        root_skins=len(result["root_skin_datums"]), skin_groups=len(result["skin_groups"]),
        bitmap_banks=len(result["images"]), frames=sum(len(t["images"]) for t in result["images"].values()))))


if __name__ == "__main__":
    main()
