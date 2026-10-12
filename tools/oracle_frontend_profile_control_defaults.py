#!/usr/bin/env python3
"""Execute unchanged1A0750 to pin newly initialized profile control bytes.

The actual profile initializer runs without intercepted callees. Only the480-byte
output buffer and normal source execution environment are supplied. Record-list
prefix relocation and native preference persistence are excluded.
"""
import hashlib
import json

from oracle_multiplayer_hud_positions import Retail, ROOT, PIN, Xbe
from inventory import read_rows
from disasm import listing
from xbe import FUNCTIONS_CSV
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EDX

retail = Retail()
profile = retail.alloc(bytes(0x1E0))
entries = []


def executed(_machine, address, _size, _context):
    if address == 0x1A0750:
        entries.append(address)


retail.g.hook_add(UC_HOOK_CODE, executed)
cases = []
for fill in (0, 0xFF):
    retail.g.mem_write(profile, bytes([fill]) * 0x1E0)
    entries.clear()
    retail.run(0x1A0750, registers=[(UC_X86_REG_EDX, profile)])
    raw = bytes(retail.g.mem_read(profile, 0x1E0))
    assert entries == [0x1A0750] and raw[0x100:0x102] == bytes(2)
    cases.append(dict(input_fill=fill, buttons=raw[0x100], thumbsticks=raw[0x101],
                      executed_entries=len(entries), output_profile_hex=raw.hex()))
out = ROOT / "analysis/frontend/profile-row-appearance"
fixture = dict(scope=__doc__, original_xbe_sha256=PIN, address="001a0750",
               buttons_offset="00000100", thumbsticks_offset="00000101", cases=cases,
               framebuffer_parity=False)
path = out / "profile-controls-default-original.json"
path.write_text(json.dumps(fixture, indent=2) + "\n")
xbe = Xbe(ROOT / "assets/disc/default.xbe")
inventory = read_rows(FUNCTIONS_CSV)
instructions = listing(xbe.read(0x1A0750, int(inventory[0x1A0750]["size"])), 0x1A0750, inventory)
(out / "profile-controls-default-original.asm").write_text("\n".join(instructions)+"\n")
print(f"{len(cases)} original control defaults passed; SHA256 {hashlib.sha256(path.read_bytes()).hexdigest()}")
