#!/usr/bin/env python3
"""Execute unchanged TU1.5 vector packing and SHORT2 decoding on i386 CPU.

Donor JSONs come from the read-only geometry namespace probe. No original
instruction, callee, operand constant, or result is replaced. The small ELF
harness supplies declared inputs and copies each completed original result.
"""
import argparse
import hashlib
import json
import struct
import subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe, _Code
from generate_physics_live_support_tu15 import complete_body
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def hardware(directory, image, assembly):
    directory.mkdir()
    sections = []
    banks = []
    for number, name in enumerate(('.text', '.rdata')):
        section = image.section(name)
        file = directory/f'original{number}.bin'
        file.write_bytes(image.section_bytes(section))
        banks.append(f'.section .original{number},"ax"\n.incbin "{file.resolve()}"')
        sections.append(f'.original{number} 0x{section.va:x} : {{ *(.original{number}) }}')
    source = '.code32\n.section .bridge,"awx"\n.globl _start\n' + assembly + '\n' + '\n'.join(banks)
    (directory/'bridge.s').write_text(source)
    (directory/'link.ld').write_text('ENTRY(_start)\nSECTIONS {\n'+'\n'.join(sections)+'\n.bridge 0x70000000 : { *(.bridge) }\n}\n')
    subprocess.run(['as', '--32', str(directory/'bridge.s'), '-o', str(directory/'bridge.o')], check=True, capture_output=True)
    subprocess.run(['ld', '-melf_i386', '-T', str(directory/'link.ld'), str(directory/'bridge.o'), '-o', str(directory/'original-i386')], check=True, capture_output=True)
    result = subprocess.run([str(directory/'original-i386')], check=True, capture_output=True)
    (directory/'output.bin').write_bytes(result.stdout)
    return result.stdout


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', required=True, type=Path)
    parser.add_argument('--mcc-donor', required=True, type=Path)
    parser.add_argument('--xbox-donor', required=True, type=Path)
    parser.add_argument('--fixture', required=True, type=Path)
    args = parser.parse_args()
    assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest() == NEW_PIN
    args.directory.mkdir(parents=True, exist_ok=False)
    args.directory = args.directory.resolve()
    image = Xbe(NEW_DEFAULT)
    code = _Code(image, '.text')
    source = {}
    for entry in (0x183af0, 0xd5300):
        body = complete_body(code, entry)
        assert not any(code.at(pc).mnemonic.startswith('call') for pc in body.code)
        source[f'{entry:x}'] = [{'address': pc, 'bytes': code.at(pc).bytes.hex(),
                                'mnemonic': code.at(pc).mnemonic, 'operands': code.at(pc).op_str}
                               for pc in sorted(body.code)]
    mcc = json.loads(args.mcc_donor.read_text())
    xbox = json.loads(args.xbox_donor.read_text())
    vectors, uv = [], []
    assert len(mcc['geometry']) == len(xbox['geometry']) == 29
    for old, new in zip(xbox['geometry'], mcc['geometry']):
        assert (old['holder'], old['index'], old['vertex_count']) == (new['holder'], new['index'], new['vertex_count'])
        old_streams = {s['channel']: s for s in old['streams']}
        new_streams = {s['channel']: s for s in new['streams']}
        assert old_streams[0]['data'] == new_streams[0]['data']
        assert [t['data'] for t in old['tables'] if t['locator'] == 32] == [t['data'] for t in new['tables'] if t['locator'] == 32]
        if 4 in old_streams:
            words = struct.iter_unpack('<I', bytes.fromhex(old_streams[4]['data']))
            inputs = struct.iter_unpack('<3I', bytes.fromhex(new_streams[4]['data']))
            for packed, bits in zip(words, inputs):
                vectors.append({'bits': bits, 'xbox_word': packed[0]})
        if 3 in old_streams:
            words = struct.iter_unpack('<h', bytes.fromhex(old_streams[3]['data']))
            inputs = struct.iter_unpack('<I', bytes.fromhex(new_streams[3]['data']))
            uv.extend({'bits': bits[0], 'xbox_short': packed[0]} for packed, bits in zip(words, inputs))
    assert len(vectors) == 12093 and len(uv) == 28836
    binary = args.directory/'vectors.bin'
    binary.write_bytes(b''.join(struct.pack('<3I', *case['bits']) for case in vectors))
    tail = '''write_output:
 mov $4,%eax
 mov $1,%ebx
 mov $output,%ecx
 mov $OUTPUT_SIZE,%edx
 int $0x80
 cmp $OUTPUT_SIZE,%eax
 jne failed
 xor %ebx,%ebx
 jmp exit
failed: mov $1,%ebx
exit: mov $1,%eax
 int $0x80
.balign 16
controls: .long 0x1f80
output: .space OUTPUT_SIZE
stack: .space 65536
stack_end:
'''
    assembly = f'''_start:
 mov $stack_end,%esp
 ldmxcsr controls
 xor %ebx,%ebx
next:
 imul $12,%ebx,%eax
 lea input(%eax),%edx
 mov $0x183af0,%eax
 call *%eax
 mov %eax,output(,%ebx,4)
 inc %ebx
 cmp ${len(vectors)},%ebx
 jb next
 jmp write_output
input: .incbin "{binary}"
''' + tail.replace('OUTPUT_SIZE', str(len(vectors)*4))
    packed = hardware(args.directory/'vectors', image, assembly)
    assert len(packed) == len(vectors)*4
    for case, result in zip(vectors, struct.iter_unpack('<I', packed)):
        case['original_word'] = result[0]
        assert result[0] == case['xbox_word']
    mesh = bytearray(64)
    # Fixed bridge bank addresses are linker labels, supplied below in ASM.
    assembly = '''_start:
 mov $stack_end,%esp
 ldmxcsr controls
 mov $triangles,mesh+0x24
 mov $blocks,mesh+0x3c
 mov $uv,blocks+0x28
 xor %ebp,%ebp
next:
 lea -32768(%ebp),%eax
 mov %ax,uv
 mov %ax,uv+2
 lea output(,%ebp,8),%ebx
 lea 4(%ebx),%edi
 xor %eax,%eax
 mov $mesh,%ecx
 push $0
 push $0
 mov $0xd5300,%edx
 call *%edx
 inc %ebp
 cmp $65536,%ebp
 jb next
 jmp write_output
.balign 16
mesh: .space 64
blocks: .space 32
 .byte 31
 .space 31
triangles: .short 0,0,0
uv: .short 0,0
''' + tail.replace('OUTPUT_SIZE', str(65536*8))
    decoded = hardware(args.directory/'short2', image, assembly)
    assert len(decoded) == 65536*8
    short_bits = []
    for x, y in struct.iter_unpack('<II', decoded):
        assert x == y
        short_bits.append(x)
    fixture = {'original_sha256': NEW_PIN, 'source': source, 'incident': vectors,
               'lightmap_uv': uv, 'original_short2_midpoint_bits': short_bits,
               'authority': 'Unchanged original183AF0 and D5300 executed on Linux i386 CPU; not Xbox hardware. UV is a source-decoder inverse, not an identified shipped UV compiler.'}
    args.fixture.write_text(json.dumps(fixture, indent=2)+'\n')
    print('PASS 12093 original packed vectors and65536 genuine SHORT2 decoder results')


if __name__ == '__main__':
    main()
