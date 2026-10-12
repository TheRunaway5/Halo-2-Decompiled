#!/usr/bin/env python3
"""Freeze complete original150DD0 executions on explicit input/state controls.

No source instructions or callees are replaced. Input device polling and the
later interaction dispatcher are outside this pure player-edge comparison.
"""
from pathlib import Path
import hashlib
import random
import struct
from audit_world_vertex_declarations import Xbe, ROOT
from weapon_reference_versions import images, unique_entry
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP

PIN = 'aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377'
x = Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
assert hashlib.sha256(x.data).hexdigest() == PIN
pool, player, action, stack, stop = 0x1000000, 0x1001000, 0x1002000, 0x100ff00, 0x3000000
versions=[]
for label,image,pin in images():
    entry=unique_entry(image,x.read(0x150dd0+9,32),9)
    # This complete leaf has one relocated global operand and no callees.
    body=x.read(0x150dd0,0x130);other=image.read(entry,0x130)
    assert body[:5]==other[:5] and body[9:]==other[9:]
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x800000)
    for s in image.sections:u.mem_write(s.va,image.section_bytes(s))
    u.mem_map(0x1000000,0x10000);u.mem_map(0x3000000,0x1000)
    u.mem_write(struct.unpack_from('<I',other,5)[0],struct.pack('<I',pool))
    u.mem_write(pool+0x44,struct.pack('<I',player))
    versions.append((label,u,entry,pin))
rng = random.Random(0x150dd0)
rows = []
for case in range(4096):
    p = bytearray(rng.randbytes(0x21c))
    a = bytearray(rng.randbytes(0x5c))
    # Exhaust all low-button/consumption combinations, with unrelated high
    # bits randomized to detect overly broad masks or accidental stores.
    struct.pack_into('<H', p, 2, (struct.unpack_from('<H', p, 2)[0]&~0x30) | ((case>>8)&3)*16)
    struct.pack_into('<H', p, 0x3c, (struct.unpack_from('<H', p, 0x3c)[0]&~0x11) | ((case>>10)&1) | ((case>>11)&1)*16)
    a[0x1c] = case&255
    results=[]
    for label,u,entry,pin in versions:
        u.mem_write(player, bytes(p));u.mem_write(action, bytes(a))
        u.mem_write(stack,struct.pack('<I',stop))
        u.reg_write(UC_X86_REG_ESP, stack);u.reg_write(UC_X86_REG_EAX, 0x12340000)
        u.reg_write(UC_X86_REG_ECX, action);u.emu_start(entry, stop, count=1000)
        assert u.reg_read(UC_X86_REG_EIP) == stop
        results.append((bytes(u.mem_read(player,len(p))),bytes(u.mem_read(action,len(a)))))
    assert all(r==results[0] for r in results),f'Original versions differ case {case}'
    rows.append(' '.join([p.hex(),a.hex(),results[0][0].hex(),results[0][1].hex()]))
out = ROOT/'tests/fixtures/player_action_edges_original.tsv'
out.write_text('# Untouched original player-edge leaves; full player21C/action5C buffers\n'+
    ''.join(f'# {label} entry={entry:08x} XBE {pin}\n' for label,u,entry,pin in versions)+'\n'.join(rows)+'\n')
print('Wrote',len(rows),'full-buffer cases matching all',len(versions),'original versions to',out.relative_to(ROOT))
