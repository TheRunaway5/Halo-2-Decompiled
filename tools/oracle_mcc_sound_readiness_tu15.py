#!/usr/bin/env python3
"""Execute unchanged TU1.5 NONE-chunk queries on the host's i386 CPU.

No cache pools, hardware registers, callees or return values are supplied.
Only the original full21F260 body, caller stack and serialized chunk exist.
Any attempted resource allocation/pool access faults instead of succeeding.
"""
import argparse
import hashlib
import json
import struct
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent / 'reference/halo2-decompiled/tools'))
from xbe import Xbe

PIN = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe', required=True, type=Path)
    parser.add_argument('--directory', required=True, type=Path)
    parser.add_argument('--fixture', required=True, type=Path)
    args = parser.parse_args()
    assert hashlib.sha256(args.xbe.read_bytes()).hexdigest() == PIN
    args.directory.mkdir(parents=True, exist_ok=False)
    root = args.directory.resolve()
    image = Xbe(args.xbe)
    original = image.read(0x21f260, 0xff)
    assert original[:3] == bytes.fromhex('83ec08') and original[-3:] == bytes.fromhex('c20c00')
    (root/'original.bin').write_bytes(original)
    cases = [(flags, owner, encoded) for flags in (0, 1, 4, 5, 8, 9, 12, 13)
             for owner, encoded in ((0xffffffff, 36), (0xe12a1234, 0x80012345), (0xf98bffff, 0xc0000000))]
    rows = ['.code32', '.section .bridge,"awx"', '.global _start', '_start:']
    sections = []
    for n, (flags, owner, encoded) in enumerate(cases):
        address = 0x60000000 + n*0x1000
        stack_end, chunk = address+512, address+512
        rows += [f'mov $0x{stack_end:x},%esp', 'push $0x202', 'popfl',
                 f'mov $0x{0xa1000000+n:x},%eax', f'mov $0x{0xb2000000+n:x},%ecx',
                 f'mov $0x{0xc3000000+n:x},%edx', f'mov $0x{0xd4000000+n:x},%ebx',
                 f'mov $0x{0xe5000000+n:x},%ebp', f'mov $0x{0xf6000000+n:x},%esi',
                 f'mov $0x{0x87000000+n:x},%edi', f'push ${flags}', f'push $0x{chunk:x}',
                 f'push $0x{owner:x}', 'call 0x21f260', f'.global returned{n}', f'returned{n}:']
        for i, reg in enumerate(('eax', 'ecx', 'edx', 'ebx', 'esp', 'ebp', 'esi', 'edi')):
            rows.append(f'mov %{reg},output{n}+{i*4}')
        rows += ['mov $capture_end,%esp', f'pushfl', f'popl output{n}+32']
        sections.append(f'.case{n} 0x{address:x} : {{ *(.case{n}) }}')
    for n in range(len(cases)):
        rows += ['mov $4,%eax', 'mov $1,%ebx', f'mov $output{n},%ecx', 'mov $36,%edx', 'int $0x80',
                 'mov $4,%eax', 'mov $1,%ebx', f'mov $case{n},%ecx', 'mov $524,%edx', 'int $0x80']
    rows += ['mov $1,%eax', 'xor %ebx,%ebx', 'int $0x80', '.align 16', 'capture:', '.space 512', 'capture_end:']
    for n, (flags, owner, encoded) in enumerate(cases):
        rows += [f'output{n}:', '.space 36', f'.section .case{n},"aw"', f'case{n}:', '.fill 512,1,0xcd',
                 f'.long {0x123400+n*0x100},{encoded},0xffffffff']
    rows += ['.section .original,"ax"', f'.incbin "{root}/original.bin"']
    (root/'bridge.s').write_text('\n'.join(rows)+'\n')
    (root/'link.ld').write_text('ENTRY(_start)\nSECTIONS {\n.original 0x21f260 : { *(.original) }\n'+
                              '\n'.join(sections)+'\n.bridge 0x70000000 : { *(.bridge) }\n}\n')
    subprocess.run(['as', '--32', str(root/'bridge.s'), '-o', str(root/'bridge.o')], check=True, capture_output=True)
    subprocess.run(['ld', '-melf_i386', '-T', str(root/'link.ld'), str(root/'bridge.o'), '-o', str(root/'original-i386')], check=True, capture_output=True)
    result = subprocess.run([str(root/'original-i386')], check=True, capture_output=True)
    (root/'output.bin').write_bytes(result.stdout)
    symbols = {line.split()[2]: int(line.split()[0], 16) for line in subprocess.check_output(['nm', str(root/'original-i386')], text=True).splitlines() if len(line.split())==3}
    assert len(result.stdout) == len(cases)*560
    output = []
    for n, (flags, owner, encoded) in enumerate(cases):
        at = n*560
        gp = list(struct.unpack_from('<8I', result.stdout, at))
        eflags = struct.unpack_from('<I', result.stdout, at+32)[0]
        actual = result.stdout[at+36:at+560]
        address = 0x60000000+n*0x1000
        before = bytearray(b'\xcd'*512+struct.pack('<III', 0x123400+n*0x100, encoded, 0xffffffff))
        struct.pack_into('<4I', before, 496, symbols[f'returned{n}'], owner, address+512, flags)
        initial = [0xa1000000+n, 0xb2000000+n, 0xc3000000+n, 0xd4000000+n,
                   address+496, 0xe5000000+n, 0xf6000000+n, 0x87000000+n]
        assert gp[0] == 0 and gp[4] == address+512 and actual[512:] == before[512:]
        output.append({'name': f'NONE_flags{flags}_owner{owner:08x}_size{encoded:08x}',
                       'entry':0x21f260, 'input_gp':initial, 'input_eflags':0x202,
                       'original_gp':gp, 'original_eflags':eflags, 'eflags_mask':0xfffffdff,
                       'address':address, 'before':before.hex(), 'after':actual.hex()})
    fixture = {'schema':'H2_MCC_NONE_SOUND_READINESS_TU15_V1', 'original_sha256':PIN,
               'original_bytes':original.hex(), 'original_bytes_sha256':hashlib.sha256(original).hexdigest(),
               'cases':output, 'scope':'Original NONE/non-loading query only; no hardware/load/readiness completion is synthesized'}
    args.fixture.write_text(json.dumps(fixture, indent=2)+'\n')
    print(f'Unchanged original i386 NONE readiness: {len(cases)} GP/RET/flags/stack/chunk captures')


if __name__ == '__main__':
    main()
