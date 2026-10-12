#!/usr/bin/env python3
"""Execute original Vista PCR asset owners over the real supplied tag region.

The original60BB1D,63965D,63967A,60C1A9 and5B4DB6 instructions run unchanged.
Only runtime relocation roots and the caller's argument/return stack are bound.
This is asset/consumer evidence, not native-route or GPU emblem verification.
"""
from pathlib import Path
import argparse
import hashlib
import json
import struct

from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EIP, UC_X86_REG_ESP
from export_vista_menu_ui import VistaCache
from oracle_vista_pcr_screen import ROOT, SHA, STOP, STACK, machine, w32

MAP_SHA = "eb259c207da26adc1820ae0757d23efb4ce1d2c2018956635d1094c8364f6b44"
TAGDATA = 0x2000000
WINDOWS = {
    "ui_owner": (0x60BB1D, 0x60BB89),
    "tag_lookup": (0x63965D, 0x639697),
    "rank_lookup": (0x60C1A9, 0x60C226),
    "team_palette": (0x5B4DB6, 0x5B4E48),
}


def sha(data):
    return hashlib.sha256(data).hexdigest()


def execute(cache, image):
    u = machine(image)
    region = bytes(cache.data[cache.index:])
    u.mem_map(TAGDATA, (len(region) + 4095) & ~4095)
    u.mem_write(TAGDATA, region)
    visits = 0

    def count(_u, _pc, _size, _data):
        nonlocal visits
        visits += 1

    u.hook_add(UC_HOOK_CODE, count)
    table = cache.u32(cache.index + 8)
    scenario = cache.tag(cache.scenario, "scnr")[0]
    matg = cache.tag(cache.u32(cache.index + 16), "matg")[0]
    w32(u, 0x87CD50, TAGDATA + table)
    w32(u, 0x87CD54, TAGDATA)
    w32(u, 0x882290, TAGDATA)
    w32(u, 0x879E74, TAGDATA + scenario["address"])
    w32(u, 0x879E70, TAGDATA + matg["address"])

    def call(pc, args=()):
        before = visits
        at = STACK + 0x18000
        u.mem_write(at, struct.pack("<" + "I" * (len(args) + 1), STOP, *args))
        u.reg_write(UC_X86_REG_ESP, at)
        u.emu_start(pc, STOP, count=100000)
        assert u.reg_read(UC_X86_REG_EIP) == STOP, "original owner did not return"
        return u.reg_read(UC_X86_REG_EAX), visits - before

    matg_at = cache.tag(matg["datum"], "matg")[1]
    scenario_at = cache.tag(scenario["datum"], "scnr")[1]
    scenario_type = cache.u16(scenario_at + 0x10)
    assert scenario_type == 2 and cache.u32(matg_at + 0x110) == 1
    interface = cache.pointer(cache.u32(matg_at + 0x114), cache.magic, 0x98)
    wgtz_datum = cache.u32(interface + 0x84)
    wgtz, wgtz_at = cache.tag(wgtz_datum, "wgtz")
    owner, owner_visits = call(0x60BB1D)
    assert owner == TAGDATA + wgtz["address"], "actual original UI owner differs"
    rank_datum = struct.unpack("<I", u.mem_read(owner + 4, 4))[0]
    assert rank_datum == cache.u32(wgtz_at + 4)
    rank_tag, rank_at = cache.tag(rank_datum, "wigl")
    ranges = [struct.unpack("<hh", cache.data[p:p + 4])
              for p in cache.block(rank_at + 0x158, 4)]
    rank_cases = []
    for rank in [-32768, *range(-1, 129), 32767]:
        value, steps = call(0x60C1A9, (rank & 0xFFFFFFFF,))
        expected = next((i for i, (first, last) in enumerate(ranges)
                         if first <= rank <= last), 0xFFFFFFFF)
        assert value == expected, ("original rank consumer", rank, value, expected)
        rank_cases.append(dict(rank=rank, result=value, visits=steps))

    mulg_datum = cache.u32(matg_at + 0x16C)
    mulg, mulg_at = cache.tag(mulg_datum, "mulg")
    universal = cache.pointer(cache.u32(mulg_at + 4), cache.magic, 0x18)
    colors = [list(struct.unpack("<III", cache.data[p:p + 12]))
              for p in cache.block(universal + 0x10, 12)]
    fallback_address = struct.unpack("<I", u.mem_read(0x8127B4, 4))[0]
    fallback = list(struct.unpack("<III", u.mem_read(fallback_address, 12)))
    team_cases = []
    for team in [*range(10), 0x7FFF, 0x8000, 0xFFFF]:
        result, steps = call(0x5B4DB6, (STACK + 0x1000, team))
        assert result == STACK + 0x1000
        rgb = list(struct.unpack("<III", u.mem_read(result, 12)))
        expected = colors[team] if team < len(colors) else fallback
        assert rgb == expected, ("original team consumer", team, rgb, expected)
        team_cases.append(dict(team=team, rgb=rgb, visits=steps))
    return dict(schema="H2V_PCR_APPEARANCE_ASSET_OWNERS_V1",
        source_sha256=sha(image), source_map_sha256=sha(cache.data),
        source_owners=dict(scenario=scenario["datum"], scenario_type=scenario_type,
            matg=matg["datum"], mulg=mulg["datum"], wgtz=wgtz["datum"],
            rank_wigl=rank_tag["datum"], original_ui_owner_result=owner,
            original_ui_owner_visits=owner_visits),
        team_palette=colors, rank_ranges=ranges,
        rank_cases=rank_cases, team_cases=team_cases, cases=1 + len(rank_cases) + len(team_cases),
        visits=visits, boundaries=[
            "Original UI owner, tag lookup, rank lookup and team palette instructions execute unchanged over the real supplied tag region.",
            "Runtime relocation roots and the caller stack are explicit bindings; no source callee is stubbed.",
            "This proves actual imported owners and source consumers, not live session identity, the native live route or GPU emblem rendering."])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--map", type=Path, default=ROOT / "Halo 2 Vista installer/maps/mainmenu.map")
    parser.add_argument("--exe", type=Path, default=ROOT / "Halo 2 Vista installer/halo2.exe")
    parser.add_argument("--output", type=Path, default=ROOT / "analysis/frontend/vista-session-flow-audit/pcr-appearance-original")
    args = parser.parse_args()
    image = args.exe.read_bytes()
    assert sha(image) == SHA, "supplied original executable changed"
    cache = VistaCache(args.map)
    try:
        assert sha(cache.data) == MAP_SHA, "supplied original map changed"
        report = execute(cache, image)
    finally:
        cache.close()
    decoder, lines, windows = Cs(CS_ARCH_X86, CS_MODE_32), [], {}
    for name, (lo, hi) in WINDOWS.items():
        raw = image[lo - 0x400000:hi - 0x400000]
        windows[name] = dict(begin=hex(lo), end_exclusive=hex(hi), sha256=sha(raw))
        lines.append("; " + name)
        lines.extend(f"{i.address:08X} {i.mnemonic} {i.op_str}" for i in decoder.disasm(raw, lo))
    report["windows"] = windows
    report["generator_sha256"] = sha(Path(__file__).read_bytes())
    args.output.mkdir(parents=True, exist_ok=True)
    path = args.output / "asset-owners-execution.json"
    path.write_text(json.dumps(report, indent=2) + "\n")
    (args.output / "asset-owners-source.asm").write_text("\n".join(lines) + "\n")
    print(json.dumps(dict(cases=report["cases"], visits=report["visits"],
        team_colors=len(report["team_palette"]), rank_ranges=len(report["rank_ranges"]),
        report_sha256=sha(path.read_bytes()))))


if __name__ == "__main__":
    main()
