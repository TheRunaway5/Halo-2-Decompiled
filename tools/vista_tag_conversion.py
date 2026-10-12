#!/usr/bin/env python3
"""Checked, bounded Xbox TU1.5 postprocessing of private Vista cache copies.

This implements the two recovered map-load callees, not a general assertion
that every Vista tag or native engine behavior equals Xbox TU1.5.
"""
from __future__ import annotations

from bisect import bisect_right
from contextlib import ExitStack
import mmap
import struct

from audit_rendering import FormatError, file_sha
from setup_assets import VistaUiCache, VISTA_BUILD

PROFILE = "H2V_TU15_VERIFIED_POSTPROCESS_V1"
TU15_SHA256 = "21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"
# First-encounter order of the original operands at 00165B30. These words are
# reproduced from the owned, fingerprinted executable and its unchanged oracle.
BALANCE_WORDS = (
    0x3AE4C388, 0x42B40000, 0x42F00000, 0x425C0000, 0x42960000,
    0x3F000000, 0x3FE00000, 0x42480000, 0x3FC00000, 0x3F400000,
    0x43200000, 0x3FA66666, 0x43480000, 0x3F666666, 0x40B00000,
    0x3F333333, 0x40940000, 0x41F00000, 0x42700000,
)
NONE = 0xFFFFFFFF


def float_value(bits):
    return struct.unpack("<f", struct.pack("<I", bits))[0]


def float_bits(value):
    return struct.unpack("<I", struct.pack("<f", value))[0]


def rounded(value):
    return float_value(float_bits(value))


class TagGraph:
    """Resolve Vista's resident/local and exact shared-datum metadata.

    References keep their authored IDs. A shared slot must agree on group and
    path as well as datum; low-index guesses and absent-tag fallbacks are errors.
    Every virtual access is confined to one real resident tag allocation.
    """
    def __init__(self, local, shared=None):
        self.local, self.shared = local, shared
        self.tags = {}
        for datum, tag in local.tags.items():
            owner, resident = local, tag
            if not tag["address"] or not tag["size"]:
                resident = shared.tags.get(datum) if shared else None
                if resident and (resident["group"], resident["name"]) != (tag["group"], tag["name"]):
                    raise FormatError(f"Vista shared reference disagrees: {tag['name']}")
                owner = shared
            if resident and resident["address"] and resident["size"]:
                self.tags[datum] = (owner, resident)
        # The local table can omit resident shared entries which are reached by
        # a shared tag. Index all shared tags, retaining exact datum identity.
        if shared:
            for datum, tag in shared.tags.items():
                if datum not in self.tags and tag["address"] and tag["size"]:
                    local_tag = local.tags.get(datum)
                    if local_tag and (local_tag["group"], local_tag["name"]) != (tag["group"], tag["name"]):
                        raise FormatError("Vista shared datum collision")
                    self.tags[datum] = (shared, tag)
        self.regions = sorted(self.tags.values(), key=lambda pair: pair[1]["address"])
        self.starts = [t["address"] for _, t in self.regions]
        for (_, a), (_, b) in zip(self.regions, self.regions[1:]):
            if a["address"] + a["size"] > b["address"]:
                raise FormatError("overlapping Vista resident metadata allocations")
        self.stores = []

    def resolve(self, datum, group):
        result = self.tags.get(datum)
        if not result or result[1]["group"] != group:
            raise FormatError(f"missing Vista {group} reference {datum:08x}")
        return result[1]["address"]

    def location(self, address, size):
        i = bisect_right(self.starts, address) - 1
        if i < 0:
            raise FormatError("Vista virtual access precedes resident metadata")
        owner, tag = self.regions[i]
        if address + size > tag["address"] + tag["size"]:
            raise FormatError(f"Vista virtual access escapes {tag['name']}")
        return owner, tag, owner.pointer(address, owner.magic, size)

    def u32(self, address):
        owner, _, at = self.location(address, 4)
        return owner.u32(at)

    def u16(self, address):
        owner, _, at = self.location(address, 2)
        return owner.u16(at)

    def store(self, address, bits, instruction):
        owner, tag, at = self.location(address, 4)
        before = owner.u32(at)
        self.stores.append(dict(map=owner.path.name, datum=f"{tag['datum']:08x}",
            group=tag["group"], tag=tag["name"], address=address, file_offset=at,
            tag_offset=address-tag["address"], before=f"{before:08x}", after=f"{bits:08x}",
            instruction=f"{instruction:08x}"))
        struct.pack_into("<I", owner.data, at, bits)


def apply_combat(graph, scenario, globals_address):
    """00165B30: same walks, NONE gates and ordered DWORD stores as Xbox."""
    if graph.u16(scenario + 0x10) != 1:
        return
    mulg = graph.resolve(graph.u32(globals_address + 0x16C), "mulg")
    runtime = graph.u32(mulg + 0xC)
    # The executable assumes these authored blocks contain the selected slots.
    # Validate the host input rather than reading into another allocation.
    if graph.u32(mulg + 8) == 0 or graph.u32(runtime + 0x68) < 13 or graph.u32(runtime + 0x70) < 8:
        raise FormatError("Vista multiplayer runtime lacks original tuning slots")

    def weapon(slot):
        datum = graph.u32(graph.u32(runtime + 0x6C) + slot * 8 + 4)
        return None if datum == NONE else graph.resolve(datum, "weap")

    def store(address, value, site):
        graph.store(address, BALANCE_WORDS[value], site + 0x13E0)

    def bounds(damage, values, sites):
        if damage == NONE:
            return
        address = graph.resolve(damage, "jpt!")
        for i, (value, site) in enumerate(zip(values, sites)):
            store(address + 0x1C + i * 4, value, site)

    def melee(slot, values, sites):
        address = weapon(slot)
        if address is not None:
            bounds(graph.u32(address + 0x1C0), values, sites)

    def projectile(slot):
        address = weapon(slot)
        if address is None:
            return None
        datum = graph.u32(graph.u32(address + 0x2D4) + 0x90)
        return None if datum == NONE else graph.resolve(datum, "proj")

    address = weapon(3)
    if address is not None and 0 < graph.u32(address + 0x2D0) < 0x80000000:
        barrel = graph.u32(address + 0x2D4)
        store(barrel + 0x74, 0, 0x1647B4)
        store(barrel + 0x78, 0, 0x1647B9)
    melee(12, (1, 2, 2), (0x1647FE, 0x164803, 0x164808))
    melee(5, (3, 1, 1), (0x164845, 0x16484A, 0x16484F))
    melee(1, (3, 4, 4), (0x16488C, 0x164891, 0x164896))

    if graph.u32(globals_address + 0x100) < 2:
        raise FormatError("Vista globals lacks the two original grenade entries")
    grenades = graph.u32(globals_address + 0x104)
    for slot, upper, sites, arming in (
        (0, 10, (0x16490C, 0x164910, 0x164915, 0x16491A, 0x16491F, 0x164924, 0x164929, 0x16492E), 0x164964),
        (1, 12, (0x1649B8, 0x1649BC, 0x1649C1, 0x1649C6, 0x1649CB, 0x1649D0, 0x1649D5, 0x1649DA), 0x164A13),
    ):
        datum = graph.u32(grenades + slot * 0x2C + 0x28)
        if datum == NONE:
            continue
        proj = graph.resolve(datum, "proj")
        damage = graph.u32(proj + 0x104)
        if damage != NONE:
            address = graph.resolve(damage, "jpt!")
            for field, value, site in zip((0, 4, 0x1C, 0x20, 0x24, 0x44, 0x48, 0x4C),
                                         (5, 6, 7, upper, upper, 8, 9, 9), sites):
                store(address + field, value, site)
        store(proj + 0xCC, 11, arming)
        if slot:
            store(proj + 0xD4, 13, 0x164A1B)
            store(proj + 0xD8, 13, 0x164A23)

    proj = projectile(0)
    if proj is not None:
        bounds(graph.u32(proj + 0x14C), (14, 14, 14), (0x164A81, 0x164A86, 0x164A8B))
    address = weapon(7)
    if address is not None:
        store(graph.u32(address + 0x2D4) + 0x64, 15, 0x164AB6)
    proj = projectile(1)
    if proj is not None:
        damage = graph.u32(proj + 0x14C)
        if damage != NONE:
            address = graph.resolve(damage, "jpt!")
            store(address + 0x20, 16, 0x164B11)
            store(address + 0x24, 16, 0x164B16)
    proj = projectile(12)
    if proj is not None:
        bounds(graph.u32(proj + 0x104), (17, 18, 18), (0x164B71, 0x164B7E, 0x164B83))
    datum = graph.u32(graph.u32(runtime + 0x74) + 7 * 8 + 4)
    if datum != NONE:
        store(graph.resolve(datum, "vehi") + 4, 5, 0x164BA1)


def apply_netgame(graph, scenario):
    """0011F060: bounded original Beaver Creek / Relic flag corrections."""
    if graph.u16(scenario + 0x10) != 1:
        return None
    map_id = graph.u32(graph.u32(graph.u32(scenario + 0x39C) + 0x14))
    if map_id not in (100, 1200):
        return map_id
    if graph.u32(scenario + 0x118) < (7 if map_id == 100 else 4):
        raise FormatError("Vista scenario lacks original netgame correction rows")
    flags = graph.u32(scenario + 0x11C)
    targets = ((5, (0x4193271E, 0xBD4A4350, 0x3E19999A)),
               (6, (0xC1838AA6, 0xBD8416CB, 0x3E19999A))) if map_id == 100 else (
               (2, (0x425BC1BE, 0xC33ADAA0, 0xBFABFB62)),
               (3, (0x4280C8CE, 0xC3070000, 0xC00A38B4)))
    for index, target in targets:
        at = flags + index * 0x20
        if graph.u16(at + 0x10) != (8 if map_id == 100 else 7):
            continue
        xyz = [graph.u32(at + i * 4) for i in range(3)]
        d = [rounded(float_value(a) - float_value(b)) for a, b in zip(target, xyz)]
        squares = [rounded(a * a) for a in d]
        order = (1, 0, 2) if map_id == 100 else (2, 1, 0)
        distance = rounded(rounded(squares[order[0]] + squares[order[1]]) + squares[order[2]])
        if not float_value(0x38D1B717) > distance:
            continue
        if map_id == 100 and index == 5:
            graph.store(at + 4, xyz[1], 0x11F11A)
            graph.store(at, float_bits(float_value(xyz[0]) + float_value(0x3F400000)), 0x11F124)
            graph.store(at + 8, xyz[2], 0x11F128)
        elif map_id == 100:
            graph.store(at, float_bits(float_value(xyz[0]) - float_value(0x3F400000)), 0x11F192)
            graph.store(at + 4, xyz[1], 0x11F196)
            graph.store(at + 8, xyz[2], 0x11F199)
        else:
            graph.store(at, float_bits(0.0 + float_value(xyz[0])), 0x11F296)
            graph.store(at + 4, float_bits(0.0 + float_value(xyz[1])), 0x11F2AB)
            graph.store(at + 8, float_bits(float_value(xyz[2]) - float_value(0x3DCCCCCD)), 0x11F2BD)
    return map_id


def convert_maps(maps, progress=print):
    """Patch an already private extraction; callers provide the transaction."""
    paths = sorted(maps.glob("*.map"))
    if not paths or any(p.is_symlink() for p in paths):
        raise FormatError("missing regular Vista gameplay caches")
    before = {p.name: file_sha(p) for p in paths}
    results = []
    with ExitStack() as stack:
        caches = {}
        for path in paths:
            cache = VistaUiCache(path)
            stack.callback(cache.close)
            # Writes use ACCESS_COPY: original input bytes stay unchanged until
            # all reference walks in the complete import have passed validation.
            cache.data.close()
            cache.data = mmap.mmap(cache.file.fileno(), 0, access=mmap.ACCESS_COPY)
            caches[path.name] = cache
        shared = caches.get("shared.map")
        if shared is None:
            raise FormatError("Vista shared.map is required for multiplayer tags")
        for name, cache in caches.items():
            scenario_id = cache.u32(cache.u32(16) + 12)
            # Resource maps are not played scenarios, even when their directory
            # retains a synthetic multiplayer scenario/global root.
            if name in ("shared.map", "single_player_shared.map"):
                results.append(dict(map=name, role="resources", stores=[]))
                continue
            graph = TagGraph(cache, shared if cache.u32(32) else None)
            scenario = graph.resolve(scenario_id, "scnr")
            multiplayer = graph.u16(scenario + 0x10) == 1
            if multiplayer:
                progress(f"Applying verified TU1.5 postprocessing: {name}")
                map_id = apply_netgame(graph, scenario)
                globals_id = cache.u32(cache.u32(16) + 16)
                apply_combat(graph, scenario, graph.resolve(globals_id, "matg"))
            else:
                map_id = None
            results.append(dict(map=name, role="multiplayer" if multiplayer else "campaign-or-menu",
                map_id=map_id, resident_tags=len(graph.tags), stores=graph.stores))
        writes = {}
        for result in results:
            for row in result["stores"]:
                writes.setdefault(row["map"], set()).add(row["file_offset"])
        for name, offsets in writes.items():
            cache = caches[name]
            with cache.path.open("r+b") as output:
                for at in sorted(offsets):
                    output.seek(at)
                    output.write(cache.data[at:at+4])
    after = {p.name: file_sha(p) for p in paths}
    return dict(profile=PROFILE, vista_build=VISTA_BUILD, source_xbe_sha256=TU15_SHA256,
        original_routines=["0011f060", "00165b30"], full_tu15_parity=False,
        coverage="recovered multiplayer netgame and combat postprocessors only",
        maps=[dict(**r, source_sha256=before[r["map"]], output_sha256=after[r["map"]]) for r in results],
        unresolved=["complete Vista versus Xbox TU1.5 semantic tag audit",
                    "earlier original map-loader and tag-group fixups",
                    "remaining Vista animation-owner, audio and other resource-consumer adapters",
                    "independently implemented Xbox material behavior in Vulkan"])
