#!/usr/bin/env python3
"""Freeze Vista full-scoreboard paint ownership and actual connection strips.

46BF66 executes unchanged. 672D24 is a void draw-submission recorder, not an
invented renderer result. This is bounded source evidence, not GPU parity.
"""
from pathlib import Path
import hashlib
import json
import struct
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_EIP, UC_X86_REG_ESP
from oracle_vista_pcr_screen import ROOT, SHA, STOP, STACK, OBJ, machine


def main():
    image = (ROOT / "Halo 2 Vista installer/halo2.exe").read_bytes()
    assert hashlib.sha256(image).hexdigest() == SHA
    u = machine(image)
    draws = []
    visits = 0

    def hook(u, pc, size, data):
        nonlocal visits
        visits += 1
        if pc != 0x672d24:
            return
        sp = u.reg_read(UC_X86_REG_ESP)
        ret, rect, color = struct.unpack("<III", u.mem_read(sp, 12))
        draws.append((struct.unpack("<4h", u.mem_read(rect, 8)),
                      struct.unpack("<4I", u.mem_read(color, 16))))
        u.reg_write(UC_X86_REG_ESP, sp + 4)
        u.reg_write(UC_X86_REG_EIP, ret)

    u.hook_add(UC_HOOK_CODE, hook)
    rows = []
    for quality in (0, 1, 2, 3, 4, 5, 6, 7, 65535):
        for alpha in (0., .25, 1.):
            widget = bytearray(64)
            struct.pack_into("<4h", widget, 0, 20, 100, 39, 106)
            struct.pack_into("<f", widget, 0x24, alpha)
            struct.pack_into("<H", widget, 0x38, quality)
            widget[0x3c] = 1
            u.mem_write(OBJ, bytes(widget))
            sp = STACK + 0x18000
            u.mem_write(sp, struct.pack("<I", STOP))
            u.reg_write(UC_X86_REG_ESP, sp)
            u.reg_write(UC_X86_REG_ECX, OBJ)
            draws.clear()
            u.emu_start(0x46bf66, STOP, count=10000)
            assert u.reg_read(UC_X86_REG_EIP) == STOP
            for rect, color in draws:
                rows.append(" ".join(map(str, (quality, struct.unpack("<I", struct.pack("<f", alpha))[0], *rect, *color))))
    fixture = ROOT / "tests/fixtures/vista_full_scoreboard_quality.tsv"
    fixture.write_text("# unchanged Vista46BF66; quality alpha-word top left bottom right A R G B\n" + "\n".join(rows) + "\n")
    # Literal operands of4742E2,473AC1,473AEB,473AB6,473B1F.
    addresses = (0x79ded0, 0x7a03e0, 0x79c168, 0x7ba848, 0x7b6c78)
    words = [struct.unpack_from("<I", image, at - 0x400000)[0] for at in addresses]
    assert words == [0x3e000000, 0x3e800000, 0x3e99999a, 0x3f333333, 0x3ecccccd]
    output = ROOT / "analysis/multiplayer/hud-functionality/vista-scoreboard-gui"
    output.mkdir(parents=True, exist_ok=True)
    report = dict(source_executable_sha256=SHA, instruction_patches=[], cases=27,
        rectangle_submissions=len(rows), visits=visits,
        style_words=words, style_addresses=[hex(a) for a in addresses],
        fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),
        original_full_scoreboard="4749F7", original_emblem_owner="46BE60",
        original_bitmap_draw="62265C", original_solid_draw="672D24",
        boundaries=__doc__)
    (output / "original-artwork-owners.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report))


if __name__ == "__main__":
    main()
