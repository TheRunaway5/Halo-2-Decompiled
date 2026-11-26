#!/usr/bin/env python3
"""Compare native World 1CBE70 with its translated and physical i386 owners."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import zlib

import native_collection_filter_world_replacements as binding
import native_world_leaves_remap_sites as remaps
import native_world_leaves_replacements as world


ROOT = Path(__file__).resolve().parents[1]
UNIT = ROOT / 'src/physics/runtime/physics_world_runtime_generated.cpp'
LIVE = ROOT / 'src/physics/physics_live_support_tu15_generated.cpp'
TEST = ROOT / 'tests/native_tu15_collection_filter_world_tests.cpp'
HEADER = ROOT / 'src/physics/runtime/native_tu15_collection_filter_world.hpp'
FIXTURE = ROOT / 'tests/fixtures/workshop_collection_filter_tu15.json.zlib'
ORACLE = ROOT / 'tools/original_workshop_collection_filter_host_x86.cpp'
IMAGE_PIN = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
MODES = ('', 'census', 'verify', *remaps.MODES)
MUTANTS = (
    ('payload-order', 'port.push(0x1cbf98u, gp[edx]);', 'port.push(0x1cbf98u, gp[eax]);'),
    ('group-offset', '0xfffffff4u, 4);', '0xfffffff0u, 4);'),
    ('shape-kind', 'flags.minus(gp[eax], 23u, 4);', 'flags.minus(gp[eax], 24u, 4);'),
    ('user-flag', 'gp[esi] + 31u, 1)', 'gp[esi] + 30u, 1)'),
    ('missing-body', 'gp[ecx] + 32u, 4), 4);', 'gp[ecx] + 28u, 4), 4);'),
    ('return-pop', 'port.ret(returned, 24u);', 'port.ret(returned, 20u);'),
)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def prepare(scratch, xbe):
    sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
    from xbe import Xbe

    image = Xbe(xbe)
    if sha(image.data) != IMAGE_PIN:
        raise ValueError('Wrong TU1.5 image for World collection filter')
    fixture = json.loads(zlib.decompress(FIXTURE.read_bytes()))
    if fixture['xbe_sha256'] != IMAGE_PIN or not fixture['main_body_all_sites_executed']:
        raise ValueError('World collection fixture provenance drift')

    source = UNIT.read_text()
    canonical = world.body(source, '1c6c30')
    child = world.body(source, '2e5e20')
    if not binding.admitted(source, world):
        raise ValueError('World collection filter source bodies drifted')
    sites = re.findall(r'world_observation_pc\(0x([0-9a-f]+)u,0x([0-9a-f]+)u,s\)', canonical[0])
    unique_sites = {(int(c, 16), int(t, 16)) for c, t in sites}
    if len(unique_sites) != fixture['main_body_sites'] or any(t-c != 0x5240 for c, t in unique_sites):
        raise ValueError('World collection filter observation relocations drifted')
    live = LIVE.read_text()
    contents = [world.body(live, value)[0].replace('WorldState &s', 'ObservedWorldState &s')
                for value in ('306220', '185020')]
    contents += [child[0], canonical[0].replace('run_2e5e20(s);', 'group_filter(s);'), binding.reference(canonical[0])]
    (scratch / 'bodies.inc').write_text('\n'.join(contents) + '\n')
    (scratch / 'resolvers.inc').write_text('\n'.join(
        re.search(r'^std::uint32_t ' + name + r'\(.*$', source if name == 'selected' else live, re.M)[0]
        for name in ('selected', 'canonical_branch')) + '\n')
    tables = image.read(0x185044, 36)
    (scratch / 'tables.inc').write_text(','.join(f'0x{x:02x}u' for x in tables) + '\n')
    methods = []
    for row in fixture['literal_owners']:
        address, value, encoding = row['owner'], row['value'], bytes.fromhex(row['bytes'])
        if image.read(address, len(encoding)) != encoding:
            raise ValueError(f'Constant method drift at {address:x}')
        flags = {b'\x33\xc0': 'zero', b'\x83\xc8': 'ones'}.get(encoding[:2], 'move')
        methods.append(f'{{0x{address:x}u,0x{value:x}u,ResultFlags::{flags}}},')
    (scratch / 'methods.inc').write_text('\n'.join(methods) + '\n')
    remaps.write(scratch)

    packets = []
    for row in fixture['cases']:
        if row['entry'] != 0x1cbe70:
            continue
        bank, negative = bytearray([0xa5])*0x4000, bytearray([0xa5])*4096
        special_body = None
        for address, width, value in row['cells']:
            if 0x2100000 <= address < 0x2104000:
                offset = address - 0x2100000
                bank[offset:offset+width] = value.to_bytes(width, 'little')
            elif 0x90000000 <= address < 0x90001000:
                offset = address - 0x90000000
                negative[offset:offset+width] = value.to_bytes(width, 'little')
            elif address == 0x48b860 and width == 4:
                special_body = value
            else:
                raise ValueError(f'Unexpected collection fixture address {address:x}')
        words = [row['entry'], row['receiver'], row['flags'], special_body, *row['args']]
        packets.append(' '.join(f'{word:x}' for word in words) + f' {bank.hex()} {negative.hex()}')
    path = scratch / 'cases.txt'
    path.write_text('\n'.join(packets) + '\n')
    return path, len(packets), sha(tables), len(unique_sites)


def compile_test(compiler, scratch, name, override=None, sanitize=False):
    target = scratch / name
    command = [compiler, '-std=c++20', '-O2', '-fno-fast-math', '-ffp-contract=off']
    if sanitize:
        command += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer']
    if override:
        command += [f'-I{override}']
    command += [f'-I{ROOT / "include"}', f'-I{ROOT / "src"}', f'-I{scratch}', str(TEST), '-o', str(target)]
    subprocess.run(command, check=True, cwd=scratch)
    return target


def run(binary, cases, mode):
    result = subprocess.run([str(binary), str(cases), mode], text=True, capture_output=True)
    if result.returncode:
        raise RuntimeError(f'{binary.name} ({mode or "literal"}) failed:\n{result.stderr.strip()}')
    return json.loads(result.stdout)


def run_physical(scratch, binary, cases, xbe):
    oracle = scratch / 'original-i386'
    command = ['g++', '-m32', '-msse2', '-O2', '-fomit-frame-pointer', '-fno-pie', '-no-pie', str(ORACLE), '-o', str(oracle)]
    subprocess.run(command, check=True, cwd=scratch)
    with cases.open() as packet_source:
        original = subprocess.check_output([str(oracle), str(xbe)], stdin=packet_source, text=True)
    native = subprocess.check_output([str(binary), str(cases), 'physical'], text=True)
    original_rows, native_rows = original.splitlines(), native.splitlines()
    for index, (a, b) in enumerate(zip(original_rows, native_rows, strict=True)):
        if a != b:
            left, right = a.split(), b.split()
            mismatches = [f'{n} {x[:24]}/{y[:24]}' for n, (x, y) in enumerate(zip(left, right, strict=True)) if x != y]
            raise ValueError(f'Physical collection filter case {index} differs: {", ".join(mismatches)}')
    return len(original_rows), sha(original.encode())


def run_mutants(compiler, scratch, cases):
    rejected = []
    source = HEADER.read_text()
    for name, before, after in MUTANTS:
        override = scratch / name
        header = override / HEADER.relative_to(ROOT / 'src')
        header.parent.mkdir(parents=True)
        if source.count(before) != 1:
            raise ValueError(f'Mutation anchor drift: {name}')
        header.write_text(source.replace(before, after, 1))
        binary = compile_test(compiler, scratch, f'mutant-{name}', override)
        result = subprocess.run([str(binary), str(cases), 'verify'], text=True, capture_output=True)
        if not result.returncode:
            raise ValueError(f'Mutant survived: {name}')
        rejected.append({'name': name, 'diagnostic': result.stderr.strip()[-400:]})
        binary.unlink()
    return rejected


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe', required=True, type=Path)
    parser.add_argument('--report', required=True, type=Path)
    parser.add_argument('--compiler', default='g++')
    parser.add_argument('--smoke', action='store_true')
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Run through tools/run_test.py')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    files = {UNIT, LIVE, TEST, HEADER, FIXTURE, ORACLE, Path(__file__).resolve(),
             Path(binding.__file__).resolve(), Path(world.__file__).resolve(), *remaps.SOURCES}
    def source_hashes():
        return {str(path.relative_to(ROOT)):sha(path.read_bytes()) for path in sorted(files)}
    hashes = source_hashes()
    cases, physical_count, tables, sites = prepare(scratch, args.xbe.resolve())
    binary = compile_test(args.compiler, scratch, 'release')
    results = {mode or 'literal':run(binary, cases, mode) for mode in MODES}
    compared, physical_digest = run_physical(scratch, binary, cases, args.xbe.resolve())
    if compared != physical_count:
        raise ValueError('Incomplete physical collection filter comparison')
    mutants, sanitized = [], {}
    if not args.smoke:
        san = compile_test(args.compiler, scratch, 'sanitized', sanitize=True)
        sanitized = {mode or 'literal':run(san, cases, mode) for mode in ('', 'verify', 'remap-verify')}
        mutants = run_mutants(args.compiler, scratch, cases)
    if hashes != source_hashes():
        raise ValueError('World collection filter sources changed during validation')
    receipt = {'passed':True, 'image_sha256':IMAGE_PIN, 'table_sha256':tables, 'source_sha256':hashes,
               'physical_cases':compared, 'physical_sha256':physical_digest, 'original_sites':sites,
               'results':results, 'sanitized':sanitized, 'mutants':mutants, 'scratch':str(scratch)}
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(receipt, indent=2) + '\n')
    print(f'PASS 1CBE70: {compared} physical comparisons, '
          f'{sum(row["compared"] for row in results.values())} translated comparisons, {len(mutants)} mutants rejected')


if __name__ == '__main__':
    main()
