#!/usr/bin/env python3
"""Inventory authored multiplayer world features from the supplied Xbox caches.

This is a requirements inventory, not a claim that the native runtime executes
the features. All maps and BSPs are required; a missing cache is an error.
"""
import argparse
import collections
import json
from pathlib import Path
import struct

from audit_rendering import Cache, FormatError, file_sha

ROOT = Path(__file__).resolve().parents[1]
DISC = "ascension beavercreek burial_mounds coagulation colossus cyclotron foundation headlong lockout midship waterworks zanzibar".split()
PACK = "backwash containment deltatap dune elongation gemini triplicate turf warlock".split()
NONE = 0xffffffff


def inspect(path):
    c = Cache(path)
    try:
        _, s = c.tag(c.scenario, "scnr")

        def signed(at):
            return struct.unpack_from("<h", c.data, at)[0]

        def floats(at, count):
            c.require(at, count * 4)
            return list(struct.unpack_from("<" + "f" * count, c.data, at))

        def reference(datum):
            if datum == NONE:
                return None
            t = c.tags[datum]
            return {"datum": f"{datum:08x}", "group": t["group"], "name": t["name"]}

        groups = [{"index": i, "file_offset": p, "name": c.string(p, 32),
                   "initial_value": floats(p + 32, 1)[0], "flags": c.u32(p + 36)}
                  for i, p in enumerate(c.block(s + 0xa0, 40))]
        names = [c.string(p, 32) for p in c.block(s + 0x48, 36)]
        placements = []
        # scnr placement/palette blocks; stride is independent of object tag size.
        for kind, off, stride, group in ((6, 0x50, 0x5c, "scen"),
                                        (7, 0xa8, 0x48, "mach"),
                                        (8, 0xb8, 0x44, "ctrl"),
                                        (10, 0xd8, 0x50, "ssce"),
                                        (11, 0x328, 0x4c, "bloc")):
            palette = c.block(s + off + 8, 40)
            for ordinal, p in enumerate(c.block(s + off, stride)):
                index = signed(p)
                if index < 0 or index >= len(palette):
                    raise FormatError("world placement palette index is invalid")
                datum = c.u32(palette[index] + 4)
                tag, d = c.tag(datum, group)
                name = signed(p + 2)
                if name < -1 or name >= len(names):
                    raise FormatError("world placement name index is invalid")
                row = {"type": kind, "placement": ordinal, "file_offset": p,
                       "definition": reference(datum), "name": names[name] if name >= 0 else None,
                       "placement_flags": c.u32(p + 4), "position": floats(p + 8, 3),
                       "rotation": floats(p + 0x14, 3), "origin_bsp": signed(p + 0x2c),
                       "creation_effect": reference(c.u32(d + 0x50)),
                       "attachments": []}
                for a in c.block(d + 0x94, 24):
                    row["attachments"].append({"file_offset": a,
                        "definition": reference(c.u32(a + 4)), "marker": c.u32(a + 8)})
                if kind in (7, 8):
                    for field, at in (("power_group", 0x34), ("position_group", 0x36)):
                        value = signed(p + at)
                        if value < -1 or value >= len(groups):
                            raise FormatError("world device group index is invalid")
                        row[field] = value
                    row.update(device_flags=c.u32(p + 0x38), type_flags=c.u32(p + 0x3c),
                               device_type=c.u16(d + 0x11c),
                               transition=floats(d + 0xc0, 6),
                               delay_seconds=floats(d + 0x10c, 1)[0],
                               activation_radius=floats(d + 0x118, 1)[0])
                    if kind == 8:
                        row.update(triggers_when=c.u16(d + 0x11e),
                                   call_value=floats(d + 0x120, 1)[0],
                                   action_string=c.u32(d + 0x124),
                                   on=reference(c.u32(d + 0x12c)),
                                   off=reference(c.u32(d + 0x134)),
                                   deny=reference(c.u32(d + 0x13c)))
                placements.append(row)

        bsps = []
        for ordinal, p in enumerate(c.block(s + 0x210, 68)):
            at, size, memory = (c.u32(p + j * 4) for j in range(3))
            c.require(at, size)
            b, magic = at + 16, memory - at
            bounds = (at, at + size)
            clusters = c.block(b + 0x9c, 0xb0, magic, bounds)
            palette = []
            for i, w in enumerate(c.block(b + 0x84, 0x88, magic, bounds)):
                datum = c.u32(w + 0x24)
                entry = {"index": i, "name": c.string(w, 32),
                         "definition": reference(datum),
                         "clusters": [j for j, q in enumerate(clusters) if signed(q + 0x76) == i],
                         "particles": [], "plates": []}
                if datum != NONE:
                    _, d = c.tag(datum, "weat")
                    for system, q in enumerate(c.block(d, 0x8c)):
                        entry["particles"].append({"index": system, "type": c.u16(q + 0x6c),
                            "maximum_particles": c.u32(q + 0x2c),
                            "sprite": reference(c.u32(q + 4)), "file_offset": q})
                    for plate, q in enumerate(c.block(d + 8, 0x3a8)):
                        entry["plates"].append({"index": plate, "file_offset": q,
                            "textures": [reference(c.u32(q + j * 8 + 4)) for j in range(3)]})
                palette.append(entry)
            bsps.append({"index": ordinal, "clusters": len(clusters), "weather": palette,
                         "weather_exclusion_polyhedra": len(c.block(b + 0x8c, 24, magic, bounds))})

        nodes = [struct.unpack_from("<4H3I", c.data, p) for p in c.block(s + 0x238, 20)]
        text = c.block(s + 0x1b0, 1)
        strings = bytes(c.data[text[0]:text[-1] + 1]) if text else b""

        def node(datum):
            if datum == NONE or (datum & 65535) >= len(nodes):
                raise FormatError("script datum is outside syntax bank")
            n = nodes[datum & 65535]
            if n[0] != datum >> 16 or n[3] == 0xbaba:
                raise FormatError("script datum generation is invalid")
            return n

        def script_calls(datum, active):
            if datum in active:
                raise FormatError("cycle in compiled script expression")
            n = node(datum)
            if n[3] & 1:
                return []
            fn = node(n[6])
            end = strings.find(b"\0", fn[5])
            if fn[5] >= len(strings) or end < fn[5]:
                raise FormatError("script function name is outside string bank")
            result = [strings[fn[5]:end].decode("utf8")]
            child, seen = fn[4], set()
            while child != NONE:
                if child in seen:
                    raise FormatError("cycle in script argument chain")
                seen.add(child)
                result.extend(script_calls(child, active | {datum}))
                child = node(child)[4]
            return result

        scripts = [{"name": c.string(p, 32), "type": c.u16(p + 32),
                    "calls": dict(collections.Counter(script_calls(c.u32(p + 36), set())))}
                   for p in c.block(s + 0x1b8, 40)]
        flags = [{"index": i, "position": floats(p, 3), "type": signed(p + 0x10),
                  "channel": signed(p + 0x14)} for i, p in enumerate(c.block(s + 0x118, 32))]
        control_links = []
        for control in (r for r in placements if r["type"] == 8):
            group = control["position_group"]
            control_links.append({"control": control["placement"], "position_group": group,
                "group": groups[group] if group >= 0 else None,
                "machines": [{"placement": r["placement"], "field": field,
                              "definition": r["definition"]}
                             for r in placements if r["type"] == 7
                             for field in ("position_group", "power_group")
                             if group >= 0 and r[field] == group]})
        return {"map": path.stem, "cache": str(path.relative_to(ROOT)),
                "cache_sha256": file_sha(path), "bsps": bsps, "device_groups": groups,
                "placements": placements, "control_links": control_links,
                "scripts": scripts, "netgame_flags": flags,
                "trigger_volumes": len(c.block(s + 0x108, 0x44))}
    finally:
        c.close()


def inspect_vista_only(path):
    # These two caches have no supplied Xbox counterpart. Preserve that
    # boundary instead of treating the21-map Xbox inventory as all PC maps.
    from inspect_pc_combat_tag_sample import PcCache
    c = PcCache(path)

    def block(at, stride):
        count = c.u(at)
        if count > 65536:
            raise FormatError("excessive Vista feature block")
        start = c.u(at + 4) - c.magic
        if count and not 0 <= start <= start + count * stride <= len(c.data):
            raise FormatError("Vista feature block escapes its cache")
        return range(start, start + count * stride, stride) if count else []

    def tag(datum, group):
        t = c.tags[datum & 65535]
        if t["datum"] != f"{datum:08x}" or t["group"] != group or t["offset"] is None:
            raise FormatError("Vista feature metadata has no local owner")
        return t["offset"]

    s = tag(c.u(c.index + 12), "scnr")
    weather = []
    for i, p in enumerate(block(s + 0x210, 68)):
        b = tag(c.u(p + 0x14), "sbsp")
        weather.append({"bsp": i, "palette_count": len(block(b + 0x84, 0x88)),
                        "exclusion_polyhedra": len(block(b + 0x8c, 24))})
    return {"map": path.stem, "cache_sha256": file_sha(path),
            "machines": len(block(s + 0xa8, 0x48)),
            "controls": len(block(s + 0xb8, 0x44)),
            "scripts": len(block(s + 0x1b8, 40)), "weather": weather,
            "scope": "Vista-only scenario/BSP counts; no supplied Xbox counterpart. Full interaction behavior is unverified."}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path,
                        default=ROOT / "analysis/map-world-features/inventory.json")
    args = parser.parse_args()
    maps = [inspect(ROOT / "assets" / directory / "maps" / (name + ".map"))
            for directory, names in (("disc", DISC), ("map-pack", PACK)) for name in names]
    vista_only = [inspect_vista_only(path) for name in ("needle", "street_sweeper")
                  if (path := ROOT / "Halo 2 Vista installer/maps" / (name + ".map")).exists()]
    report = {"scope": "All 21 supplied Xbox multiplayer caches, all authored BSPs. Metadata only; live completeness is unverified.",
              "live_feature_completeness_verified": False, "maps": maps,
              "additional_vista_only_maps": vista_only}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2, allow_nan=False) + "\n")
    for m in maps:
        weather = sum(len(b["weather"]) for b in m["bsps"])
        kinds = collections.Counter(r["type"] for r in m["placements"])
        attached = sum(len(r["attachments"]) for r in m["placements"])
        print(f'{m["map"]}: weather palettes={weather}, machines={kinds[7]}, controls={kinds[8]}, attachments={attached}, scripts={len(m["scripts"])}')
    print(f'Wrote {len(maps)} maps to {args.output}')


if __name__ == "__main__":
    main()
