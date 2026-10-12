#!/usr/bin/env python3
"""Replay retail BDEF0 against already retained original player records.

No original process, input, cache, executable or retained export is modified.
The complete type/physics gate and regular sphere producer execute untouched;
the separately proved registration/update tail is outside this bounded replay.
"""
from pathlib import Path
import argparse
import hashlib
import json
import struct
import subprocess
import sys

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import *

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools/reference/halo2-decompiled/tools"))
from xbe import Xbe
sys.path.insert(0, str(ROOT / "tools"))
from inspect_multiplayer import Cache

PIN = "03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d"
EXPORTS = ROOT / "analysis/multiplayer-shading/original-xemu"


def words(raw):
    return [f"{v:08X}" for v in struct.unpack("<" + "I" * (len(raw) // 4), raw)]


def floats(raw):
    return list(struct.unpack("<" + "f" * (len(raw) // 4), raw))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--native", type=Path)
    parser.add_argument("--output", type=Path, default=ROOT / "analysis/multiplayer-shading/reference-player-spheres.json")
    parser.add_argument("--write-fixture", type=Path)
    args = parser.parse_args()
    xbe = Xbe(ROOT / "assets/disc/default.xbe")
    assert hashlib.sha256(xbe.data).hexdigest() == PIN
    records, native_input = [], []
    for name, file, expected_hash, datum, definition, separate_node in [
        ("cyclotron", "root-unit.bin", "ecb395e2ff033e90f019174e3672691b2173b9c1fafa8fc5e1264bb7cc6ef5ae", 0xE2C60057, 0xEB23099E, "unit-root-node.bin"),
        ("zanzibar", "zanzibar-player20-unit.bin", "5a15494237fb28b8ec48c7638a27df3d84ca1f7f85871a8e248249d2a10f202b", 0xE2E60077, 0xEDF10C6B, None),
    ]:
        raw = (EXPORTS / file).read_bytes()
        assert hashlib.sha256(raw).hexdigest() == expected_hash
        assert struct.unpack_from("<I", raw)[0] == definition and raw[0xAA] == 0
        assert struct.unpack_from("<I", raw, 0x14)[0] == 0xFFFFFFFF
        node_size, node_offset = struct.unpack_from("<hh", raw, 0x114)
        assert node_size > 0 and node_size % 52 == 0 and node_offset > 0
        node = (EXPORTS / separate_node).read_bytes() if separate_node else raw[node_offset:node_offset + 52]
        assert len(node) == 52
        cache = Cache(ROOT / "assets/disc/maps" / (name + ".map"))
        tag = cache.tags[definition]
        assert tag["group"] == "bipd"
        bounds = bytes(cache.data[tag["offset"] + 4:tag["offset"] + 20])
        scale = raw[0xA0:0xA4]
        retained = raw[0x30:0x40]

        u = Uc(UC_ARCH_X86, UC_MODE_32)
        u.mem_map(0x10000, 0x600000)
        for section in xbe.sections:
            u.mem_write(section.va, xbe.section_bytes(section))
        base, stack, stop = 0x1000000, 0x200FF00, 0x3000000
        u.mem_map(base, 0x200000)
        u.mem_map(0x2000000, 0x10000)
        u.mem_map(stop, 0x1000)
        globals_at, handles, tags, object_at, definition_at = base, base + 0x1000, base + 0x10000, base + 0x100000, base + 0x110000
        put = lambda at, value: u.mem_write(at, struct.pack("<I", value))
        put(0x4E0300, globals_at)
        put(globals_at + 0x44, handles)
        put(handles + (datum & 0xFFFF) * 12 + 8, object_at)
        put(0x4E3B44, tags)
        put(tags + (definition & 0xFFFF) * 16 + 8, definition_at)
        # Relocation changes only absolute host pointers. Authored tag bytes,
        # all object bytes and the relative node block offset are unchanged.
        u.mem_write(object_at, raw)
        u.mem_write(object_at + node_offset, node)
        u.mem_write(definition_at, bytes(cache.data[tag["offset"]:tag["offset"] + 0x30]))
        physics_calls = []
        u.hook_add(UC_HOOK_CODE, lambda uc, pc, size, context: physics_calls.append(pc) if pc == 0x1D44F0 else None)
        u.mem_write(stack, struct.pack("<II", stop, datum))
        u.reg_write(UC_X86_REG_ESP, stack)
        u.reg_write(UC_X86_REG_FPCW, 0x027F)
        u.reg_write(UC_X86_REG_FPTAG, 0xFFFF)
        u.reg_write(UC_X86_REG_MXCSR, 0x1F80)
        u.emu_start(0xBDEF0, 0xBE07F, count=2000)
        assert u.reg_read(UC_X86_REG_EIP) == 0xBE07F and not physics_calls
        result = bytes(u.mem_read(object_at + 0x30, 16))
        assert result == retained, (name, words(result), words(retained))
        native_input.append(" ".join(format(v, ".9g") for v in floats(bounds + scale + node)))
        records.append(dict(map=name, object_datum=f"{datum:08X}", definition=f"{definition:08X}", object_type=raw[0xAA],
            raw_file=file, raw_sha256=expected_hash, raw_size=len(raw),
            node_file=separate_node or file, node_file_offset=0 if separate_node else node_offset,
            node_sha256=hashlib.sha256(node).hexdigest(), node0_words=words(node),
            object_flags=words(raw[4:8])[0], physics_datum=words(raw[0xB4:0xB8])[0], physics_flags_C0=words(raw[0xC0:0xC4])[0],
            scale_word=words(scale)[0], authored_bounds_words=words(bounds),
            sphere_words=words(retained), sphere_values=floats(retained),
            original_BDEF0_words=words(result), original_physics_calls=len(physics_calls),
            map_sha256=hashlib.sha256(cache.data).hexdigest()))
        cache.data.close()
        cache.stream.close()
    if args.native:
        output = subprocess.check_output([str(args.native.resolve()), "--sphere-probe"], input="\n".join(native_input) + "\n", text=True)
        rows = output.splitlines()
        assert len(rows) == len(records)
        for record, row in zip(records, rows):
            native = struct.pack("<4f", *map(float, row.split()))
            record["native_words"] = words(native)
            assert record["native_words"] == record["sphere_words"]
    report = dict(source_executable_sha256=PIN, executed="Untouched retail BDEF0 entry through BE07F", source_services_replaced=[],
        native_binary_sha256=hashlib.sha256(args.native.read_bytes()).hexdigest() if args.native else None,
        source_record_count=len(records), exact_original_cached_spheres=len(records), records=records,
        scope="Retained type0 original WORLD node0 and actual object+A0 reproduce all four cached object+30/+3C words. BDEF0 physics bounds override is type11 only. This does not prove a native player idle controller/aiming/pose has the original current WORLD node0, nor equality of native pose/update/cache scheduling. First-person coverage uses the actual root owner cache; its weapon/hands palette is a separate input.")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n")
    if args.write_fixture:
        args.write_fixture.write_text("# Retail retained player BDEF0 sphere. Map, full handle, definition, authored radius/offset, scale, WORLD node0, cached sphere: hex IEEE754.\n"
            + "".join(" ".join([r["map"], r["object_datum"], r["definition"], *r["authored_bounds_words"], r["scale_word"], *r["node0_words"], *r["sphere_words"]]) + "\n" for r in records))
    print(json.dumps({key: value for key, value in report.items() if key != "records"}, indent=2))


if __name__ == "__main__":
    main()
