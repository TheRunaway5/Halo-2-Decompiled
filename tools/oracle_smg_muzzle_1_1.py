#!/usr/bin/env python3
"""Original authored muzzle-marker and supporting-platform request oracle.

Current animation node matrices, unit state and platform physics records are
supplied boundaries. Authored marker/definition bytes and executed code are
unchanged. This does not recover complete pose animation or real world queries.
"""
import argparse
import csv
import hashlib
import json
import struct
from pathlib import Path

from oracle_smg_projectile_1_1 import AllocationTrace
from oracle_smg_firing_1_1 import (
    ROOT, PIN, CACHE_PIN, Cache, WEAPON, UNIT, HEADERS, TAGS, POOL, BITMAP,
    UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EBX, UC_X86_REG_ESP,
    Xbe, _Code, _trace,
)

PLATFORM_INDEX = 0x10003
PLATFORM_OBJECT = 0x10E8000
PLATFORM_DEF = 0x10E7000
PHYSICS = 0x10F0000
TURF = ROOT / "assets/multiplayer-map-pack/demos/dlc/00000002/turf.map"
TURF_PIN = "3000571be44bde1a62e2738b3a34b482c192772956631d555d8142e7a6d0e446"


def authored():
    cache = Cache(ROOT / "assets/disc/maps/lockout.map")
    weapon = next(t for t in cache.tags.values() if t["name"] == r"objects\weapons\rifle\smg\smg")
    model = cache.tags[cache.u(weapon["offset"] + 0x38)]
    render = cache.tags[cache.u(model["offset"] + 4)]
    group = next(offset for offset in cache.block(render["offset"] + 0x58, 0xC)
                 if cache.u(offset) == 0x0F0000DB)
    records = cache.block(group + 4, 0x24)
    spartan = next(t for t in cache.tags.values() if t["name"] == r"objects\characters\masterchief\masterchief_mp")
    result = {"render_model": render, "marker_string_id": "0f0000db",
              "marker_group_file_offset": group,
              "marker_records": [{"file_offset": offset,
                                  "raw_hex": bytes(cache.data[offset:offset + 0x24]).hex(),
                                  "node": cache.data[offset + 2],
                                  "local_translation_bits": [f"{v:08x}" for v in struct.unpack_from("<3I", cache.data, offset + 4)],
                                  "local_translation": list(struct.unpack_from("<3f", cache.data, offset + 4))}
                                 for offset in records],
              "spartan": spartan,
              "eye_height_bits": [f"{cache.u(spartan['offset'] + offset):08x}" for offset in (0x218, 0x21C)]}
    cache.data.close()
    cache.stream.close()
    cache = Cache(TURF)
    assert hashlib.sha256(cache.data).hexdigest() == TURF_PIN
    tag = next(t for t in cache.tags.values() if t["name"] == r"scenarios\objects\covenant\military\scarab\scarab")
    result["platform_definition"] = {**tag, "cache": str(TURF.relative_to(ROOT)),
                                     "cache_sha256": hashlib.sha256(cache.data).hexdigest(),
                                     "raw_hex": bytes(cache.data[tag["offset"]:tag["offset"] + tag["size"]]).hex(),
                                     "flags_at_02": f"{cache.h(tag['offset'] + 2):04x}"}
    cache.data.close()
    cache.stream.close()
    return result


class MuzzleTrace(AllocationTrace):
    def __init__(self, mode, aim, crouch, matrices, platform, metadata):
        super().__init__(mode, aim, (5., -3., 2.))
        del self.services[0x1DC8F0]
        del self.services[0x1DC940]
        # Execute the original query wrapper, including its clear-result
        # fraction=1 and ray-end output. A return-only replacement of 16B240
        # would leave endpoint storage stale, which its aim caller still reads.
        # The lower BSP primitive 1E20C0 remains an explicit clear-world service.
        del self.services[0x16B240]
        self.f(UNIT + 0xA0, 1.)
        self.f(UNIT + 0x2C4, crouch)
        self.g.mem_write(UNIT + 0x64, struct.pack("<3f", 10., 20., 30.))
        self.sh(WEAPON + 0x116, 0x500)
        self.sh(WEAPON + 0x11A, 0x900)
        self.sh(WEAPON + 0x114, 4 * 0x34)
        for index, matrix in enumerate(matrices):
            self.g.mem_write(WEAPON + 0x500 + index * 0x34, struct.pack("<13f", *matrix))
            self.w(WEAPON + 0x900 + index * 4, index)
        if platform:
            definition = metadata["platform_definition"]
            datum = int(definition["datum"], 16)
            self.g.mem_write(PLATFORM_DEF, bytes.fromhex(definition["raw_hex"]))
            self.g.mem_write(TAGS + (datum & 0xFFFF) * 16, b"hcam")
            self.w(TAGS + (datum & 0xFFFF) * 16 + 8, PLATFORM_DEF)
            self.g.mem_write(HEADERS + 36, struct.pack("<HBBHHI", 1, 3, 7, 0, 512, PLATFORM_OBJECT))
            self.w(PLATFORM_OBJECT, datum)
            self.w(PLATFORM_OBJECT + 0x14, -1)
            self.w(POOL + 0x38, 4)
            self.w(BITMAP, 15)
            self.sh(UNIT + 0xC0, 2)
            self.w(UNIT + 0xB8, PLATFORM_INDEX)
            self.w(PHYSICS + 0x1D80, 1)
            self.w(PHYSICS + 0x1D00, PLATFORM_INDEX)
            self.g.mem_write(PHYSICS + 0x0C, struct.pack("<3f", *platform["center"]))
            self.g.mem_write(PHYSICS + 0x24, struct.pack("<3f", *platform["linear"]))
            self.g.mem_write(PHYSICS + 0x3C, struct.pack("<3f", *platform["angular"]))
            self.g.mem_write(PHYSICS + 0xE5, b"\x01")
        self.observed_returns = []

    def hook(self, guest, address, size, user):
        for return_address, row, pointer, kind in self.observed_returns[:]:
            if address == return_address:
                row["returned_eax"] = f"{guest.reg_read(UC_X86_REG_EAX):08x}"
                row["output"] = self.vec(pointer)
                if kind == "marker":
                    row["marker_record_hex"] = bytes(guest.mem_read(pointer - 0x60, 0x70)).hex()
                self.observed_returns.remove((return_address, row, pointer, kind))
        stack = guest.reg_read(UC_X86_REG_ESP)
        if address in (0xB9C20, 0xCC320, 0x10DF90):
            kind = {0xB9C20: "marker", 0xCC320: "eye", 0x10DF90: "platform"}[address]
            if kind == "marker":
                pointer = guest.reg_read(UC_X86_REG_EBX) + 0x60
                row = {"original_muzzle_entry": f"{address:08x}",
                       "requested_object": f"{self.u(stack + 4):08x}",
                       "requested_marker_string_id": f"{self.u(stack + 8):08x}",
                       "supplied_max_markers": self.u(stack + 12, 2)}
            elif kind == "eye":
                pointer = guest.reg_read(UC_X86_REG_EAX)
                row = {"original_muzzle_entry": f"{address:08x}",
                       "requested_object": f"{guest.reg_read(UC_X86_REG_ECX):08x}"}
            else:
                pointer = self.u(stack + 8)
                row = {"original_muzzle_entry": f"{address:08x}",
                       "requested_object": f"{self.u(stack + 4):08x}",
                       "definition_flag_gate_enabled": self.u(stack + 12, 1)}
            self.events.append(row)
            self.observed_returns.append((self.u(stack), row, pointer, kind))
        super().hook(guest, address, size, user)


def dump_instructions(directory):
    directory = directory.resolve()
    directory.mkdir(parents=True, exist_ok=True)
    image = Xbe(ROOT / "assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03")
    assert hashlib.sha256(image.data).hexdigest() == PIN
    code = _Code(image, ".text")
    starts = {int(row["address"], 16) for row in csv.DictReader(
        (ROOT / "analysis/map-pack-update/functions.tsv").open(), delimiter="\t")}
    entries = (0xB9C20, 0xB9B90, 0x105CC0, 0x106070, 0x1DC8F0, 0x1DC940,
               0x143580, 0x1441D0, 0xC95F0, 0xCC320, 0xBACC0, 0xBB0C0,
               0x10DF90, 0x1A8560, 0x156F10, 0x23DD90, 0x16B240,
               0x1E2020, 0x1E20C0, 0x16C240)
    rows = []
    for entry in entries:
        routine = _trace(code, entry, starts | set(entries))
        path = directory / f"{entry:08x}.asm"
        instructions = [code.at(address) for address in sorted(routine.code)]
        path.write_text("".join(f"{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n"
                                for i in instructions))
        rows.append({"start": f"{entry:08x}", "end_exclusive": f"{routine.end:08x}",
                     "instruction_count": len(instructions),
                     "reachable_instruction_bytes_sha256": hashlib.sha256(
                         b"".join(i.bytes for i in instructions)).hexdigest(),
                     "listing": str(path.relative_to(ROOT)),
                     "listing_sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
                     "direct_calls": [{"site": f"{site:08x}", "target": f"{target:08x}"}
                                      for site, target, kind in routine.sites if kind == "call"]})
    (directory / "inventory.json").write_text(json.dumps({"xbe_sha256": PIN,
        "oracle_tool_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        "routines": rows,
        "scope": "Original static reachable instruction operands/calls; listing a branch is not proof of its execution."}, indent=2) + "\n")
    print(json.dumps({"routines": len(rows)}))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=ROOT / "analysis/multiplayer/map-pack/smg-muzzle-platform-1.1-oracle.json")
    parser.add_argument("--instruction-inventory", type=Path, help="Write original marker/eye/platform/query routine listings and hashes.")
    args = parser.parse_args()
    if args.instruction_inventory is not None:
        dump_instructions(args.instruction_inventory)
        return
    metadata = authored()
    cases = []
    pose = (1., 1., 0., 0., 0., 1., 0., 0., 0., 1., 10., 20., 30.)
    rotated = (1., 0., 1., 0., -1., 0., 0., 0., 0., 1., 10., 20., 30.)
    platform = {"center": (1., 2., 3.), "linear": (4., 5., 6.), "angular": (.1, -.2, .3)}
    for mode in (1, 5):
        for aim in ((1., 0., 0.), (0., 1., 0.), (0., 0., 1.), (.6, 0., .8)):
            for crouch in (0., .5, 1.):
                for platform_present in (False, True):
                    matrices = [pose] * 4
                    trace = MuzzleTrace(mode, aim, crouch, matrices, platform if platform_present else None, metadata)
                    trace.execute(1, 0)
                    request = next(e for e in trace.events if e.get("entry") == "000b8a30")
                    source = next(e for e in trace.events if e.get("source_velocity_instruction") == "001088b2")
                    assert source["platform_velocity_present"] == int(platform_present)
                    assert sum("rng_write_instruction" in e for e in trace.events) == 2
                    marker = next(e for e in trace.events if e.get("original_muzzle_entry") == "000b9c20")
                    assert int(marker["returned_eax"], 16) & 0xFFFF == 1
                    cases.append({"mode": mode, "supplied_aim": list(aim), "supplied_crouch_fraction": crouch,
                                  "supplied_unit_position": [10., 20., 30.], "supplied_unit_scale": 1.,
                                  "supplied_current_node_matrices": matrices,
                                  "supplied_owner_velocity": [5., -3., 2.],
                                  "supplied_platform_physics_record": platform if platform_present else None,
                                  "request": request, "events": trace.events})
    # A supplied rotated current node pose is a separate boundary case, not a
    # claim that this pose is produced by the original animation graph.
    trace = MuzzleTrace(5, (1., 0., 0.), 0., [rotated] * 4, None, metadata)
    trace.execute(1, 0)
    cases.append({"mode": 5, "supplied_aim": [1., 0., 0.], "supplied_crouch_fraction": 0.,
                  "supplied_unit_position": [10., 20., 30.], "supplied_unit_scale": 1.,
                  "supplied_current_node_matrices": [rotated] * 4,
                  "supplied_owner_velocity": [5., -3., 2.],
                  "supplied_platform_physics_record": None,
                  "request": next(e for e in trace.events if e.get("entry") == "000b8a30"),
                  "events": trace.events})
    assert sum("rng_write_instruction" in event for event in trace.events) == 2
    for index in range(24):
        left, right = cases[index]["request"], cases[index + 24]["request"]
        assert left["request_descriptor_hex"] == right["request_descriptor_hex"]
        assert left["returned_object_hex"] == right["returned_object_hex"]
    report = {"xbe_sha256": PIN, "cache_sha256": CACHE_PIN,
              "oracle_tool_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
              "dependency_tool_sha256": hashlib.sha256((ROOT / "tools/oracle_smg_projectile_1_1.py").read_bytes()).hexdigest(),
              "authored": metadata, "cases": cases, "case_count": len(cases),
              "mode_pairs_with_identical_descriptors_and_objects": 24,
              "scope": "Unchanged original authored marker lookup/transform, unit eye calculation, aim-line projection, supporting-platform helper and upper projectile allocator. Original16B240 and1E2020 query wrappers execute; only lower1E20C0 is supplied clear. Current node matrices, unit states, physics records and lower allocation services are explicitly supplied. No original animation-pose production, actual moving platform physics, collision-world registration, live map location or full-world RNG parity is established."}
    args.output.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"cases": len(cases), "authored_marker_records": len(metadata["marker_records"])}))


if __name__ == "__main__":
    main()
