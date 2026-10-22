#!/usr/bin/env python3
"""Compile the missing whole TU1.5 teleporter update and contact-selection bodies.

The source startup already owns 1A65F0/1A6640. Reuse existing original runtime
services and recover only the direct transitive closure missing from the live
dispatcher. No collision result, pose, event or instruction is substituted.
Generation intermediates belong to tools/run_test.py's scratch directory.
"""
import argparse
import csv
import hashlib
import json
import os
from pathlib import Path
from types import SimpleNamespace

from generate_multiplayer_engines_tu15 import existing_owners
from generate_physics_live_support_tu15 import generate
from oracle_smg_firing_1_1 import ROOT, Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

ROOTS = (0x1a66b0, 0x1a6830, 0x1a9df0)
NATIVE_IO = {0x125870}  # Actual bounded cache-file reads, provided by the native world.
EXPECTED = {0xb2cd0, 0x11ef30, 0x1a6530, 0x1a66b0, 0x1a6830,
            0x1a6930, 0x1a6a40, 0x1a9df0, 0x24eee0, 0x24f090, 0x24f200}


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, required=True)
    parser.add_argument('--output', type=Path, default=ROOT / 'src/gameplay/world/source_teleporter_tu15_generated.cpp')
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    if not args.directory.resolve().is_relative_to(scratch):
        raise ValueError('Teleporter generation intermediates must stay under HALO2_TEST_SCRATCH')
    assert sha(NEW_DEFAULT) == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    code = _Code(image, '.text')
    existing, reused = existing_owners(('source_teleporter_tu15_owns_entry',))
    assert not set(ROOTS) & existing, 'Teleporter update now has another production owner'
    starts = {int(row['address'], 16) for row in csv.DictReader(
        (ROOT / 'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv').open(), delimiter='\t')}
    pending, bodies = set(ROOTS), {}
    while pending:
        entry = min(pending)
        pending.remove(entry)
        body = _trace(code, entry, starts)
        assert body.code and not body.tables
        bodies[entry] = body
        pending |= (body.calls | body.tail_jumps) - existing - bodies.keys() - NATIVE_IO
        assert len(bodies) <= len(EXPECTED), 'Changed teleporter dependency closure'
    assert set(bodies) == EXPECTED, 'Changed original teleporter function closure'
    args.directory.mkdir(parents=True, exist_ok=True)
    sites, instructions, addresses, listings = {}, {}, {}, []
    for entry, body in sorted(bodies.items()):
        text, raw = [], bytearray()
        for pc in sorted(body.code):
            instruction = code.at(pc)
            assert bytes(instruction.bytes) == image.read(pc, instruction.size)
            text.append(f'{pc:08x} {instruction.bytes.hex()} {instruction.mnemonic} {instruction.op_str}\n')
            raw.extend(pc.to_bytes(4, 'little') + bytes(instruction.bytes))
            instructions[f'{pc:08x}'] = pc
            addresses[pc] = pc
            addresses[pc + instruction.size] = pc + instruction.size
            for operand in instruction.operands:
                if operand.type == 2:
                    addresses[operand.imm & 0xffffffff] = operand.imm & 0xffffffff
                elif operand.type == 3:
                    addresses[operand.mem.disp & 0xffffffff] = operand.mem.disp & 0xffffffff
        listing = args.directory / f'{entry:08x}.asm'
        listing.write_text(''.join(text))
        sites[f'{entry:08x}'] = sorted(body.code)
        listings.append(dict(entry=f'{entry:08x}', instructions=len(body.code),
            instruction_bytes_sha256=hashlib.sha256(raw).hexdigest(), listing_sha256=sha(listing),
            direct_callees=[f'{p:08x}' for p in sorted(body.calls)],
            tail_targets=[f'{p:08x}' for p in sorted(body.tail_jumps)]))
    paired = dict(image='English TU1.5', original_sha256=NEW_PIN,
        entries={f'{entry:08x}': entry for entry in bodies}, body_sites=sites,
        instruction_map=instructions, address_map={f'{p:08x}': v for p, v in addresses.items()},
        routines=[dict(entry=entry, tables=[], byte_tables=[]) for entry in bodies])
    correspondence = args.directory / 'bodies.json'
    correspondence.write_text(json.dumps(paired, indent=2, sort_keys=True) + '\n')
    manifest = args.directory / 'generation.json'
    generate(SimpleNamespace(output=args.output, manifest=manifest, correspondence=correspondence,
        additional_entries=Path(__file__)), paired, complete_listings=True, code_override=code,
        runtime_header='halo2/source_teleporter_tu15.hpp', runtime_function='source_run_teleporter_tu15')
    args.output.write_text(args.output.read_text().replace(
        'source_physics_live_support_tu15_owns_entry', 'source_teleporter_tu15_owns_entry').replace(
        'source_run_physics_live_support_tu15', 'source_run_teleporter_tu15'))
    report = json.loads(manifest.read_text())
    report.update(generator_source_sha256=sha(__file__), generated_cpp_sha256=sha(args.output),
        generator_dependencies={name: sha(ROOT / 'tools' / name) for name in
            ('generate_multiplayer_engines_tu15.py', 'generate_physics_live_support_tu15.py')},
        instruction_listings=listings, reused_owner_source_hashes=reused,
        supplied_service_results=[],native_resource_boundaries=['00125870: actual local/shared cache file reads'],
        scope='Whole original teleporter update/query/notice and locale-load bodies; live runtime acceptance remains required')
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    print(json.dumps(dict(bodies=len(bodies), instructions=len(instructions),
                         generated_cpp_sha256=sha(args.output))), flush=True)


if __name__ == '__main__':
    main()
