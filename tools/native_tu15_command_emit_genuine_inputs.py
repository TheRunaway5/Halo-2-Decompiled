#!/usr/bin/env python3
"""Recover live 41030 inputs from original Lockout C35D0/31C70/44AB0.

The discovery run executes original instructions in Unicorn to recover caller
packets, arguments, and the pre-entry bytes of accessed pages. It supplies
no 41030 store, FP, or output result to the comparison. Those are observed
by rerunning 41030 and its complete ordinary closure on physical i386.
"""
import hashlib
import json
import struct

from oracle_effect_light_snapshot import Snapshot
from unicorn import UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R

SNAPSHOT = 'analysis/multiplayer-shading/world-effects-goal-20261011/xbox-tu15-frag-capture-v7'
SNAPSHOT_SHA = '1120cd1b7c4838c7b30fc3504a0c233d537e03147a7b5c368a0a4b4fa7b5ff5c'
FIXTURE_SHA = '738c12df27512dc845c5ee479c5380bb590939204324a125181f9f596a950a68'
EMIT = 0x41030


def pack(*words):
    return struct.pack('<'+'I'*len(words), *words)


def words(raw):
    return struct.unpack('<'+'I'*(len(raw)//4), raw)


def collect(image, root, sp, stop):
    original = Snapshot(root/SNAPSHOT)
    if hashlib.sha256(original.ram).hexdigest() != SNAPSHOT_SHA:
        raise ValueError('Wrong Lockout command snapshot')
    fixture_path = root/'tests/fixtures/tu15_full_light_commands.original.json'
    fixture_bytes = fixture_path.read_bytes()
    if hashlib.sha256(fixture_bytes).hexdigest() != FIXTURE_SHA:
        raise ValueError('Wrong original Lockout command fixture')
    fixture = json.loads(fixture_bytes)
    section = image.section('.text')
    if original.read(section.va, section.rsize) != image.read(section.va, section.rsize):
        raise ValueError('Lockout snapshot has different TU1.5 code')
    text_lo, text_hi = section.va & ~4095, (section.va+section.rsize+4095) & ~4095
    guest = original.guest()
    stack, top, sentinel = 0x70000000, 0x700fe000, 0x700fff00
    guest.mem_map(stack, 0x100000)

    def read(at, n):
        for page in range(at & ~4095, (at+n+4095) & ~4095, 4096):
            if not any(lo <= page <= hi for lo, hi, _ in guest.mem_regions()):
                guest.mem_map(page, 4096)
                guest.mem_write(page, original.read(page, 4096))
        return bytes(guest.mem_read(at, n))

    def call(entry, end=sentinel, args=()):
        guest.mem_write(top, pack(sentinel, *args))
        guest.reg_write(R.UC_X86_REG_ESP, top)
        guest.reg_write(R.UC_X86_REG_FPCW, 0x23f)
        guest.reg_write(R.UC_X86_REG_MXCSR, 0x1fa0)
        guest.emu_start(entry, end, count=30000000)
        if guest.reg_read(R.UC_X86_REG_EIP) != end:
            raise ValueError('Original command input discovery did not return')

    selected = False
    active = None
    rows, layout = [], set()
    relocated_packet = sp+0x80

    def code(g, pc, size, _):
        nonlocal active
        if not selected:
            return
        if active is not None and pc == active['return_pc']:
            data = active.pop('pages')
            active.pop('return_pc')
            layout.update(data)
            ranges = []
            for at, raw in sorted(data.items()):
                if ranges and ranges[-1][0]+len(ranges[-1][1]) == at:
                    ranges[-1][1].extend(raw)
                else:
                    ranges.append((at, bytearray(raw)))
            active['banks'] += [dict(address=at, before=raw.hex()) for at, raw in ranges]
            rows.append(active)
            active = None
        if pc != EMIT:
            return
        if active is not None:
            raise ValueError('Unexpected nested original 41030')
        here = g.reg_read(R.UC_X86_REG_ESP)
        ret, mode, coverage, packet, shader, grouped, mask, add = words(read(here, 32))
        if not stack <= packet <= stack+0x100000-44:
            raise ValueError('Original 41030 caller packet is not in its original stack')
        raw_stack = bytearray(0x20000)
        raw_stack[sp-0x62000000+12:sp-0x62000000+32] = pack(relocated_packet, shader, grouped, mask, add)
        raw_stack[relocated_packet-0x62000000:relocated_packet-0x62000000+44] = read(packet, 44)
        regs = [g.reg_read(getattr(R, 'UC_X86_REG_'+name)) for name in ('EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI')]
        regs[4] = sp
        unit, part = regs[2], regs[1]
        flags = words(read(0x4c6958+32*unit, 8))
        kind = 'model' if flags[1] != 0xffffffff else 'instance' if flags[0] & 0x1000 else 'cluster'
        active = dict(name=f'lockout-secondary-{kind}-u{unit}-p{part}', entry=EMIT,
                      gp=regs, matrix=mode, source=coverage, stop_pc=stop,
                      args=[unit, part, mode, coverage, relocated_packet, shader, grouped, mask, add],
                      before_fp=[g.reg_read(getattr(R, 'UC_X86_REG_'+reg)) for reg in ('FPCW','FPSW','MXCSR')],
                      live_x87=[], max_steps=10000, kind=kind, pages={}, return_pc=ret,
                      banks=[dict(address=0x62000000, before=raw_stack.hex())])

    def memory(g, access, at, n, value, _):
        if active is None:
            return
        for page in range(at & ~4095, (at+n+4095) & ~4095, 4096):
            if text_lo <= page < text_hi or stack <= page < stack+0x100000:
                continue
            if page not in active['pages']:
                active['pages'][page] = read(page, 4096)

    guest.hook_add(UC_HOOK_CODE, code)
    guest.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, memory)
    call(0xc35d0)
    call(0x31c70, 0x31177)
    for i in (1, 2):
        at = 0x4ce268+24*i+4
        read(at, 12)
        guest.mem_write(at, bytes(12))
    context = words(read(0x4cd3a0, 4))[0]
    count, prefix = words(read(0x4cd398, 8))
    mode = words(read(context+0x2a78, 4))[0]
    if (mode, prefix, count) != (fixture['mode'], fixture['prefix'], fixture['count']):
        raise ValueError('Original collector inputs differ from the retained Lockout fixture')
    selected = True
    call(0x44ab0, args=(mode, 0xffffffff, 31))
    if active is not None or len(rows) != fixture['calls'].count(0x41040):
        raise ValueError('Original 41030 invocation coverage differs from the retained Lockout fixture')
    kinds = {k: sum(r['kind']==k for r in rows) for k in ('cluster', 'instance', 'model')}
    return rows, layout, dict(snapshot=SNAPSHOT, sha256=SNAPSHOT_SHA, fixture_sha256=FIXTURE_SHA,
                              mode=mode, prefix=prefix, units=count, calls=len(rows), kinds=kinds)
