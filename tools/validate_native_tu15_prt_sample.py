#!/usr/bin/env python3
"""Qualify the complete native22CD0 transaction against unchanged TU1.5 i386."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import random
import struct
import subprocess
import sys

from validate_native_tu15_prt_helpers import PIN, physical_original

ROOT = Path(__file__).resolve().parent.parent
REGIONS = ((0x46a000, 4096), (0x4c5000, 4096), (0x4ec000, 4096),
           (0x515000, 4096), (0x61000000, 32768), (0x62000000, 8192))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe', type=Path, required=True)
    parser.add_argument('--native', type=Path)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    if sys.platform != 'linux' or platform.machine().lower() not in ('x86_64', 'amd64', 'i386', 'i686'):
        print('SKIP physical PRT sample needs Linux x86 ptrace'); return 77
    if not args.xbe.is_file():
        print('SKIP missing owned English TU1.5 executable'); return 77
    if hashlib.sha256(args.xbe.read_bytes()).hexdigest() != PIN:
        raise ValueError('Wrong English TU1.5 original')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
    from xbe import Xbe
    image = Xbe(args.xbe)
    native = args.native or scratch / 'native-prt-sample'
    if not args.native:
        subprocess.run(['c++', '-std=c++20', '-O2', '-fno-fast-math', '-ffp-contract=off',
                        '-frounding-math', '-I' + str(ROOT / 'include'), '-I' + str(ROOT / 'src'),
                        str(ROOT / 'tests/native_tu15_prt_sample_tests.cpp'), '-o', str(native)], check=True)
    rng = random.Random(0x22cd022c1022730)
    cases = []
    for count in (0, -1, -2147483648):
        for order in (0, 1, 2, 3, 4): cases.append((count, order, 'empty', 0))
    for count in (1, 2, 8):
        for backwards in (0, 1): cases.append((count, 3, 'debug', backwards))
    for order in (0, 1, 2, 3, 4, 5):
        for backwards in (0, 1):
            for alias in ('single', 'output-alias', 'record-alias', 'saved-si-alias'):
                if alias == 'saved-si-alias' and order >= 4 and not backwards:
                    continue  # Larger forward copies overwrite the return PC.
                cases.append((1, order, alias, backwards))
    for count in (2, 3, 4, 5, 7, 8, 9, 16):
        for label in ('linear', 'same-side', 'equal-distance', 'distant', 'random'):
            for order in (0, 1, 3, 4): cases.append((count, order, label, 0))
    for count in (2, 4, 5, 8):
        for label in ('interpolation-output-alias', 'nearest-record-alias', 'incident-record-alias',
                      'saved-bp-alias', 'nan-query', 'nan-record', 'infinite-record', 'near-epsilon'):
            cases.append((count, 3, label, 0))
        cases.append((count, 3, 'same-side', 1))
        for order in (0, 1, 3):
            for backwards in (0, 1):
                cases.append((count, order, 'opposite-copy-stack-query', backwards))
    requests, expected, actual = (scratch / name for name in ('sample.in', 'sample.expected', 'sample.actual'))
    rows, receipts = [], []
    with requests.open('wb') as inputs:
        for ordinal, (count, order, label, backwards) in enumerate(cases):
            banks = [bytearray(rng.randbytes(size)) for _, size in REGIONS]
            banks[0][:] = image.read(REGIONS[0][0], REGIONS[0][1])
            def put(at, value):
                for (base, size), bank in zip(REGIONS, banks):
                    if base <= at <= base + size - 4:
                        struct.pack_into('<I', bank, at - base, value & 0xffffffff); return
                raise ValueError(f'Fixture store outside regions0x{at:x}')
            def fp(at, value): put(at, struct.unpack('<I', struct.pack('<f', value))[0])
            stack = 0x62001000
            context, records, point = 0x61000400, 0x61001000, 0x61000800
            gp = [rng.getrandbits(32) for _ in range(8)]
            gp[2], gp[4] = point, stack
            flags = 0x202 | (rng.getrandbits(12) & 0x8d5) | (0x400 if backwards else 0)
            control = 0x1f80 | ((ordinal % 4) << 13) | (0x8040 if ordinal & 4 else 0)
            control_word = (0x7f, 0x27f, 0x37f)[ordinal % 3]
            xmm = [[rng.getrandbits(32) for _ in range(4)] for _ in range(8)]
            put(0x4ecc14, 0x61000100); put(0x61000184, 0x61000200)
            put(0x61000238, count); put(context + 4, records)
            put(0x515ed4, 1 if label == 'debug' else 0)
            for channel in range(3):
                for coefficient in range(9):
                    put(0x4c5160 + channel * 36 + coefficient * 4, rng.getrandbits(32))
            for component in range(3): fp(point + component * 4, 0)
            for index in range(max(count, 1)):
                record = records + index * 0xdc
                for offset in range(0, 0xdc, 4): fp(record + offset, rng.uniform(-2, 2))
                if label == 'same-side': position = (1.0 + index, 0, 0)
                elif label == 'equal-distance': position = ((-1 if index & 1 else 1), 0, 0)
                elif label == 'distant': position = (2048.0 + index, 2048, 2048)
                elif label == 'random': position = tuple(rng.uniform(-10, 10) for _ in range(3))
                elif label == 'near-epsilon': position = ((index * 2 - 1) * 1e-8, 0, 0)
                else: position = (-1 if index == 0 else 2 + index, 0, 0)
                for component, value in enumerate(position): fp(record + component * 4, value)
            if label == 'nan-query': put(point, 0x7f812345)
            if label == 'nan-record': put(records + 0xdc, 0x7fc12345)
            if label == 'infinite-record': put(records + 0xdc, 0x7f800000)
            red, green, blue, incident = 0x61005000, 0x61005400, 0x61005800, 0x61005c00
            if label in ('output-alias', 'interpolation-output-alias'): green, blue = red + 4, red - 4
            if label in ('record-alias', 'nearest-record-alias'): red = records + 16
            if label == 'incident-record-alias': incident = records + 0xa8
            if label == 'saved-si-alias': red = stack - 0x34
            if label == 'saved-bp-alias': incident = stack - 0x3c
            if label == 'opposite-copy-stack-query':
                # Query aliases the original context argument. Nearest search
                # initially sees its raw pointer as a huge float, retaining
                # index0 beyond the distance cap. Opposite search first stores
                # the cap, then distance1, into that query word. The genuine
                #22C10 reread consequently returns weight>1 and reaches the
                # otherwise uncommon opposite-copy branch without fake results.
                gp[2] = stack + 4
                for index in range(count):
                    fp(records + index * 0xdc, 1048580 if index == 0 else 1048577 + index - 1)
                    put(records + index * 0xdc + 4, red)
                    put(records + index * 0xdc + 8, green)
            for slot, value in enumerate((0x63000000, context, red, green, blue, order, incident)):
                put(stack + slot * 4, value)
            inputs.write(struct.pack('<I11I', 0x22cd0, *gp, flags, control, control_word))
            for lane in xmm: inputs.write(struct.pack('<4I', *lane))
            inputs.write(b''.join(banks))
            rows.append(dict(id=ordinal, name=f'{ordinal}-{count}-{order}-{label}-df{backwards}',
                entry=0x22cd0, gp=gp, eflags=flags, matrix=context, source=red,
                max_steps=100000, xmm=xmm, before_fp=[control_word, 0, control], live_x87=[],
                banks=[dict(address=base, before=bank.hex()) for (base, _), bank in zip(REGIONS, banks)]))
            receipts.append(dict(case=label, count=count, order=order, backwards=backwards,
                                 mxcsr=control, x87_control_word=control_word))
    try:
        physical, physical_receipt = physical_original(image, rows, scratch,
            entries=(0x22cd0, 0x22c10, 0x22730, 0x148b20, 0x145e60), regions=REGIONS)
    except ValueError:
        diagnostic = subprocess.run(['readelf', '-l', str(scratch / 'physical-original/reference')],
                                    capture_output=True, text=True)
        print(diagnostic.stdout[-5000:], file=sys.stderr)
        raise
    with expected.open('wb') as outputs:
        for receipt, original in zip(receipts, physical['cases'], strict=True):
            if not original['completed'] or not original['live_x87_unchanged']:
                raise ValueError('Original PRT sample did not complete: ' + str(receipt))
            start = outputs.tell()
            outputs.write(struct.pack('<12I', *original['boundary_gp'], original['boundary_eflags'],
                original['after_fp'][2], original['after_fp'][1], original['after_fp'][0]))
            for lane in original['boundary_xmm']: outputs.write(struct.pack('<4I', *lane))
            outputs.write(b''.join(bytes.fromhex(bank['after']) for bank in original['banks']))
            outputs.write(struct.pack('<I', len(original['writes'])))
            for event in original['writes']:
                before, written = bytes.fromhex(event['before']), bytes.fromhex(event['written'])
                outputs.write(struct.pack('<III', event['instruction'], event['address'], len(written)) +
                              before.ljust(4, b'\0') + written.ljust(4, b'\0'))
            receipt.update(stores=len(original['writes']), stream_start=start,
                           stream_bytes=outputs.tell() - start, original_instructions=original['original_instructions'])
    with requests.open('rb') as inputs, actual.open('wb') as outputs:
        subprocess.run([str(native)], stdin=inputs, stdout=outputs, check=True)
    digest = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
    if actual.stat().st_size != expected.stat().st_size or digest(actual) != digest(expected):
        left, right = actual.read_bytes(), expected.read_bytes()
        first = next((i for i, (a, b) in enumerate(zip(left, right)) if a != b), min(len(left), len(right)))
        case = next((case for case in receipts if case['stream_start'] <= first <
                     case['stream_start'] + case['stream_bytes']), {})
        raise ValueError(f'Native PRT sample mismatch at{first}, local{first-case.get("stream_start",0)}: '
                         f'{case}; native{left[first:first+16].hex()} original{right[first:first+16].hex()}')
    from model_prt_sample_native_replacements import replace_prt_sample, PREFIX
    generated_path = ROOT / 'src/rendering/models/source_model_render_tu15_generated.cpp'
    generated = generated_path.read_text()
    if replace_prt_sample(generated) != generated:
        raise ValueError('Production22CD0 default native binding absent or irreproducible')
    import re
    body = re.search(r'static void run_22cd0\(WorldState &s\) \{.*?\n\}', generated, re.S).group()
    rejected = 0
    for candidate in (body, body.replace(PREFIX, '', 1)):
        for old, new in (('0x4ecc14u', '0x4ecc18u'), ('0x2cu', '0x28u'),
                         ('if(s.flags.zf||', 'if(!s.flags.zf||')):
            changed = candidate.replace(old, new, 1)
            if changed == candidate: raise ValueError('22CD0 mutation witness absent')
            try: replace_prt_sample(generated.replace(body, changed, 1))
            except ValueError: rejected += 1
            else: raise ValueError('22CD0 binder accepted a changed complete body')
    branch_sites = dict(empty=0x234cf, debug=0x23496, single=0x22d06,
        nearest_bulk=0x22db0, nearest_tail=0x22f00, opposite_bulk=0x22fb0,
        opposite_tail=0x23230, projection_child=0x22c10,
        nearest_copy=0x232ec, opposite_copy=0x2337f, interpolation=0x233d6)
    coverage = {label: sum(site in original['executed_sites'] for original in physical['cases'])
                for label, site in branch_sites.items()}
    if not coverage['opposite_copy']:
        raise ValueError('Required genuine opposite-copy branch was not reached')
    source_paths = ('src/rendering/models/native_tu15_prt_sample.hpp',
                    'tests/native_tu15_prt_sample_tests.cpp', 'tools/validate_native_tu15_prt_sample.py',
                    'tools/validate_native_tu15_prt_helpers.py', 'tools/model_prt_sample_native_replacements.py',
                    'tools/generate_model_render_tu15.py', 'cmake/NativeTu15PrtSample.cmake',
                    'src/rendering/models/source_model_render_tu15_generated.cpp',
                    'src/rendering/models/native_tu15_prt_interpolation.hpp',
                    'src/rendering/models/native_tu15_prt_scaled_basis.hpp',
                    'src/rendering/models/native_tu15_prt_basis.hpp')
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(dict(passed=True, entry='22cd0', cases=receipts,
        physical_original=physical_receipt, original_xbe_sha256=PIN,
        branch_cases=coverage, rejected_semantic_mutations=rejected,
        compared_bytes=actual.stat().st_size, comparison_sha256=digest(actual),
        validation_scope='complete normal-return state and ordered stores',
        memory_fault_prefix_abi_verified=False, ordered_read_fault_semantics_verified=False,
        native_instruction_callbacks=0, instruction_patches=[], supplied_results=[],
        source_sha256={path: digest(ROOT / path) for path in source_paths}), indent=2, sort_keys=True) + '\n')
    print(f'PASS {len(cases)} complete native22CD0 physical-original samples')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
