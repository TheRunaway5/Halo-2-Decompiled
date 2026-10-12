#!/usr/bin/env python3
"""Compare Vista conversion against unchanged, fingerprinted TU1.5 code.

Vista metadata uses low virtual addresses occupied by the Xbox executable.
The ABI fixture relocates resident allocations and their walked block pointers
by +80000000, without changing tag values, references or source instructions.
This verifies the two callees on Vista inputs, not the full Xbox map loader.
"""
import argparse
from contextlib import ExitStack
import ctypes
import json
import mmap
from pathlib import Path
import struct
import sys

from audit_rendering import FormatError, file_sha
from setup_assets import VistaUiCache
from vista_tag_conversion import TagGraph, apply_combat, apply_netgame, BALANCE_WORDS, TU15_SHA256

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools/reference/halo2-decompiled/tools"))
from xbe import Xbe
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_MXCSR
from oracle_mp_netgame_postprocess_updates import adapter

ALIAS, TABLE, STACK, STOP = 0x80000000, 0x1080000, 0x2010000, 0x3000000
ADDRESSES = (0x46A374, 0x46A370, 0x455A90, 0x46A36C, 0x46A368,
    0x46A0CC, 0x46A364, 0x46A1A4, 0x449FD4, 0x46A13C, 0x46A360,
    0x46A35C, 0x46A358, 0x46A194, 0x46A354, 0x46A22C, 0x46A350,
    0x46A20C, 0x455A94)


def instruction_inventory(xbe):
    directory = ROOT / "analysis/multiplayer/title-update-1.5"
    paths = (directory / "combat-postprocess/measured/owned_1_5/00165b30.asm",
             directory / "netgame-postprocess/measured/beavercreek-owned_1_5/0011f060.asm",
             directory / "netgame-postprocess/measured/beavercreek-owned_1_5/002db4ca.asm")
    instructions = {}
    for path in paths:
        for row in path.read_text().splitlines():
            at, hex_bytes, _ = row.split(" ", 2)
            address, binary = int(at, 16), bytes.fromhex(hex_bytes)
            if xbe.read(address, len(binary)) != binary:
                raise FormatError("frozen original instruction differs from the pinned XBE")
            instructions[address] = binary
    return instructions


def execute(cache, shared, xbe, instructions, platform):
    graph = TagGraph(cache, shared if cache.u32(32) else None)
    scenario = graph.resolve(cache.u32(cache.u32(16) + 12), "scnr")
    multiplayer = graph.u16(scenario + 0x10) == 1
    globals_address = graph.resolve(cache.u32(cache.u32(16) + 16), "matg") if multiplayer else 0
    g = Uc(UC_ARCH_X86, UC_MODE_32)
    g.mem_map(0x10000, 0x800000)
    for section in xbe.sections:
        g.mem_write(section.va, xbe.section_bytes(section))
    size = (max(t["address"] + t["size"] for _, t in graph.regions) + 4095) & ~4095
    g.mem_map(ALIAS, size)
    g.mem_map(TABLE, 0x100000)
    g.mem_map(0x2000000, 0x200000)
    g.mem_map(STOP, 4096)
    for owner, tag in graph.regions:
        at = owner.pointer(tag["address"], owner.magic, tag["size"])
        g.mem_write(ALIAS + tag["address"], bytes(owner.data[at:at+tag["size"]]))
        g.mem_write(TABLE + (tag["datum"] & 65535) * 16 + 8,
                    struct.pack("<I", ALIAS + tag["address"]))
    relocations = {}

    def relocate(at):
        pointer = graph.u32(at)
        graph.location(pointer, 1)
        relocations[at] = pointer
        g.mem_write(ALIAS + at, struct.pack("<I", ALIAS + pointer))
        return pointer

    if multiplayer:
        mulg = graph.resolve(graph.u32(globals_address + 0x16C), "mulg")
        runtime = relocate(mulg + 0xC)
        weapons = relocate(runtime + 0x6C)
        relocate(runtime + 0x74)
        relocate(globals_address + 0x104)
        for slot in (0, 1, 3, 5, 7, 12):
            datum = graph.u32(weapons + slot * 8 + 4)
            if datum != 0xFFFFFFFF:
                weapon = graph.resolve(datum, "weap")
                if graph.u32(weapon + 0x2D4):
                    relocate(weapon + 0x2D4)
        level = relocate(scenario + 0x39C)
        relocate(level + 0x14)
        if graph.u32(scenario + 0x118):
            relocate(scenario + 0x11C)
    for at, value in ((0x4F0414, TABLE), (0x4ECC1C, ALIAS + globals_address), (0x4ECC20, ALIAS + scenario)):
        g.mem_write(at, struct.pack("<I", value))
    stores, executed, protection = [], set(), []
    lib, cb_type = platform

    def word(at):
        return int.from_bytes(g.mem_read(at, 4), "little")

    def code_hook(_g, at, _size, _user):
        if at != 0x800000B6:
            if at not in instructions or bytes(g.mem_read(at, len(instructions[at]))) != instructions[at]:
                raise FormatError(f"execution escaped unchanged source instructions: {at:08x}")
            executed.add(at)
            return
        sp = g.reg_read(UC_X86_REG_ESP)
        base, length, flags = (word(sp + i) for i in (4, 8, 12))
        if base != ALIAS + graph.u32(scenario + 0x11C) or length != graph.u32(scenario + 0x118) * 32:
            raise FormatError("original protection request escaped authored netgame rows")
        lower, upper = base & ~4095, (base + length + 4095) & ~4095
        conversion = (ctypes.c_uint32 * 4)()
        if lib.contract(flags, conversion) != 1:
            raise FormatError("unsupported original metadata protection request")
        applied = []
        def transport(address, count, windows, old):
            if (address, count, windows) != (base, length, conversion[3]):
                return 0
            permissions = {0x40: 7, 0x20: 5}[windows]
            regions = [perms for lo, hi, perms in g.mem_regions() if lo < upper and hi >= lower]
            old[0] = {7: 0x40, 5: 0x20}[regions[0]]
            g.mem_protect(lower, upper-lower, permissions)
            applied.append(permissions)
            return 1
        callback = cb_type(transport)
        if lib.apply(base, length, flags, callback) != 1 or len(applied) != 1:
            raise FormatError("original access adapter failed")
        protection.append(dict(base=base-ALIAS, bytes=length, original_flags=flags,
                               applied_unicorn_permissions=applied[0]))
        g.reg_write(UC_X86_REG_EIP, word(sp))
        g.reg_write(UC_X86_REG_ESP, sp + 16)

    def write_hook(_g, _access, at, length, value, _user):
        if 0x2000000 <= at < 0x2200000:
            return
        if not ALIAS <= at < ALIAS + size or length != 4:
            raise FormatError("unexpected original source write")
        address = at - ALIAS
        owner, tag, offset = graph.location(address, length)
        stores.append(dict(map=owner.path.name, datum=f"{tag['datum']:08x}",
            group=tag["group"], tag=tag["name"], address=address, file_offset=offset,
            tag_offset=address-tag["address"], before=f"{word(at):08x}", after=f"{value & 0xffffffff:08x}",
            instruction=f"{g.reg_read(UC_X86_REG_EIP):08x}"))

    g.hook_add(UC_HOOK_CODE, code_hook)
    g.hook_add(UC_HOOK_MEM_WRITE, write_hook)
    for entry in (0x11F060, 0x165B30):
        g.mem_write(STACK, struct.pack("<I", STOP))
        g.reg_write(UC_X86_REG_ESP, STACK)
        g.reg_write(UC_X86_REG_FPCW, 0x027F)
        g.reg_write(UC_X86_REG_MXCSR, 0x1F80)
        g.emu_start(entry, STOP, count=10000)
        if g.reg_read(UC_X86_REG_EIP) != STOP or g.reg_read(UC_X86_REG_ESP) != STACK + 4:
            raise FormatError("original postprocessor did not return normally")
    apply_netgame(graph, scenario)
    apply_combat(graph, scenario, globals_address)
    if stores != graph.stores:
        raise FormatError(f"converter writes differ from unchanged TU1.5 executable: {cache.path.name}")
    # Compare every resident tag byte, restoring only the declared pointer
    # relocations. This detects extra or missing writes outside the store trace.
    for at, value in relocations.items():
        g.mem_write(ALIAS + at, struct.pack("<I", value))
    for owner, tag in graph.regions:
        at = owner.pointer(tag["address"], owner.magic, tag["size"])
        if bytes(g.mem_read(ALIAS + tag["address"], tag["size"])) != bytes(owner.data[at:at+tag["size"]]):
            raise FormatError("complete processed resident metadata differs")
    return dict(map=cache.path.name, source_sha256=file_sha(cache.path), multiplayer=multiplayer,
        stores=len(stores), actual_changed_stores=sum(r["before"] != r["after"] for r in stores),
        unchanged_original_instructions_executed=len(executed), protection=protection,
        declared_pointer_relocations=[dict(address=at, original=value, fixture=ALIAS+value)
                                      for at, value in sorted(relocations.items())],
        exact_ordered_stores_equal=True, complete_resident_metadata_equal=True)


def verify(maps, xbe_path, output):
    if file_sha(xbe_path) != TU15_SHA256:
        raise FormatError("verification requires the inspected, unmodified Xbox TU1.5 executable")
    xbe = Xbe(xbe_path)
    if tuple(int.from_bytes(xbe.read(at, 4), "little") for at in ADDRESSES) != BALANCE_WORDS:
        raise FormatError("converter balance words differ from original executable operands")
    instructions, platform = instruction_inventory(xbe), adapter()
    cases = []
    with ExitStack() as stack:
        shared = VistaUiCache(maps / "shared.map")
        stack.callback(shared.close)
        for path in sorted(maps.glob("*.map")):
            if path.name in ("shared.map", "single_player_shared.map"):
                continue
            cache = VistaUiCache(path)
            try:
                for c in (cache, shared):
                    c.data.close()
                    c.data = mmap.mmap(c.file.fileno(), 0, access=mmap.ACCESS_COPY)
                cases.append(execute(cache, shared, xbe, instructions, platform))
                print(f"Source differential passed: {path.name}", flush=True)
            finally:
                cache.close()
    report = dict(source_xbe_sha256=TU15_SHA256, cases=cases,
        scope="two unchanged original callees on Vista metadata with explicit ABI pointer relocations; original preceding loader/fixups and live gameplay excluded",
        original_code_patched=False, original_gameplay_results_supplied=False, full_tu15_parity=False)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, indent=2) + "\n")
    return report


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--maps", type=Path, required=True)
    parser.add_argument("--xbe", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    verify(args.maps, args.xbe, args.output)
