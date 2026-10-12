#!/usr/bin/env python3
"""Unchanged TU1.1 BD170/BD270/BD1F0 with actual shared pool and finite heap.

Explicit native allocator inputs and controlled mutations only; no captured
body rows, supplied allocation results, world birth or constructor admission.
"""
import argparse
import hashlib
import json
import random
import struct
from collections import Counter
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
from oracle_mp_title_update_display import ROOT, OLD, NEW, PINS, Xbe, _Code, sha

HEAP, HEADERS, SCRATCH = 0x02000000, 0x02200000, 0x02300000
POOL, STACK, STOP = HEADERS-0x4c, 0x02408000, 0x03000000
NONE = 0xffffffff
ENTRIES = {0x16d070, 0x16d210, 0x16d220, 0x16d2c0, 0x16d4c0,
           0xbd170, 0xbd270, 0xbd1f0, 0x18fc80, 0x18fce0, 0x18fd70,
           0x18fe60, 0x18ff20, 0x18ff90, 0x190030, 0x1900c0, 0x326970}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    (out/'states').mkdir()
    for path in (OLD, NEW):
        assert sha(path) == PINS[str(path)], path
    image = Xbe(OLD)
    code = _Code(image, '.text')
    extent_rows, extent_metadata = [], []
    for side, path, bank in ((1, OLD, 0x46fad0), (2, NEW, 0x474b90)):
        x = Xbe(path)
        pointers = struct.unpack('<13I', x.read(bank, 52))
        for typ, pointer in enumerate(pointers):
            raw = x.read(pointer, 16)
            extent = struct.unpack_from('<h', raw, 8)[0]
            assert extent > 0
            extent_rows.append(f'{side} {typ} {extent}')
            extent_metadata.append(dict(image=side, type=typ, bank=f'{bank:08x}',
                                        record=f'{pointer:08x}', raw16=raw.hex(), extent=extent))
    assert [r['extent'] for r in extent_metadata[:13]] == [r['extent'] for r in extent_metadata[13:]]
    (out/'extents.tsv').write_text('\n'.join(extent_rows)+'\n')
    lines, cases, states, all_executed = [], [], {}, set()

    class Case:
        def __init__(self, name, capacity=16, heap_capacity=4096, initial=0xcd):
            self.name, self.capacity, self.heap_capacity = name, capacity, heap_capacity
            self.g = Uc(UC_ARCH_X86, UC_MODE_32)
            self.g.mem_map(0x10000, 0x800000)
            for section in image.sections:
                self.g.mem_write(section.va, image.section_bytes(section))
            self.g.mem_map(HEAP, 0x500000)
            self.g.mem_map(STOP, 4096)
            self.g.mem_write(HEAP, bytes([initial])*((heap_capacity+0x53)&~3))
            self.g.mem_write(SCRATCH+16, b'objects\0')
            self.g.mem_write(SCRATCH+48, b'object\0')
            self.w(0x4e78d0, POOL)
            self.w(0x4e58bc, HEAP)
            self.rows, self.stores, self.calls = [], [], []
            self.executed = set()
            self.g.hook_add(UC_HOOK_CODE, self.instruction)
            self.g.hook_add(UC_HOOK_MEM_WRITE, self.write)
            self.call(0x16d070, [POOL, SCRATCH+48, 0],
                      {UC_X86_REG_ECX: 0, UC_X86_REG_EDX: 12, UC_X86_REG_EBX: capacity})
            self.call(0x16d210, [], {UC_X86_REG_EAX: POOL})
            assert self.u(POOL+0x44) == HEADERS
            assert self.u(POOL+0x48) == HEADERS+capacity*12
            assert self.u(POOL+0x40, 2) == 0xe26f
            self.call(0x18fc80, [SCRATCH+16, 0], {UC_X86_REG_ESI: HEAP, UC_X86_REG_EBX: heap_capacity})
            lines.append(f'N {name} {capacity} {heap_capacity} {initial}')
            self.rows.append(dict(kind='initialize', calls=self.calls[:], stores=self.stores[:]))
            self.snapshot()

        def u(self, address, n=4):
            return int.from_bytes(self.g.mem_read(address, n), 'little')

        def w(self, address, value, n=4):
            self.g.mem_write(address, value.to_bytes(n, 'little'))

        def instruction(self, g, pc, size, _):
            # Fail at any unmapped/outside original owner, never synthesize a return.
            instruction = code.at(pc)
            assert instruction.bytes == image.read(pc, size) == bytes(g.mem_read(pc, size)), hex(pc)
            self.executed.add(pc)
            if pc in ENTRIES:
                self.calls.append(f'{pc:08x}')

        def write(self, g, kind, address, n, value, _):
            if 0x02400000 <= address < 0x02410000:
                return
            pc = g.reg_read(UC_X86_REG_EIP)
            self.stores.append([f'{pc:08x}', f'{address:08x}', n,
                                bytes(g.mem_read(address, n)).hex(),
                                (value & ((1 << (8*n))-1)).to_bytes(n, 'little').hex()])

        def call(self, entry, stack, registers):
            for register in (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX,
                             UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP):
                self.g.reg_write(register, registers.get(register, 0))
            self.g.reg_write(UC_X86_REG_ESP, STACK)
            self.g.reg_write(UC_X86_REG_EFLAGS, 2)
            self.g.reg_write(UC_X86_REG_FPCW, 0x027f)
            self.g.reg_write(UC_X86_REG_MXCSR, 0x1f80)
            self.g.mem_write(STACK, struct.pack('<'+'I'*(1+len(stack)), STOP, *stack))
            self.g.emu_start(entry, STOP, count=200000)
            assert self.g.reg_read(UC_X86_REG_EIP) == STOP, (self.name, hex(entry))
            return self.g.reg_read(UC_X86_REG_EAX)

        def snapshot(self):
            values = [self.u(POOL+offset) for offset in (0x34, 0x38, 0x3c)] + [self.u(POOL+0x40, 2)]
            raw = struct.pack('<4I', *values)
            raw += bytes(self.g.mem_read(self.u(POOL+0x48), ((self.capacity+31)//32)*4))
            raw += bytes(self.g.mem_read(HEADERS, self.capacity*12))
            raw += bytes(self.g.mem_read(HEAP, (self.heap_capacity+0x53)&~3))
            digest = hashlib.sha256(raw).hexdigest()
            relative = f'states/{digest}.bin'
            if digest not in states:
                (out/relative).write_bytes(raw)
                states[digest] = dict(file=relative, bytes=len(raw))
            lines.append('S '+relative)
            self.rows.append(dict(kind='snapshot', state_sha256=digest))

        def operation(self, kind, *args):
            stores, calls = len(self.stores), len(self.calls)
            result = None
            if kind == 'A':
                extent, = args
                result = self.call(0xbd170, [extent], {})
                line = f'A {extent} {result:08x}'
            elif kind == 'G':
                h, field, n, alignment = args
                result = self.call(0xbd270, [field, n, alignment], {UC_X86_REG_EAX: h}) & 255
                assert result in (0, 1)
                line = f'G {h:08x} {field} {n} {alignment} {result}'
            elif kind == 'F':
                h, = args
                self.call(0xbd1f0, [h], {})
                line = f'F {h:08x}'
            elif kind == 'P':
                h, n, seed = args
                pointer = self.u(HEADERS+(h & 65535)*12+8)
                assert n <= self.u(pointer-16)-16
                self.g.mem_write(pointer, bytes((i*37+seed)&255 for i in range(n)))
                line = f'P {h:08x} {n} {seed}'
            elif kind == 'W':
                h, offset, n, value = args
                self.w(HEADERS+(h & 65535)*12+offset, value, n)
                line = f'W {h:08x} {offset} {n} {value}'
            elif kind == 'M':
                typ, = args
                result = self.call(0x16d2c0, [], {UC_X86_REG_EDX: POOL})
                if result != NONE:
                    self.w(HEADERS+(result & 65535)*12+3, typ, 1)
                line = f'M {typ} {result:08x}'
            elif kind == 'D':
                h, = args
                self.call(0x16d4c0, [], {UC_X86_REG_EDX: POOL, UC_X86_REG_EAX: h})
                line = f'D {h:08x}'
            else:
                raise ValueError(kind)
            lines.append(line)
            self.rows.append(dict(kind=kind, inputs=list(args), result=result,
                                  stores=self.stores[stores:], calls=self.calls[calls:]))
            self.snapshot()
            return result

        def finish(self):
            all_executed.update(self.executed)
            cases.append(dict(name=self.name, pool_capacity=self.capacity, heap_capacity=self.heap_capacity,
                              rows=self.rows, executed_unique=len(self.executed)))

    for typ, record in enumerate(extent_metadata[:13]):
        c = Case(f'actual_type_extent_{typ}', heap_capacity=8192)
        h = c.operation('A', record['extent'])
        c.operation('P', h, ((record['extent']+15)&~15), typ+1)
        c.operation('G', h, 0, 5, typ % 5)
        c.operation('G', h, 4, 0, 255)
        c.operation('W', h, 2, 1, 0x81)
        c.operation('W', h, 3, 1, typ)
        c.operation('F', h)
        c.operation('A', record['extent'])
        c.finish()
    c = Case('header_exhaustion', capacity=2, heap_capacity=512)
    first = c.operation('A', 32)
    c.operation('A', 32)
    assert c.operation('A', 32) == NONE
    c.operation('F', first)
    c.operation('A', 32)
    c.finish()
    c = Case('failed_base_and_zero_extent', capacity=4, heap_capacity=64)
    assert c.operation('A', 1124) == NONE
    h = c.operation('A', 0)
    c.operation('F', h)
    c.operation('A', 48)
    assert c.operation('A', 4) == NONE
    c.finish()
    c = Case('relocation_and_physical_padding', heap_capacity=1024)
    h = c.operation('A', 65)
    c.operation('P', h, 80, 11)
    c.operation('A', 80)
    c.operation('G', h, 0, 128, 2)
    c.operation('G', h, 4, 0, 31)
    c.operation('G', h, 8, 3, 33)
    c.finish()
    c = Case('failed_append_retains_old_body', heap_capacity=256)
    h = c.operation('A', 64)
    c.operation('P', h, 64, 17)
    c.operation('A', 160)
    assert c.operation('G', h, 0, 16, 4) == 0
    c.finish()
    c = Case('shared_category_namespace', capacity=4, heap_capacity=512)
    h = c.operation('M', 0)
    b = c.operation('A', 64)
    c.operation('M', 5)
    c.operation('D', h)
    c.operation('A', 80)
    c.operation('F', b)
    c.operation('M', 2)
    c.finish()
    c = Case('exact_startup_heap_capacity', capacity=2048, heap_capacity=0x100000)
    h = c.operation('A', 1124)
    c.operation('A', 604)
    c.operation('G', h, 0, 2048, 4)
    c.operation('F', h)
    c.finish()
    rng = random.Random(0xBD170BD270)
    for n in range(64):
        c = Case(f'bounded_stream_{n:02}', capacity=(4, 8, 16)[n % 3],
                 heap_capacity=(256, 512, 1024, 4096)[n % 4], initial=(0, 0x57, 0xcd)[n % 3])
        live = []
        for _ in range(24):
            draw = rng.randrange(100)
            if not live or draw < 45:
                h = c.operation('A', rng.choice((4, 15, 16, 17, 31, 32, 65, 128, 316, 1124)))
                if h != NONE:
                    live.append(h)
            elif draw < 65:
                h = rng.choice(live)
                c.operation('F', h)
                live.remove(h)
            elif draw < 85:
                h = rng.choice(live)
                c.operation('G', h, 0, rng.choice((0, 1, 3, 16, 31, 64, 256)), rng.choice((0, 1, 2, 3, 4, 32, 33, 34)))
            else:
                h = rng.choice(live)
                pointer = c.u(HEADERS+(h & 65535)*12+8)
                c.operation('P', h, c.u(pointer-16)-16, rng.randrange(256))
        for h in live:
            c.operation('F', h)
        c.finish()
    (out/'storage.tsv').write_text('\n'.join(lines)+'\n')
    listing = ''.join(f'{pc:08x} {code.at(pc).bytes.hex()} {code.at(pc).mnemonic} {code.at(pc).op_str}\n'
                      for pc in sorted(all_executed))
    (out/'executed.asm').write_text(listing)
    for path in (OLD, NEW):
        assert sha(path) == PINS[str(path)]
    counts = Counter(entry for c in cases for r in c['rows'] for entry in r.get('calls', []))
    snapshot_count = sum(r['kind'] == 'snapshot' for c in cases for r in c['rows'])
    report = dict(schema='original-source-object-body-storage-v1', tool_sha256=sha(__file__), source_pins=PINS,
                  extent_metadata=extent_metadata, cases=cases, states=states,
                  total_snapshots=snapshot_count, actual_entries=dict(sorted(counts.items())),
                  executed_listing_sha256=sha(out/'executed.asm'), storage_fixture_sha256=sha(out/'storage.tsv'),
                  extent_fixture_sha256=sha(out/'extents.tsv'), instruction_patches=[], supplied_allocator_results=[],
                  native_addresses=dict(heap=HEAP, headers=HEADERS, pool=POOL, scratch=SCRATCH),
                  scope=__doc__ + ' Actual complete TU1.1 instructions, including pool initialization and nested finite '
                  'heap owners. Both immutable type banks read directly. u5 dynamic execution, complete constructor, '
                  'world registration and live damage/shield rendering are outside this comparison. Native storage '
                  'retirement composes equivalent raw free/header operations; its observer PCs are not a BD1F0 trace.')
    (out/'report.json').write_text(json.dumps(report, sort_keys=True, indent=2)+'\n')
    print(json.dumps(dict(cases=len(cases), snapshots=snapshot_count, states=len(states),
                          actual_entries=dict(sorted(counts.items())), report_sha256=sha(out/'report.json'),
                          fixture_sha256=sha(out/'storage.tsv'))))


if __name__ == '__main__':
    main()
