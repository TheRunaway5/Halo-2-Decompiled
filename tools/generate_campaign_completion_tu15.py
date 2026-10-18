#!/usr/bin/env python3
"""Recover whole original campaign-completion and transition-request CPU bodies.

Profile storage, campaign metadata, statistics, options, clocks and the actual
scenario loader remain separate owners. Recovery does not authorize bypassing
them or treating a queued request as a completed level handoff.
"""
import argparse
import json
from pathlib import Path
from types import SimpleNamespace
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import generate, sha

ENTRIES = (0x13b3c0, 0x13b4a0, 0x191750, 0x193257, 0x192a41, 0x192ac1,
           0x12e490, 0x191580, 0x191fb0, 0x154f80, 0x155190, 0x64fd0, 0x32c0a0)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--directory', type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists() or args.directory.exists():
        raise ValueError('Preserve existing generated source and evidence')
    if sha(NEW_DEFAULT) != NEW_PIN:
        raise ValueError('Expected original English TU1.5 executable')
    image = Xbe(NEW_DEFAULT)
    code = _Code(image, '.text')
    definition = int.from_bytes(image.read(0x480c00 + 566 * 4, 4), 'little')
    if int.from_bytes(image.read(definition + 4, 4), 'little') != 0x2b3710:
        raise ValueError('Original game_won evaluator registration differs')
    calls = {0x2b3710: 0x13b3c0, 0x13b3fe: 0x193257, 0x13b419: 0x12e490,
             0x1917cb: 0x1a5c80, 0x1917dd: 0x1a5ab0, 0x191864: 0x64fd0,
             0x191874: 0x191580, 0x193272: 0x192a41, 0x19329b: 0x192ac1,
             0x1a5bee: 0x32c0a0, 0x1a5c07: 0x32c0a0}
    for pc, target in calls.items():
        instruction = code.at(pc)
        if instruction.mnemonic != 'call' or instruction.operands[0].imm != target:
            raise ValueError('Original completion call correspondence differs')
    sites, instructions, addresses, routines = {}, {}, {}, []
    for entry in ENTRIES:
        body = _trace(code, entry, set(ENTRIES))
        if not body.code or body.tables:
            raise ValueError('Unexpected original completion control flow')
        sites[f'{entry:08x}'] = sorted(body.code)
        routines.append(dict(entry=entry, tables=[], byte_tables=[]))
        for pc in sorted(body.code):
            instruction = code.at(pc)
            if image.read(pc, instruction.size) != bytes(instruction.bytes):
                raise ValueError('Original completion instruction differs')
            instructions[f'{pc:08x}'] = pc
            addresses[pc] = pc
            addresses[pc + instruction.size] = pc + instruction.size
            for operand in instruction.operands:
                if operand.type == 2:
                    addresses[operand.imm & 0xffffffff] = operand.imm & 0xffffffff
                elif operand.type == 3:
                    addresses[operand.mem.disp & 0xffffffff] = operand.mem.disp & 0xffffffff
    correspondence = dict(image='English TU1.5', original_sha256=NEW_PIN,
        entries={f'{p:08x}': p for p in ENTRIES}, body_sites=sites,
        instruction_map=instructions, address_map={f'{p:08x}': q for p, q in addresses.items()},
        routines=routines)
    args.directory.mkdir(parents=True)
    manifest = args.directory / 'bodies.json'
    manifest.write_text(json.dumps(correspondence, indent=2, sort_keys=True) + '\n')
    generate(SimpleNamespace(output=args.output, manifest=args.directory / 'generation.json',
        correspondence=manifest, additional_entries=Path(__file__)), correspondence,
        runtime_header='halo2/source_campaign_completion_tu15.hpp',
        runtime_function='source_run_campaign_completion_tu15')
    source = args.output.read_text().replace('tools/generate_physics_live_support_tu15.py',
        'tools/generate_campaign_completion_tu15.py').replace('source_run_physics_live_support_tu15',
        'source_run_campaign_completion_tu15').replace('source_physics_live_support_tu15_owns_entry',
        'source_campaign_completion_tu15_owns_entry')
    args.output.write_text(source)
    path = args.directory / 'generation.json'
    report = json.loads(path.read_text())
    report.update(generated_cpp_sha256=sha(args.output), module_generator_sha256=sha(Path(__file__)),
        original_call_sites={f'{p:08x}': f'{q:08x}' for p, q in calls.items()}, scope=__doc__)
    path.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    print(len(ENTRIES), 'whole original completion bodies;', len(instructions), 'instructions')


if __name__ == '__main__':
    main()
