#!/usr/bin/env python3
"""Compare complete native PRT helpers with the unchanged English TU1.5 image.

All requests, raw banks and comparison streams live in HALO2_TEST_SCRATCH.
The small receipt records cases, source identities and exact comparison hashes.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import random
import re
import struct
import subprocess
import sys

PIN = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
BASES, SIZES = (0x46a000, 0x61000000, 0x62000000, 0x449000), (4096, 20480, 4096, 4096)


def physical_original(image, rows, scratch, *, entries=None, regions=None, trace_calls=None, return_x87=False, trace_extended=False, preserve_x87_prefix=False):
    """Run unchanged TU1.5 instructions on physical i386, including SSE flags.

    Explicit metadata for PUSH's implicit memory operand lets the existing
    single-step observer retain the complete ordered stack/store sequence.
    """
    root = Path(__file__).resolve().parent.parent
    sys.path.insert(0, str(root / 'tools'))
    import oracle_native_campaign_screen_lifecycle_tu15 as common
    common.ENTRIES = entries or (0x23520, 0x23580, 0x23c80, 0x1456f0, 0x145d50, 0x145e60, 0x148b20, 0x22c10, 0x22730, 0x146100)
    regions = tuple(regions or zip(BASES, SIZES))
    common.geometry.STOP = 0x63000000
    metadata = common.metadata(image)
    if trace_calls is not None:
        metadata['trace_calls'] = trace_calls
    code = common._Code(image, '.text')
    for site in metadata['sites']:
        instruction = code.at(site['instruction'])
        mnemonic = instruction.mnemonic
        if any(prefix in (0xf2, 0xf3) for prefix in instruction.prefix) and mnemonic.startswith('rep'):
            site['repeat_ecx'] = True
        if mnemonic in ('push', 'call'):
            site['operands'].append(dict(base=4, index=-1, scale=1,
                displacement=0xfffffffc, bytes=4, read=False, write=True))
        # Capstone omits/mislabels x87 floating and integer memory stores.
        # The authenticated mnemonic and its real memory operand own the store.
        if mnemonic in ('fstp', 'fst', 'fistp', 'fist', 'fisttp'):
            for operand in site['operands']:
                operand.update(read=False, write=True)
    directory = scratch / 'physical-original'
    directory.mkdir()
    section = image.section('.text')
    (directory / 'text.bin').write_bytes(image.section_bytes(section))
    (directory / 'metadata.json').write_text(json.dumps(metadata))
    assembly = f'.code32\n.section .original,"ax"\n.incbin "{directory / "text.bin"}"\n'
    for index, (_, size) in enumerate(regions):
        assembly += f'.section .bank{index},"aw"\n.space {size}\n'
    assembly += '.section .stop,"ax"\nint3\n.section .bridge,"ax"\n.globl _start\n_start:\nfninit\nint3\njmp _start\n'
    (directory / 'reference.s').write_text(assembly)
    linker = f'ENTRY(_start)\nSECTIONS{{.original 0x{section.va:x}:{{*(.original)}} '
    linker += ''.join(f'.bank{index} 0x{base:x}:{{*(.bank{index})}} ' for index, (base, _) in enumerate(regions))
    linker += '.stop 0x63000000:{*(.stop)} .bridge 0x70000000:{*(.bridge)}}\n'
    (directory / 'link.ld').write_text(linker)
    observer_source = root / 'tools/observe_native_campaign_ai_vector_math_tu15.cpp'
    observer_text = observer_source.read_text()
    diagnostic = 'check(it!=sites.end(),"Original geometry escaped authenticated leaf bodies");'
    if observer_text.count(diagnostic) != 1:
        raise ValueError('Physical PRT observer diagnostic boundary changed')
    observer_text = observer_text.replace(diagnostic,
        'if(it==sites.end())std::cerr<<"case "<<c.value("name",std::string{})'
        '<<" unknown original PC0x"<<std::hex<<pc<<"\\n";' + diagnostic)
    incomplete = 'check(finished||!boundary.is_null(),"Original geometry did not reach its declared boundary");'
    if observer_text.count(incomplete) != 1:
        raise ValueError('Physical PRT observer completion diagnostic changed')
    observer_text = observer_text.replace(incomplete,
        'if(!finished&&boundary.is_null())std::cerr<<"case "<<c.value("name",std::string{})'
        '<<" original step budget exhausted\\n";' + incomplete)
    if preserve_x87_prefix:
        if return_x87:raise ValueError('FP-neutral prefix preservation cannot qualify a returned-ST0 owner')
        topology='if(finished){check(fp.cwd==entry_fp.cwd&&((fp.swd^entry_fp.swd)&0x3800u)==0&&fp.ftw==entry_fp.ftw,"Original path start changed live x87 topology");'
        qualification='{"live_x87_unchanged",finished}'
        if observer_text.count(topology)!=1 or observer_text.count(qualification)!=1:
            raise ValueError('Physical PRT FP-neutral prefix boundary changed')
        observer_text=observer_text.replace(topology,topology.replace('if(finished){','{',1))
        observer_text=observer_text.replace(qualification,'{"live_x87_unchanged",true},{"prefix_x87_preservation_checked",!finished}')
    if trace_calls is not None:
        marker = 'J observed{{"entry",pc},{"arguments",args}};'
        fp_marker = 'observed["xmm"]=std::move(lanes);'
        if observer_text.count(marker) != 1 or observer_text.count(fp_marker) != 1:
            raise ValueError('Physical PRT observer genuine-call trace boundary changed')
        observer_text = observer_text.replace(marker, marker +
            'observed["eflags"]=std::uint32_t(regs.eflags)&~0x100u;')
        trace_fp='observed["fp"]=J::array({current.cwd,current.swd,current.mxcsr});'
        if trace_extended:
            trace_fp+=('const auto observed_depth=std::int64_t(depth)+trace.value("x87_delta",0);'
                'check(observed_depth>=0&&observed_depth<=8,"Original x87 trace depth exceeds register file");'
                'observed["live_x87"]=J::array();for(unsigned slot=0;slot<unsigned(observed_depth);++slot)'
                'observed["live_x87"].push_back(hex(std::span(reinterpret_cast<const std::byte*>(current.st_space)+slot*16,10)));')
        observer_text = observer_text.replace(fp_marker,fp_marker+trace_fp)
    if return_x87:
        topology = 'if(finished){check(fp.cwd==entry_fp.cwd&&((fp.swd^entry_fp.swd)&0x3800u)==0&&fp.ftw==entry_fp.ftw,"Original path start changed live x87 topology");for(unsigned n=0;n<depth;++n)check(!std::memcmp(reinterpret_cast<const std::byte*>(fp.st_space)+n*16,reinterpret_cast<const std::byte*>(entry_fp.st_space)+n*16,10),"Original path start changed a live x87 input");}'
        if observer_text.count(topology) != 1:
            raise ValueError('Physical PRT observer returned-x87 topology boundary changed')
        observer_text = observer_text.replace(topology,
            'if(finished){const auto returned_top=(top+7u)&7u;'
            'check(fp.cwd==entry_fp.cwd&&((fp.swd>>11u)&7u)==returned_top&&'
            'fp.ftw==(entry_fp.ftw|(1u<<returned_top)),"Original math return changed x87 topology");'
            'for(unsigned n=0;n<depth;++n)check(!std::memcmp('
            'reinterpret_cast<const std::byte*>(fp.st_space)+(n+1u)*16,'
            'reinterpret_cast<const std::byte*>(entry_fp.st_space)+n*16,10),'
            '"Original math return changed a live x87 input");}')
        result_marker = 'if(c.contains("stop_before_entries"))result.back()["stopped_at"]=std::uint32_t(regs.rip);'
        if observer_text.count(result_marker) != 1:
            raise ValueError('Physical PRT observer returned-x87 result boundary changed')
        observer_text = observer_text.replace(result_marker,
            'result.back()["returned_extended"]=hex(std::span(reinterpret_cast<const std::byte*>(fp.st_space),10));'
            + result_marker)
    private_observer = directory / 'observer.cpp'
    private_observer.write_text(observer_text)
    commands = [
        ['as', '--32', str(directory / 'reference.s'), '-o', str(directory / 'reference.o')],
        ['ld', '-melf_i386', '-T', str(directory / 'link.ld'), str(directory / 'reference.o'), '-o', str(directory / 'reference')],
        ['c++', '-std=c++20', '-O2', str(private_observer), '-o', str(directory / 'observer')],
    ]
    for command in commands:
        result = subprocess.run(command, capture_output=True, text=True)
        if result.returncode:
            raise ValueError('Physical PRT reference build failed: ' + result.stderr[-3000:])
    (directory / 'inputs.json').write_text(json.dumps(rows, separators=(',', ':')))
    command = [str(directory / 'observer'), str(directory / 'reference'), str(directory / 'metadata.json'),
               str(directory / 'inputs.json'), str(directory / 'observed.json')]
    result = subprocess.run(command, capture_output=True, text=True, timeout=90)
    if result.returncode:
        raise ValueError('Physical PRT original failed: ' + (result.stdout + result.stderr)[-4000:])
    proof = json.loads((directory / 'observed.json').read_text())
    if proof['instruction_patches'] or proof['supplied_original_outcomes']:
        raise ValueError('Physical PRT reference supplied instructions or outcomes')
    return proof, dict(backend='physical-unchanged-i386-offline', cases=len(rows),
        original_instructions=proof['original_instructions'], owners=metadata['owners'],
        observer_sha256=hashlib.sha256(observer_source.read_bytes()).hexdigest(),
        diagnostic_observer_sha256=hashlib.sha256(private_observer.read_bytes()).hexdigest(),
        metadata_sha256=hashlib.sha256((directory / 'metadata.json').read_bytes()).hexdigest())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe', type=Path, required=True)
    parser.add_argument('--native', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    if sys.platform != 'linux' or platform.machine().lower() not in ('x86_64', 'amd64', 'i386', 'i686'):
        print('SKIP physical original PRT reference requires Linux x86 ptrace')
        return 77
    if not args.xbe.is_file():
        print('SKIP missing owned English TU1.5 executable')
        return 77
    if hashlib.sha256(args.xbe.read_bytes()).hexdigest() != PIN:
        raise ValueError('Present executable is not authenticated English TU1.5')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    sys.path.insert(0, str(Path(__file__).resolve().parent / 'reference/halo2-decompiled/tools'))
    from xbe import Xbe
    image = Xbe(args.xbe)
    rng = random.Random(0x2352023580)
    cases = []
    # Zero/negative signed16 counts, clamping, wrapping products, data aliasing,
    # saved-register aliasing, and a live stack stride modified by the output.
    for count in (-32768, -1, 0, 1, 2, 3, 16, 64):
        for stride in (0, 1, 0xffffffff, 0x80000001):
            cases.append((0x23520, count, stride, 'separate'))
    for count in (1, 3, 16):
        cases.append((0x23520, count, 0xfffffffd, 'descriptor-overlap'))
    cases += [(0x23520, 1, 0xffffffff, 'saved-register-overlap'),
              (0x23520, 3, 0xffffffff, 'stride-slot-overlap')]
    # Full raw-word transforms include signaling NaNs and signed zero; no
    # floating-point arithmetic is authorized in this packing operation.
    for delta in (0x1000, -48, -4, 0, 4, 16, 44, 48):
        for _ in range(8):
            cases.append((0x23580, delta, 0, 'transform-overlap'))
    for delta in (-16, -4, 0, 4):
        cases.append((0x23580, delta, 0, 'constant-overlap'))
    for rows in (-32768, -1, 0, 1, 2, 4):
        for components in (-1, 0, 1, 2, 4):
            cases.append((0x23c80, rows, components, 'projection-finite'))
    for rows in (1, 3):
        for components in (0, 1, 2):
            for label in ('projection-edges', 'projection-source-alias', 'projection-coeff-alias'):
                cases.append((0x23c80, rows, components, label))
    cases.append((0x23c80, 1, 0, 'projection-stack-alias'))
    cases.append((0x23c80, 3, 2, 'projection-descriptor-alias'))
    for order in (0, 1, 2, 4, -1, -3, -32768):
        cases.append((0x23c80, 2, 1, f'projection-order-{order}'))
    for delta in (0, -4, 4, 16, 0x1000):
        for determinant in (0, 0x80000000, 1, 0x3f800000, 0xbf800000, 0x7f800000,
                            0x7fc12345, 0x7f812345):
            cases.append((0x1456f0, delta, determinant, 'inverse-finite'))
    for label in ('inverse-edges', 'inverse-backward'):
        for delta in (0, -4, 4, 0x1000):
            cases.append((0x1456f0, delta, 0x3f800000, label))
    for size in (*range(10), 16, 32):
        for label in ('square-finite', 'square-matrix-alias', 'square-vector-alias'):
            cases.append((0x145d50, size, 0, label))
    for size in (1, 2, 4, 5, 6, 8, 9, 16):
        cases.append((0x145d50, size, 0, 'square-edges'))
        for delta in (-4, 4):
            cases.append((0x145d50, size, delta, 'square-matrix-alias'))
            cases.append((0x145d50, size, delta, 'square-vector-alias'))
    cases.append((0x145d50, 2, 0, 'square-stack-alias'))
    for order in (0, 1, 2, 3, 4, 8, 0xffffffff):
        for delta in (0x1000, 0, 4, -4):
            cases.append((0x145e60, order, delta, 'basis-finite'))
    for order in (2, 3, 4):
        for special in (0, 0x80000000, 1, 0x80000001, 0x7f800000, 0xff800000,
                        0x7fc12345, 0x7f812345, 0x7f7fffff):
            cases.append((0x145e60, order, special, 'basis-edges'))
    for control_word in (0x7f, 0x27f, 0x37f, 0x67f, 0xa7f, 0xe7f):
        cases.append((0x145e60, 4, control_word, 'basis-controls'))
    for order in (0, 1, 2, 3, 4, 8, 0xffffffff):
        for outputs in range(4):
            cases.append((0x148b20, order, outputs, 'scaled-finite'))
    for order in (2, 3, 4):
        for label in ('scaled-input-alias', 'scaled-output-alias', 'scaled-stack-alias'):
            cases.append((0x148b20, order, 3, label))
        for weight in (0, 0x80000000, 1, 0x7f800000, 0xff800000, 0x7fc12345, 0x7f812345):
            cases.append((0x148b20, order, weight, 'scaled-edges'))
    for witness in range(16):
        cases.append((0x22c10, witness, 0, 'point-weight-finite'))
    for edge in (0, 0x80000000, 1, 0x7f800000, 0xff800000, 0x7fc12345, 0x7f812345, 0x7f7fffff):
        cases.append((0x22c10, edge, 0, 'point-weight-edges'))
    for a in (-4, 0, 4):
        for b in (-4, 0, 4):
            cases.append((0x22c10, a, b, 'point-weight-alias'))
    cases.append((0x22c10, 0, 0, 'point-weight-stack'))
    for control_word in (0x7f, 0x27f, 0x37f, 0x67f, 0xa7f, 0xe7f):
        cases.append((0x22c10, 0, control_word, 'point-weight-controls'))
    for order in (0, 1, 2, 3, 4, 8, 0xffffffff):
        for label in ('coefficient-finite', 'coefficient-source-alias', 'coefficient-reference-alias'):
            cases.append((0x22730, order, 0x3f000000, label))
    for order in (1, 2, 3, 4):
        for factor in (0, 0x80000000, 1, 0x3f800000, 0xbf800000, 0x7f800000, 0x7fc12345, 0x7f812345):
            cases.append((0x22730, order, factor, 'coefficient-edges'))
    cases.append((0x22730, 3, 0x3f000000, 'coefficient-stack-alias'))
    for order in (0, 1, 2, 3, 4, 8, 0xffffffff):
        for label in ('rotation-identity', 'rotation-finite', 'rotation-source-alias', 'rotation-matrix-alias'):
            cases.append((0x146100, order, 0, label))
    for edge in (0, 0x80000000, 1, 0x80000001, 0x7f800000, 0xff800000, 0x7fc12345, 0x7f812345):
        for order in (2, 3, 4):
            cases.append((0x146100, order, edge, 'rotation-edges'))
    for control_word in (0x7f, 0x27f, 0x37f, 0x67f, 0xa7f, 0xe7f):
        cases.append((0x146100, 4, control_word, 'rotation-controls'))
    requests, expected = scratch / 'prt.in', scratch / 'prt.expected'
    case_receipts, rows = [], []
    with requests.open('wb') as inputs:
        for ordinal, (current_entry, value, stride, label) in enumerate(cases):
            banks = [bytearray(rng.randbytes(size)) for size in SIZES]
            gp = [rng.getrandbits(32) for _ in range(8)]
            gp[4] = stack = 0x62000800
            flags = 0x202 | (rng.getrandbits(12) & 0x8d5)
            control = 0x1f80 | ((ordinal % 4) << 13) | (0x8040 if ordinal & 4 else 0)
            control_word = stride if label in ('basis-controls', 'point-weight-controls', 'rotation-controls') else 0x27f
            status = 0x200 if current_entry == 0x146100 and ordinal & 1 else 0
            xmm = [rng.getrandbits(128) for _ in range(8)]
            struct.pack_into('<I', banks[2], 0x800, 0x63000000)
            if current_entry == 0x23520:
                gp[1] = 0x61000400
                gp[3] = rng.getrandbits(16) << 16 | (value & 0xffff)
                output = {'separate': 0x61001000,
                          'descriptor-overlap': gp[1],
                          'saved-register-overlap': stack - 0x54,
                          'stride-slot-overlap': stack - 4}[label]
                struct.pack_into('<II', banks[2], 0x804, output, stride)
                for section in range(max(0, value)):
                    struct.pack_into('<H', banks[1], 0x406 + section * 4,
                                     (0, 1, 2, 3, 65535)[section % 5])
            elif current_entry == 0x23580:
                gp[1] = 0x61000800
                gp[0] = (0x46a0d0 if label == 'constant-overlap' else gp[1]) + value
            elif current_entry == 0x23c80:
                gp[0], gp[2] = 0x61000400, 0x61000800
                order = int(label.removeprefix('projection-order-')) if label.startswith('projection-order-') else 3
                struct.pack_into('<3h', banks[1], 0x400, order, value, stride)
                # The entire projection source and each SH vector have
                # declared raw inputs; output aliases retain natural writes.
                for offset in range(0x800, 0x3800, 4):
                    struct.pack_into('<f', banks[1], offset, rng.uniform(-20, 20))
                if label == 'projection-edges':
                    edges = (0, 0x80000000, 0x7f800000, 0xff800000,
                             1, 0x80000001, 0x7fc12345, 0x7f812345)
                    for offset in range(0x800, 0x3800, 4):
                        struct.pack_into('<I', banks[1], offset, edges[(offset // 4) % len(edges)])
                if label == 'projection-descriptor-alias':
                    banks[1][0x800:0x3800] = bytes(0x3000)
                output = {'projection-source-alias': gp[2] + 4,
                          'projection-coeff-alias': 0x61003000,
                          'projection-stack-alias': stack - 0x54 + 0x38,
                          'projection-descriptor-alias': gp[0]}.get(label, 0x61002000)
                struct.pack_into('<4I', banks[2], 0x804,
                                 0x61003000, 0x61003040, 0x61003080, output)
            elif current_entry == 0x1456f0:
                gp[0] = 0x61000800
                output = gp[0] + value
                struct.pack_into('<2I', banks[2], 0x804, stride, output)
                struct.pack_into('<I', banks[0], 0xd0, 0x3f800000)
                if label == 'inverse-backward':
                    flags |= 1024
                for offset in range(0x780, 0x880, 4):
                    value_bits = struct.unpack('<I', struct.pack('<f', rng.uniform(-3, 3)))[0]
                    if label == 'inverse-edges':
                        value_bits = (0, 0x80000000, 1, 0x7f812345, 0x7fc54321,
                                      0x7f800000, 0xff800000, 0x3f800000, 0xbf800000)[(offset // 4) % 9]
                    struct.pack_into('<I', banks[1], offset, value_bits)
            elif current_entry == 0x145d50:
                gp[3] = value
                output = {'square-matrix-alias': 0x61000800 + stride,
                          'square-vector-alias': 0x61003000 + stride,
                          'square-stack-alias': stack - 0x14}.get(label, 0x61002000)
                gp[0] = output
                struct.pack_into('<2I', banks[2], 0x804, 0x61000800, 0x61003000)
                for offset in range(0x780, 0x3800, 4):
                    value_bits = struct.unpack('<I', struct.pack('<f', rng.uniform(-3, 3)))[0]
                    if label == 'square-edges':
                        value_bits = (0, 0x80000000, 1, 0x7f812345, 0x7fc54321,
                                      0x7f800000, 0xff800000, 0x3f800000, 0xbf800000)[(offset // 4) % 9]
                    struct.pack_into('<I', banks[1], offset, value_bits)
            elif current_entry == 0x145e60:
                banks[0][:] = image.read(BASES[0], SIZES[0])
                gp[1], gp[2] = 0x61000800, value
                gp[0] = gp[1] + stride if label == 'basis-finite' else 0x61001000
                for axis in range(3):
                    struct.pack_into('<f', banks[1], 0x800 + axis * 4, rng.uniform(-1, 1))
                if label == 'basis-edges':
                    struct.pack_into('<I', banks[1], 0x800, stride)
            elif current_entry == 0x148b20:
                banks[0][:] = image.read(BASES[0], SIZES[0])
                gp[1], gp[2] = 0x61000800, value
                outputs = stride if label == 'scaled-finite' else 3
                for axis in range(3):
                    struct.pack_into('<f', banks[1], 0x800 + axis * 4, rng.uniform(-1, 1))
                weights = [struct.unpack('<I', struct.pack('<f', rng.uniform(-2, 2)))[0] for _ in range(3)]
                if label == 'scaled-edges':
                    weights[0] = stride
                red = 0x61001000
                green = 0x61001200 if outputs & 1 else 0
                blue = 0x61001400 if outputs & 2 else 0
                if label == 'scaled-input-alias':
                    red = gp[1] + 4
                if label == 'scaled-output-alias':
                    green, blue = red + 4, red
                if label == 'scaled-stack-alias':
                    # Keep the genuine caller return slot intact. Sixteen
                    # coefficients can overlap the saved BX below the basis.
                    red = stack - 0x50 + (0x0c if value >= 4 else 0x14)
                struct.pack_into('<6I', banks[2], 0x804, *weights, red, green, blue)
            elif current_entry == 0x22c10:
                banks[0][:] = image.read(BASES[0], SIZES[0])
                gp[0], gp[1], gp[2] = 0x61000800, 0x61000820, 0x61000840
                for offset in range(0x7c0, 0x880, 4):
                    struct.pack_into('<f', banks[1], offset, rng.uniform(-10, 10))
                if label == 'point-weight-finite' and value < 8:
                    struct.pack_into('<3I', banks[1], 0x800, 0x38d1b713 + value, 0, 0)
                    struct.pack_into('<3I', banks[1], 0x820, 0, 0, 0)
                if label == 'point-weight-edges':
                    struct.pack_into('<I', banks[1], 0x800, value)
                if label == 'point-weight-alias':
                    gp[1], gp[2] = gp[0] + value, gp[0] + stride
                if label == 'point-weight-stack':
                    gp[0] = stack - 12
                    struct.pack_into('<3f', banks[2], 0x7f4, 1.2, -0.7, 0.3)
            elif current_entry == 0x22730:
                gp[0], gp[1] = value, 0x61000800
                output = {'coefficient-source-alias': gp[1] + 4,
                          'coefficient-reference-alias': 0x61001004,
                          'coefficient-stack-alias': stack - 56}.get(label, 0x61002000)
                struct.pack_into('<2I', banks[2], 0x804, 0x61001000, output)
                xmm[0] = (xmm[0] & ~0xffffffff) | stride
                for offset in range(0x7c0, 0x1400, 4):
                    value_bits = struct.unpack('<I', struct.pack('<f', rng.uniform(-10, 10)))[0]
                    if label == 'coefficient-edges':
                        value_bits = (0, 0x80000000, 1, 0x7f812345, 0x7fc54321,
                                      0x7f800000, 0xff800000, 0x3f800000, 0xbf800000)[(offset // 4) % 9]
                    struct.pack_into('<I', banks[1], offset, value_bits)
            else:
                banks[0][:] = image.read(BASES[0], SIZES[0])
                banks[3][:] = image.read(BASES[3], SIZES[3])
                gp[0], gp[1], gp[6] = 0x61000800, 0x61001000, 0x61002000
                struct.pack_into('<I', banks[2], 0x804, value)
                for offset in range(0x800, 0x1040, 4):
                    struct.pack_into('<f', banks[1], offset, rng.uniform(-1, 1))
                if label == 'rotation-identity':
                    struct.pack_into('<9f', banks[1], 0x800, 1, 0, 0, 0, 1, 0, 0, 0, 1)
                if label == 'rotation-edges':
                    struct.pack_into('<I', banks[1], 0x804, stride)
                if label == 'rotation-source-alias':
                    gp[6] = gp[1] + 4
                if label == 'rotation-matrix-alias':
                    gp[6] = gp[0] - 4
            inputs.write(struct.pack('<I12I', current_entry, *gp, flags, control, control_word, status))
            inputs.write(b''.join(v.to_bytes(16, 'little') for v in xmm))
            inputs.write(b''.join(banks))
            matrix, source = struct.unpack_from('<2I', banks[2], 0x804)
            rows.append(dict(id=ordinal, name=f'{ordinal}-{current_entry:x}-{label}', entry=current_entry,
                gp=gp, eflags=flags, matrix=matrix, source=source, max_steps=100000,
                xmm=[list(struct.unpack('<4I', value.to_bytes(16, 'little'))) for value in xmm],
                before_fp=[control_word, status, control], live_x87=[],
                banks=[dict(address=base, before=bank.hex()) for base, bank in zip(BASES, banks)]))
            case_receipts.append(dict(entry=f'{current_entry:x}', case=label, value=value,
                                      stride=stride, mxcsr=control, x87_control_word=control_word, x87_status=status))
    physical, physical_receipt = physical_original(image, rows, scratch)
    with expected.open('wb') as outputs:
        for case, original in zip(case_receipts, physical['cases'], strict=True):
            if not original['completed'] or not original['live_x87_unchanged']:
                raise ValueError('Physical PRT original did not complete with its live x87 preserved')
            start = outputs.tell()
            outputs.write(struct.pack('<10I', *original['boundary_gp'], original['boundary_eflags'], original['after_fp'][2]))
            outputs.write(struct.pack('<2I', original['after_fp'][1], original['after_fp'][0]))
            for lane in original['boundary_xmm']:
                outputs.write(struct.pack('<4I', *lane))
            outputs.write(b''.join(bytes.fromhex(bank['after']) for bank in original['banks']))
            outputs.write(struct.pack('<I', len(original['writes'])))
            for event in original['writes']:
                before, written = bytes.fromhex(event['before']), bytes.fromhex(event['written'])
                outputs.write(struct.pack('<III', event['instruction'], event['address'], len(written)) +
                              before.ljust(4, b'\0') + written.ljust(4, b'\0'))
            case.update(stores=len(original['writes']), retired_instructions=original['original_instructions'],
                        stream_start=start, stream_bytes=outputs.tell() - start)
    actual = scratch / 'prt.actual'
    with requests.open('rb') as inputs, actual.open('wb') as outputs:
        subprocess.run([str(args.native)], stdin=inputs, stdout=outputs, check=True)
    def digest(path):
        with path.open('rb') as stream:
            return hashlib.file_digest(stream, 'sha256').hexdigest()
    if actual.stat().st_size != expected.stat().st_size or digest(actual) != digest(expected):
        left, right = actual.read_bytes(), expected.read_bytes()
        first = next((i for i, pair in enumerate(zip(left, right)) if pair[0] != pair[1]), min(len(left), len(right)))
        case = next((case for case in case_receipts if case['stream_start'] <= first <
                     case['stream_start'] + case['stream_bytes']), {})
        raise ValueError(f'Native PRT mismatch at byte {first}: {case}; local='
                         f'{first - case.get("stream_start", 0)}; native='
                         f'{left[first:first+16].hex()}; original={right[first:first+16].hex()}')
    root = Path(__file__).resolve().parent.parent
    sys.path.insert(0, str(root / 'tools'))
    from model_prt_native_replacements import replace_prt_helpers, binding_prefix
    generated = (root / 'src/rendering/models/source_model_render_tu15_generated.cpp').read_text()
    if replace_prt_helpers(generated) != generated:
        raise ValueError('Production model helper default-native binding is absent or not reproducible')
    import generate_native_tu15_prt_rotation as rotation
    polynomial = rotation.generate(generated)
    if polynomial != rotation.OUTPUT.read_text():
        raise ValueError('Generated native SH rotation polynomial is stale')
    if any(token in polynomial for token in ('s.gp[', 's.xmm[', 's.fp.', 's.flags.', 'services.instruction', 'goto ')):
        raise ValueError('Native SH polynomial retained virtual CPU execution')
    from model_prt_native_replacements import BODY_SHA256
    literal = generated
    for entry in BODY_SHA256:
        pattern = rf'static void run_{entry:x}\(WorldState &s\) \{{.*?\n\}}'
        body = re.search(pattern, literal, re.S).group()
        rewritten = re.sub(r'selected\((0x[0-9a-f]+u),s.image\)', r'\1', body)
        literal = literal.replace(body, rewritten, 1)
    if replace_prt_helpers(literal) != literal or rotation.generate(literal) != polynomial:
        raise ValueError('Authenticated literal TU1.5 helper binding/lowering changed')
    rejected_polynomial_mutations = 0
    for shape in (generated, literal):
        rotation_body = re.search(r'static void run_146100\(WorldState &s\) \{.*?\n\}', shape, re.S).group()
        for candidate in (rotation_body, rotation_body.replace(binding_prefix(0x146100), '', 1)):
            mutated = candidate.replace('_mm_mul_ss(', '_mm_add_ss(', 1)
            try:
                rotation.generate(shape.replace(rotation_body, mutated, 1))
            except ValueError:
                rejected_polynomial_mutations += 1
            else:
                raise ValueError('Native SH polynomial accepted altered arithmetic')
    rejected_semantic_mutations = 0
    from model_prt_native_replacements import BODY_SHA256, binding_prefix
    for shape in (generated, literal):
        for entry in BODY_SHA256:
            pattern = rf'static void run_{entry:x}\(WorldState &s\) \{{.*?\n\}}'
            body = re.search(pattern, shape, re.S).group()
            prefix = binding_prefix(entry)
            for candidate in (body, body.replace(prefix, '', 1)):
                # Change an operand width while retaining all instruction sites.
                mutated = candidate.replace(',4,0)', ',2,0)', 1)
                if mutated == candidate:
                    raise ValueError('PRT semantic mutation did not change a source operation')
                try:
                    replace_prt_helpers(shape.replace(body, mutated, 1))
                except ValueError:
                    rejected_semantic_mutations += 1
                else:
                    raise ValueError('PRT native generator accepted a changed semantic body')
    rejected_mapping_mutations = 0
    for old, new in (('case 0x0u:return 0x0u;', 'case 0x0u:return 0x1u;'),
                     ('{if(image==SourcePhysicsContactImage::MapPack11)', '{if(p==0)return 1u;if(image==SourcePhysicsContactImage::MapPack11)')):
        changed = generated.replace(old, new, 1)
        if changed == generated: raise ValueError('Model operand-map mutation witness missing')
        try: replace_prt_helpers(changed)
        except ValueError: rejected_mapping_mutations += 1
        else: raise ValueError('PRT helper accepted altered selected identity semantics')
    source_paths = ('src/rendering/models/native_tu15_prt_helpers.hpp',
                    'src/rendering/models/native_tu15_prt_projection.hpp',
                    'src/rendering/models/native_tu15_prt_matrix_inverse.hpp',
                    'src/rendering/models/native_tu15_prt_matrix_vector.hpp',
                    'src/rendering/models/native_tu15_prt_basis.hpp',
                    'src/rendering/models/native_tu15_prt_scaled_basis.hpp',
                    'src/rendering/models/native_tu15_prt_interpolation.hpp',
                    'src/rendering/models/native_tu15_prt_rotation_types.hpp',
                    'src/rendering/models/native_tu15_prt_rotation_bridge.hpp',
                    'src/rendering/models/native_tu15_prt_rotation_polynomial.hpp',
                    'tools/generate_native_tu15_prt_rotation.py',
                    'src/rendering/models/source_model_render_tu15_generated.cpp',
                    'tools/generate_model_render_tu15.py',
                    'tools/model_prt_native_replacements.py',
                    'tests/native_tu15_prt_helpers_tests.cpp',
                    'tools/validate_native_tu15_prt_helpers.py')
    args.report.write_text(json.dumps(dict(passed=True, cases=case_receipts,
        original_xbe_sha256=PIN, comparison_sha256=digest(actual),
        compared_bytes=actual.stat().st_size, native_instruction_callbacks=0,
        validation_scope='complete normal-return state and ordered stores',
        memory_fault_prefix_abi_verified=False, ordered_read_fault_semantics_verified=False,
        rejected_semantic_mutations=rejected_semantic_mutations,
        rejected_polynomial_mutations=rejected_polynomial_mutations,
        rejected_mapping_mutations=rejected_mapping_mutations,
        physical_original=physical_receipt,
        source_sha256={path: digest(root / path) for path in source_paths},
        instruction_patches=[], supplied_results=[], visual_parity_verified=False), indent=2, sort_keys=True) + '\n')
    print(f'PASS {len(cases)} physical unchanged-TU1.5 complete PRT cases: GP/flags/XMM/MXCSR, all banks and ordered stores')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
