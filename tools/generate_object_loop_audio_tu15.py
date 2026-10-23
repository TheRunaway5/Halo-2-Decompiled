#!/usr/bin/env python3
"""Lower complete TU1.5 object-loop/audio-frame CPU owners.

This recovers instructions, not device readiness or voice results. Run under
tools/run_test.py; intermediate listings/manifests belong to its scratch owner.
"""
import argparse
import csv
import json
import os
from pathlib import Path
from types import SimpleNamespace
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import generate, sha

ROOT = Path(__file__).resolve().parents[1]
ENTRIES = (0x18d390, 0x18d3d0, 0x18ee50, 0x18f900, 0x18e970, 0x18ed80, 0x129dd0,
           0x18d5a0, 0x18f9b0, 0x190460, 0x1904d0, 0x18ffb0,
           0x220640, 0x220c60, 0x18dc70, 0x18d960, 0x190a80,
           0x129170, 0x18f510, 0x253070, 0x22c560, 0x223da0)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--directory', type=Path, required=True)
    parser.add_argument('--report-output', type=Path)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    if not args.directory.resolve().is_relative_to(scratch):
        parser.error('Intermediate recovery must stay under HALO2_TEST_SCRATCH')
    assert sha(NEW_DEFAULT) == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    code = _Code(image, '.text')
    # Literal initial bytes of the separately mounted sound-visibility prefix.
    # Actual18F900 supplies its clear/publication; this supplies no mask bits.
    assert image.read(0x55d730, 64) == bytes(64)
    # Genuine13A9B0 pre-object audio phase and both loop-update call sites.
    for pc, target in ((0x13a9d8, 0x18ee50), (0x18ef76, 0x18d5a0),
                       (0x18f01d, 0x18d5a0), (0x18d7fa, 0x220c60),
                       (0x18d93b, 0x220c60)):
        instruction = code.at(pc)
        assert instruction.mnemonic == 'call' and instruction.operands[0].imm == target
    inventory = ROOT / 'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv'
    starts = {int(row['address'], 16) for row in csv.DictReader(inventory.open(), delimiter='\t')}
    sites, instructions, addresses, routines = {}, {}, {}, []
    args.directory.mkdir(parents=True, exist_ok=False)
    for entry in ENTRIES:
        body = _trace(code, entry, starts | set(ENTRIES))
        assert body.code and all(width in (1, 4) for _, width, _ in body.tables)
        sites[f'{entry:08x}'] = sorted(body.code)
        (args.directory / f'{entry:08x}.asm').write_text(''.join(
            f'{pc:08x} {code.at(pc).bytes.hex()} {code.at(pc).mnemonic} {code.at(pc).op_str}\n'
            for pc in sorted(body.code)))
        routines.append(dict(entry=entry,
            tables=[dict(disc=q, count=n) for q, width, n in body.tables if width == 4],
            byte_tables=[dict(address=q, width=width, count=n, raw_hex=image.read(q, width*n).hex())
                         for q, width, n in body.tables if width == 1]))
        for pc in sorted(body.code):
            instruction = code.at(pc)
            assert image.read(pc, instruction.size) == bytes(instruction.bytes)
            instructions[f'{pc:08x}'] = pc
            addresses[pc] = pc
            addresses[pc+instruction.size] = pc+instruction.size
            for operand in instruction.operands:
                if operand.type == 2:
                    addresses[operand.imm & 0xffffffff] = operand.imm & 0xffffffff
                elif operand.type == 3:
                    addresses[operand.mem.disp & 0xffffffff] = operand.mem.disp & 0xffffffff
    paired = dict(image='English TU1.5', original_sha256=NEW_PIN,
        entries={f'{q:08x}': q for q in ENTRIES}, body_sites=sites,
        instruction_map=instructions, address_map={f'{q:08x}': v for q, v in addresses.items()},
        routines=routines)
    correspondence = args.directory / 'bodies.json'
    correspondence.write_text(json.dumps(paired, indent=2, sort_keys=True)+'\n')
    manifest = args.directory / 'generation.json'
    generate(SimpleNamespace(output=args.output, manifest=manifest, correspondence=correspondence,
        additional_entries=Path(__file__)), paired)
    source = args.output.read_text().replace('tools/generate_physics_live_support_tu15.py',
                                            'tools/generate_object_loop_audio_tu15.py')
    source = source.replace('halo2/physics_live_support_tu15.hpp', 'halo2/source_object_loop_audio_tu15.hpp')
    source = source.replace('source_run_physics_live_support_tu15', 'source_run_object_loop_audio_tu15')
    source = source.replace('source_physics_live_support_tu15_owns_entry', 'source_object_loop_audio_tu15_owns_entry')
    # Keep this original call at the common executor boundary, where a native
    # observer can copy its caller-owned location. The public220C60 entry still
    # executes its complete unchanged body and produces the real result.
    internal = 'case 0x220c60u:run_220c60(s);return;'
    assert source.count(internal) == 1
    source = source.replace(internal, 'case 0x220c60u:s.required(site,target,tail);return;')
    # The lowerer emits direct calls between bodies in this closure as well.
    # Those must cross the same observer boundary; replacing only dispatch's
    # switch arm leaves the genuine per-object callers invisible.
    for site in (0x18d7fa, 0x18d93b):
        direct = f's.push(0x{site:x}u,0x{site+5:x}u);run_220c60(s);'
        assert source.count(direct) == 1
        source = source.replace(direct,
            f's.push(0x{site:x}u,0x{site+5:x}u);s.required(0x{site:x}u,0x220c60u,false);')
    temporary = args.output.with_name(args.output.name+'.tmp')
    temporary.write_text(source)
    os.replace(temporary, args.output)
    report = json.loads(manifest.read_text())
    report.update(generated_cpp_sha256=sha(args.output), generator_sha256=sha(Path(__file__)),
                  inventory_sha256=sha(inventory), device_or_voice_results_supplied=False,
                  external_native_dispatch_boundaries=['00220c60'],
                  observed_direct_call_sites=['0018d7fa', '0018d93b'],
                  loader_zero_prefix=dict(address='0055d730', bytes=64,
                                          original_hex=image.read(0x55d730,64).hex()))
    manifest.write_text(json.dumps(report, indent=2, sort_keys=True)+'\n')
    if args.report_output:
        args.report_output.parent.mkdir(parents=True, exist_ok=True)
        args.report_output.write_text(json.dumps(report, indent=2, sort_keys=True)+'\n')


if __name__ == '__main__':
    main()
