#!/usr/bin/env python3
"""Bounded genuine diagnostic-owner provenance and unchanged integer formatting.

Run inside tools/run_test.py. The formatter probe installs source inputs only;
it never supplies a callee result or resumes a stopped original prefix.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(ROOT / 'tools'), str(ROOT / 'tools/reference/halo2-decompiled/tools')]
from xbe import Xbe
from validate_native_tu15_prt_helpers import physical_original, PIN

MAP = 'src/runtime/source_map_world_support_tu15_generated.cpp'
PRIMARY = 'src/physics/physics_live_support_tu15_generated.cpp'
WORLD = 'src/physics/runtime/physics_world_runtime_generated.cpp'
MEMORY = 'src/platform/native_tu15_world_memory.cpp'
DEFINITION = re.compile(r'^static void (run_[0-9a-f]+)\([^;\n]* &s\) \{.*?^\}', re.M | re.S)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def provenance(image):
    sources = {path: (ROOT / path).read_bytes() for path in (MAP, PRIMARY, WORLD, MEMORY)}
    definitions = {path: {m[1]: m[0] for m in DEFINITION.finditer(raw.decode())}
                   for path, raw in sources.items() if path != MEMORY}
    closure, pending = set(), ['run_32ba80']
    while pending:
        name = pending.pop()
        if name in closure:
            continue
        body = definitions[PRIMARY][name]
        closure.add(name)
        pending += list(set(re.findall(r'\b(run_[0-9a-f]+)\(s\)', body)) - closure)
    entries = tuple(sorted(int(name[4:], 16) for name in closure))
    tables = {hex(address): [hex(value) for value in struct.unpack('<' + 'I' * count,
                  image.read(address, count * 4))]
              for address, count in ((0x41be00, 3), (0x41be0c, 4), (0x41be1c, 7))}
    expected = {
        '0x41be00': ['0x2e16f0', '0x2e16d0', '0x2e16e0'],
        '0x41be0c': ['0x2e42f0', '0x2e1770', '0x2e17d0', '0x2e1a40'],
        '0x41be1c': ['0x2e17a0', '0x325d80', '0x2e1c80', '0x31d400',
                     '0x2e38c0', '0x8d140', '0x1cb030'],
    }
    if tables != expected:
        raise ValueError('Authored diagnostic factory/sink interface drift')
    if image.read(0x48c99c, 4) != bytes(4):
        raise ValueError('Diagnostic singleton is not loader-zeroed in the original')
    literal = image.read(0x41c4d4, 19).split(b'\0', 1)[0]
    if literal != b'unknown command %i\n':
        raise ValueError('Diagnostic parent literal drift')
    selected = definitions[MAP]
    owners = {}
    for entry in (0x2e3080, 0x2e30e0, 0x2e2d10, 0x2e2e00):
        body = selected[f'run_{entry:x}']
        owners[hex(entry)] = dict(body_sha256=digest(body.encode()),
            literal_sites=len(re.findall(r's\.services\.instruction\(', body)))
    return dict(source_sha256={path: digest(raw) for path, raw in sources.items()},
        raw_owner_bodies=owners, original_image_sha256=PIN,
        literal=dict(address='41c4d4', text=literal.decode()),
        cold_singleton=dict(address='48c99c', value=0),
        authored_vtables=tables,
        constructor_chain=['2e2800', '2e15f0', '2e3110', '2e1770'],
        real_default_targets=dict(logger_stream='2e16d0', append='2e1c80', flush='31d400'),
        default_sink_services=dict(debugger='33bdd9 -> INT 2D at33be04',
                                   stdout='32b47e ->32d258/stdout stream46e680'),
        retained_formatter_direct_closure=[dict(entry=hex(entry),
            body_sha256=digest(definitions[PRIMARY][f'run_{entry:x}'].encode()))
            for entry in entries],
        remaining=['The generic32BA80 formatter remains retained; the integer probe is not its whole native closure.',
                   'The genuine default sink debugger/stdout services need real host ownership; no normal service result is assumed.',
                   'Cold/uninitialized singleton faults remain genuine source behavior.',
                   'Older-image diagnostic owner variants remain unqualified.']), entries


def formatter_probe(image, entries, scratch):
    regions = tuple((section.va, section.vsize) for section in image.sections
                    if section.name in ('.rdata', '.data')) + ((0x61000000, 16384), (0x62000000, 32768))
    seed = [image.read(base, size) if base < 0x60000000 else bytes(size) for base, size in regions]
    rows = []
    for index, value in enumerate((0, 1, 127, 255, 1073741824, 2147483647, 0x80000000, 0xffffffff)):
        banks = [bytearray(data) for data in seed]
        sp = 0x62007000 + index
        destination, argument = 0x61001000 + index, 0x61000200 + index
        def put(address, data):
            for (base, size), bank in zip(regions, banks):
                if base <= address <= base + size - len(data):
                    bank[address - base:address - base + len(data)] = data
                    return
            raise ValueError('Unbacked original formatter input')
        put(argument, struct.pack('<I', value))
        put(sp, struct.pack('<5I', 0x63000000, destination, 1024, 0x41c4d4, argument))
        gp = [0x12340000 + 0x101 * n + index for n in range(8)]
        gp[4] = sp
        rows.append(dict(id=index, name=f'original-integer-{value:x}', entry=0x32ba80,
            gp=gp, eflags=0x202, matrix=destination, source=1024, max_steps=20000,
            before_fp=[0x27f, 0, 0x1f80], live_x87=[],
            banks=[dict(address=base, before=bytes(bank).hex())
                   for (base, _), bank in zip(regions, banks)]))
    proof, authority = physical_original(image, rows, scratch,
        entries=entries, regions=regions, trace_extended=True,
        trace_calls=[dict(entry=entry, gp=True, xmm=True) for entry in entries])
    results = []
    for row, case in zip(rows, proof['cases'], strict=True):
        bank = bytes.fromhex(case['banks'][-2]['after'])
        offset = row['matrix'] - regions[-2][0]
        output = bank[offset:offset + 1024].split(b'\0', 1)[0]
        value = struct.unpack('<i', bytes.fromhex(row['banks'][-2]['before'])[
            (0x61000200 + row['id']) - regions[-2][0]:
            (0x61000200 + row['id']) - regions[-2][0] + 4])[0]
        expected = f'unknown command {value}\n'.encode()
        if output != expected or case['boundary_gp'][0] != len(output):
            raise ValueError('Genuine integer formatter did not compute its expected output')
        results.append(dict(name=row['name'], bytes=len(output), output=output.decode(),
            return_value=case['boundary_gp'][0], executed_sites=len(case['executed_sites']),
            genuine_callees=sorted({hex(call['entry']) for call in case['calls']})))
    return dict(passed=True, scope='Unchanged-i386 real32BA80 signed integer formatting normal returns; no supplied outcomes.',
        authority=authority, cases=results,
        executed_sites=sorted({hex(pc) for case in proof['cases'] for pc in case['executed_sites']}))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--probe-formatter', action='store_true')
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    image = Xbe(args.xbe)
    if digest(image.data) != PIN:
        raise ValueError('Diagnostic image identity differs')
    report, entries = provenance(image)
    report.update(passed=True, private_unbound=True, owned_scratch=str(scratch), retained_large_artifacts=[])
    if args.probe_formatter:
        try:
            report['integer_formatter_probe'] = formatter_probe(image, entries, scratch)
        except Exception as error:
            report.update(passed=False, integer_formatter_probe=dict(passed=False, error=str(error)[-6000:]))
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(passed=report['passed'], report=str(args.report), owned_scratch=str(scratch))))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
