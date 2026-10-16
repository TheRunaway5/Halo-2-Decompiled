#!/usr/bin/env python3
"""Observe TU1.5 manager construction/binding with authored opening metadata.

Only declared caller buffers, arguments, the admitted solo30Hz game clock and real cache tag table are
supplied. No callee, resource readiness, RNG result or manager store is replaced.
An unavailable runtime owner is retained as an execution frontier. This direct
caller probe is not complete object birth or an accepted live idle controller.
"""
import argparse
import hashlib
import json
import mmap
from pathlib import Path
import struct

from unicorn import Uc, UcError, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import (UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EAX,
    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EDX, UC_X86_REG_MXCSR, UC_X86_REG_FPCW)

XBE_PIN = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
MAP_PIN = '5093e2f8ef7f0df41c11c6621bac5785a36f78a1cff3b8b2bc6c14b561c02a28'
NONE = 0xffffffff
MANAGER, STACK, STOP = 0x2100000, 0x21fff00, 0x3000000
ENTRIES = {0x1d3180, 0x1d3270, 0x1d3380, 0x1d5320, 0x1d46a0,
    0x1d45a0, 0x1e3420, 0x1d36c0, 0x1d3660, 0x1e58e0,
    0x1ce720, 0x1ce810, 0x126200, 0x1d5790, 0x1d39c0, 0x1e5c80}
OBJECTS = [('regret', 0xe86f06fa), ('truth', 0xe6ac0537),
           ('mercy', 0xe66304ee), ('throne', 0xe96207ed)]


def sha(path):
    with Path(path).open('rb') as source:
        return hashlib.file_digest(source, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe', type=Path, required=True)
    parser.add_argument('--map', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--ticks', type=int, default=0)
    args = parser.parse_args()
    if args.output.exists():
        raise ValueError('Refusing to overwrite original execution evidence')
    if sha(args.xbe) != XBE_PIN or sha(args.map) != MAP_PIN:
        raise ValueError('Expected pinned original TU1.5 English XBE and opening map')
    xbe = args.xbe.read_bytes()
    with args.map.open('rb') as source, mmap.mmap(source.fileno(), 0, access=mmap.ACCESS_READ) as data:
        def u(at):
            return struct.unpack_from('<I', data, at)[0]
        index = u(16)
        table = u(index + 8) - (u(index) - (index + 32))
        magic = u(table + 8) - (index + u(20))
        tags = {u(table + i * 16 + 4): table + i * 16 for i in range(u(index + 24))}
        def root(tag):
            return u(tags[tag] + 8) - magic
        cases = []
        for name, object_tag in OBJECTS:
            model = u(root(object_tag) + 0x38)
            graph = u(root(model) + 0x14)
            for fill in (0, 0xa5):
                guest = Uc(UC_ARCH_X86, UC_MODE_32)
                for address, size in [(0x10000, 0x600000), (MANAGER, 0x100000), (STOP, 4096)]:
                    guest.mem_map(address, size)
                base = struct.unpack_from('<I', xbe, 0x104)[0]
                count, sections = struct.unpack_from('<II', xbe, 0x11c)
                for i in range(count):
                    _, address, _, offset, size = struct.unpack_from('<5I', xbe, sections - base + i * 56)
                    if size:
                        guest.mem_write(address, xbe[offset:offset + size])
                start, end = (index + magic) & ~4095, (len(data) + magic + 4095) & ~4095
                guest.mem_map(start, end - start)
                guest.mem_write(index + magic, data[index:])
                # Same authored sixteen-byte tag records, with original pointers.
                guest.mem_write(0x4f0414, struct.pack('<I', table + magic))
                # The same explicit loaded solo30Hz clock input exercised by
                # oracle_campaign_frame_clock.py; no clock callee is replaced.
                game_clock = struct.pack('<BBHfIfI', 1, 0, 30, 1 / 30, 0, 1, 0)
                guest.mem_write(MANAGER + 0x1000, game_clock)
                guest.mem_write(0x51df54, struct.pack('<I', MANAGER + 0x1000))
                guest.mem_write(MANAGER, bytes([fill]) * 0x90)
                calls, stores, executed = [], [], set()
                def code(g, pc, size, user):
                    executed.add(pc)
                    if pc in ENTRIES:
                        sp = g.reg_read(UC_X86_REG_ESP)
                        calls.append(dict(entry=f'{pc:08x}', caller=bytes(g.mem_read(sp, 4)).hex(),
                            manager=bytes(g.mem_read(MANAGER, 0x90)).hex(),
                            stack=bytes(g.mem_read(sp + 4, 32)).hex()))
                def store(g, access, address, size, value, user):
                    if MANAGER <= address and address + size <= MANAGER + 0x90:
                        stores.append(dict(instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',
                            offset=address - MANAGER, size=size,
                            before=bytes(g.mem_read(address, size)).hex(),
                            value=(value & ((1 << (size * 8)) - 1)).to_bytes(size, 'little').hex()))
                guest.hook_add(UC_HOOK_CODE, code)
                guest.hook_add(UC_HOOK_MEM_WRITE, store)
                def run(entry, arguments, registers):
                    guest.mem_write(STACK, struct.pack('<' + 'I' * (len(arguments) + 1), STOP, *arguments))
                    guest.reg_write(UC_X86_REG_ESP, STACK)
                    guest.reg_write(UC_X86_REG_MXCSR, 0x1f80)
                    guest.reg_write(UC_X86_REG_FPCW, 0x027f)
                    for register, value in registers:
                        guest.reg_write(register, value)
                    error = None
                    try:
                        guest.emu_start(entry, STOP, count=2000000)
                    except UcError as failure:
                        error = str(failure)
                    return dict(entry=f'{entry:08x}', complete=guest.reg_read(UC_X86_REG_EIP) == STOP,
                        pc=f'{guest.reg_read(UC_X86_REG_EIP):08x}', eax=f'{guest.reg_read(UC_X86_REG_EAX):08x}',
                        manager=bytes(guest.mem_read(MANAGER, 0x90)).hex(), error=error)
                constructed = run(0x1d3180, [], [(UC_X86_REG_EAX, MANAGER)])
                if not constructed['complete']:
                    raise ValueError('Original manager constructor failed')
                # B9F6A is the generic object's actual PUSH1 bind argument.
                if bytes(guest.mem_read(0xb9f6a, 2)) != b'\x6a\x01':
                    raise ValueError('Original generic object bind caller differs')
                bound = run(0x1d3380, [1], [(UC_X86_REG_ESI, MANAGER),
                    (UC_X86_REG_EDI, graph), (UC_X86_REG_EDX, model)])
                steps = []
                nodes = struct.unpack_from('<h', data, root(graph) + 0xc)[0]
                for tick in range(args.ticks):
                    call_begin = len(calls)
                    transition = run(0x1d5790, [], [(UC_X86_REG_ESI, MANAGER)])
                    updated = run(0x1d39c0, [0, 0, nodes, 0, 0], [(UC_X86_REG_ESI, MANAGER)])
                    steps.append(dict(tick=tick + 1, transition=transition, updated=updated,
                        variant_calls=sum(c['entry'] == '001d3660' for c in calls[call_begin:])))
                    if not transition['complete'] or not updated['complete']:
                        break
                packed = struct.unpack_from('<I', bytes.fromhex(bound['manager']), 4)[0]
                clip_metadata = '' if packed >> 16 == 0xffff else data[
                    u(root(graph) + 0x30) - magic + (packed >> 16) * 0x6c:
                    u(root(graph) + 0x30) - magic + (packed >> 16) * 0x6c + 0x6c].hex()
                cases.append(dict(name=name, object=f'{object_tag:08x}', model=f'{model:08x}',
                    graph=f'{graph:08x}', caller_buffer_fill=fill, constructed=constructed, bound=bound,
                    node_count=nodes, clip_metadata_6c=clip_metadata,
                    steps=steps, calls=calls, stores=stores, executed_instructions=len(executed)))
                print(name, hex(fill), bound['complete'], bound['pc'], bound['error'],
                    len(steps), 'manager ticks', flush=True)
    report = dict(scope=__doc__, xbe_sha256=XBE_PIN, map_sha256=MAP_PIN,
        tool_sha256=sha(__file__), patched_instructions=[], intercepted_callees=[],
        supplied_runtime_owner_results=[], admitted_game_clock_hex=game_clock.hex(),
        resource_phase='Original image defaults; initialized animation-resource startup is not supplied',
        update_scope='Direct ordinary transition/manager callers; event callback NONE, actual graph node count; inactive local-bank branches are not supplied poses',
        cases=cases)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    main()
