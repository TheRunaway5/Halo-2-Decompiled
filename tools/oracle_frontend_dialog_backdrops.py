#!/usr/bin/env python3
"""Replay the original shared mainmenu dialog backdrop predicate.

Uses actual wgit flags and common screen virtuals, a retained lobby14 on
channel5/window4, and a foreground dialog on channel0/window4 in UI mode2.
This isolates the common policy; it does not prove every original navigation
route, transient screen lifetime, or complete original/native framebuffer.
"""
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from audit_rendering import Cache
from oracle_multiplayer_hud_positions import Retail, PIN
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_EDI, UC_X86_REG_ESI, UC_X86_REG_ESP, UC_X86_REG_XMM0


def main():
    cache = Cache(ROOT / "assets/disc/maps/mainmenu.map")
    cache_pin = hashlib.sha256(cache.data).hexdigest()
    assert cache_pin == "1b5de78212ee7439b086ef3554885a4188884864ce1040b24ec6604af4a0143b"
    channels = json.loads((ROOT / "analysis/frontend/profile-regression/original-profile-channels.json").read_text())
    lobby = json.loads((ROOT / "analysis/frontend/lobby/reference/split-live-state.json").read_text())
    r = Retail()
    g = r.g
    g.mem_map(0x82000000, 0x1000000)
    for channel in channels["channels_in_147AF1_render_order"]:
        pointer = int(channel["address"], 16)
        g.mem_write(pointer, bytes.fromhex(channel["raw_hex"]))
        for screen in channel["screens"].values():
            if screen:
                g.mem_write(int(screen["address"], 16), bytes.fromhex(screen["raw_hex"]))
        if channel["channel"] != "default":
            for field in (8, 12, 0x30):
                r.w(pointer + field, 0)
    lobby_pointer = int(lobby["screen_pointer"], 16)
    g.mem_write(lobby_pointer, bytes.fromhex(lobby["screen_hex"]))
    r.w(0x54d72c + 8, lobby_pointer)
    foreground_channel = next(int(c["address"], 16) for c in channels["channels_in_147AF1_render_order"] if c["channel"] == "0")
    dialog = r.alloc(bytes(0x600))
    r.w(foreground_channel + 8, dialog)
    g.mem_write(0x54d5a8, b"\x02")
    r.w(0x54d5b8, 1500)
    _, wigl = cache.tag(next(k for k, t in cache.tags.items() if t["group"] == "wigl"), "wigl")
    globals_pointer = r.alloc(cache.data[wigl:wigl + 452])

    # Screen vtables with the common channel/window virtuals also
    # share1473C9: the authored flag5 stored by22F916, rather than an ID list.
    data = bytes(g.mem_read(0x450000, 0xe000))
    vtables = [0x450000 + offset for offset in range(0, len(data) - 112, 4)
               if struct.unpack_from("<2I", data, offset + 80) == (0x23012c, 0x230130)]
    assert len(vtables) == 85
    assert all(r.r(p + 108) == 0x1473c9 for p in vtables)
    submissions = []

    def hook(_guest, address, _size, _user):
        arguments = {0x148350: 0, 0x147cdb: 4, 0x235756: 0, 0x2353a5: 4}
        if address not in arguments:
            return
        sp = g.reg_read(UC_X86_REG_ESP)
        if address == 0x148350:
            r.ret(globals_pointer)
        else:
            if address == 0x147cdb:
                submissions.append({"service": "tint", "argb": f"{r.r(sp + 4):08x}"})
            elif address == 0x235756:
                submissions.append({"service": "blur", "fade_bits": f"{g.reg_read(UC_X86_REG_XMM0) & 0xffffffff:08x}"})
            else:
                submissions.append({"service": "screen", "id": r.r(g.reg_read(UC_X86_REG_EDI) + 0x70)})
            r.ret(0)
        g.reg_write(UC_X86_REG_ESP, sp + 4 + arguments[address])

    g.hook_add(UC_HOOK_CODE, hook)
    rows = []
    for tag in cache.tags.values():
        if tag["group"] != "wgit":
            continue
        p = tag["address"] - cache.magic
        flags, screen_id = cache.u32(p), cache.u16(p + 4)
        if not flags & 25:
            continue
        definition = r.alloc(cache.data[p:p + 44])
        g.mem_write(dialog, bytes(0x600))
        r.w(dialog, {7: 0x454560, 8: 0x4545d0}.get(screen_id, 0x458840))
        r.w(dialog + 0x70, screen_id)
        r.w(dialog + 0x74, 0)
        r.w(dialog + 0x78, 4)
        r.run(0x22f916, stop=0x22f923,
              registers=[(UC_X86_REG_ESI, dialog), (UC_X86_REG_EDI, definition)])
        predicate = bool(r.run(0x23029a, registers=[(UC_X86_REG_ESI, dialog)]) & 255)
        submissions.clear()
        r.run(0x234f8c, argv=[globals_pointer], registers=[(UC_X86_REG_ECX, foreground_channel)])
        expected = not bool(flags & 32)
        assert predicate == expected, (screen_id, flags, predicate)
        assert submissions == ([{"service": "tint", "argb": "d900142c"},
                                {"service": "blur", "fade_bits": "3f800000"}] if expected else []) + [{"service": "screen", "id": screen_id}]
        rows.append({"id": screen_id, "flags": flags, "backdrop": predicate, "submissions": submissions.copy()})
    report = {"scope": __doc__, "xbe_sha256": PIN, "mainmenu_sha256": cache_pin,
              "common_vtables_checked": vtables, "dialogs": rows}
    output = ROOT / "analysis/frontend/gui-overlap-20261010/original-dialog-backdrops.json"
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, indent=2) + "\n")
    print(f"{len(rows)} original dialog policies passed; {len(vtables)} common virtual tables verified")


if __name__ == "__main__":
    main()
