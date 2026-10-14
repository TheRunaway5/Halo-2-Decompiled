#!/usr/bin/env python3
"""Execute whole TU1.5 1C310 for authored model stream/register declarations.

Literal declarations come from the owned Xbox cache and unchanged executable.
The relocated tag/context/output/stack banks are explicit test inputs. No
instruction or callee result is replaced. This is declaration evidence, not
live collector, palette production or rendered framebuffer parity.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

from audit_rendering import Cache
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn import x86_const as R

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe

PIN = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
ENTRY, STREAM_TABLE, TAG_TABLE_POINTER = 0x1c310, 0x4494c8, 0x4f0414


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe', type=Path, required=True)
    parser.add_argument('--map', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    assert not args.output.exists(), 'Refusing to overwrite retained original evidence'
    original = Xbe(args.xbe)
    assert sha(original.data) == PIN
    # The entry prefix independently identifies the complete original producer.
    assert original.read(ENTRY, 13).hex() == '8b54240883ec5c5355565733c0'
    cache = Cache(args.map)
    programs = []
    for tag in cache.tags.values():
        if tag['group'] != 'vrtx' or not tag['name'].endswith('\\lightaccum_sphere_diffuse'):
            continue
        _, root = cache.tag(tag['datum'], 'vrtx')
        for classification, at in enumerate(cache.block(root + 4, 28)):
            if classification > 6:
                break
            semantics = [cache.u16(value) for value in cache.block(at + 4, 2)]
            programs.append(dict(datum=tag['datum'], name=tag['name'],
                                 classification=classification, semantics=semantics))
    assert programs and {row['classification'] for row in programs} >= {1, 2, 3, 4, 5, 6}
    vm = Uc(UC_ARCH_X86, UC_MODE_32)
    vm.mem_map(0x10000, 0x600000)
    for section in original.sections:
        vm.mem_write(section.va, original.section_bytes(section))
    vm.mem_map(0x1000000, 0x100000)
    vm.mem_map(0x3000000, 0x1000)
    executed = {}

    def trace(guest, address, size, _):
        raw = bytes(guest.mem_read(address, size))
        assert raw == original.read(address, size)
        executed[address] = raw

    vm.hook_add(UC_HOOK_CODE, trace)
    context, output, table, tag, entry, semantic = [0x1000000 + i * 0x1000 for i in range(6)]
    rows = []
    for program in programs:
        classification = program['classification']
        types = (1, 2) if classification == 1 else (3, 4) if classification in (2, 3) else {
            4: (5, 6), 5: (7, 8), 6: (9, 10)}.get(classification, ())
        for position_type in types:
            streams = [position_type, 25, 27]
            vm.mem_write(context, bytes(0x300))
            vm.mem_write(TAG_TABLE_POINTER, struct.pack('<I', table))
            vm.mem_write(table + 8, struct.pack('<I', tag))
            vm.mem_write(tag + 8, struct.pack('<I', entry))
            vm.mem_write(entry, struct.pack('<III', 0, len(program['semantics']), semantic))
            vm.mem_write(semantic, struct.pack('<' + 'H' * len(program['semantics']), *program['semantics']))
            for index, stream in enumerate(streams):
                vm.mem_write(context + 0x8c + 4 * index, struct.pack('<I', STREAM_TABLE + 21 * stream))
            vm.mem_write(context + 0x2d0, struct.pack('<I', len(streams)))
            stack, stop = 0x10fff00, 0x3000000
            vm.mem_write(stack, struct.pack('<III', stop, context, output))
            vm.reg_write(R.UC_X86_REG_ESP, stack)
            vm.reg_write(R.UC_X86_REG_FPCW, 0x023f)
            vm.reg_write(R.UC_X86_REG_MXCSR, 0x1fa0)
            vm.emu_start(ENTRY, stop, count=100000)
            assert vm.reg_read(R.UC_X86_REG_EIP) == stop
            words = struct.unpack('<64I', bytes(vm.mem_read(output, 256)))
            rows.append(dict(program, stream_types=streams,
                             stream_declarations=[original.read(STREAM_TABLE + 21 * value, 21).hex() for value in streams],
                             registers=[list(words[i:i + 4]) for i in range(0, 64, 4)]))
    cache.close()
    report = dict(schema='tu15-skinned-vertex-declarations-v1', scope=__doc__,
                  original_xbe_sha256=PIN, cache_sha256=sha(args.map.read_bytes()),
                  entry=ENTRY, stream_table=STREAM_TABLE, intercepted_instructions=0,
                  executed_instructions=len(executed),
                  executed_bank_sha256=sha(b''.join(struct.pack('<I', pc) + raw for pc, raw in sorted(executed.items()))),
                  tool_sha256=sha(Path(__file__).read_bytes()), rows=rows)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS', len(rows), 'whole TU1.5 declarations;', len(executed), 'unchanged instructions')


if __name__ == '__main__':
    main()
