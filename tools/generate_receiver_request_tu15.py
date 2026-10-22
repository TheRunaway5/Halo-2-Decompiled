#!/usr/bin/env python3
"""Lower the complete TU1.5 receiver request and its queued callbacks.

The existing original shadow-frame owner retains queue allocation/dispatch.
This module never supplies a queue record, callback result or solved receiver.
"""
import argparse
import json
from pathlib import Path
from types import SimpleNamespace
from generate_physics_live_support_tu15 import complete_body, generate, sha
from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

ENTRIES = (0x47860, 0x48e30, 0x8fdb0)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--directory', type=Path, required=True)
    args = parser.parse_args()
    assert sha(NEW_DEFAULT) == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    code = _Code(image, '.text')
    assert code.at(0x118793).bytes.hex() == 'e8c8f0f2ff'
    assert code.at(0x11878e).bytes.hex() == 'bfb0fd0800'
    assert image.read(0x8fdb0, 3) == bytes.fromhex('c20800')
    sites, instructions, addresses, routines = {}, {}, {}, []
    for entry in ENTRIES:
        body = complete_body(code, entry)
        assert body.code and not body.tables
        sites[f'{entry:08x}'] = sorted(body.code)
        routines.append(dict(entry=entry, tables=[]))
        for pc in sorted(body.code):
            instruction = code.at(pc)
            assert image.read(pc, instruction.size) == bytes(instruction.bytes)
            instructions[f'{pc:08x}'] = pc
            addresses[pc] = pc
            addresses[pc + instruction.size] = pc + instruction.size
            for operand in instruction.operands:
                if operand.type == 2:
                    addresses[operand.imm & 0xffffffff] = operand.imm & 0xffffffff
                elif operand.type == 3:
                    addresses[operand.mem.disp & 0xffffffff] = operand.mem.disp & 0xffffffff
    paired = dict(image='English TU1.5', original_sha256=NEW_PIN,
        entries={f'{p:08x}': p for p in ENTRIES}, body_sites=sites,
        instruction_map=instructions,
        address_map={f'{p:08x}': q for p, q in addresses.items()}, routines=routines)
    args.directory.mkdir(parents=True, exist_ok=True)
    correspondence = args.directory / 'bodies.json'
    correspondence.write_text(json.dumps(paired, indent=2, sort_keys=True) + '\n')
    manifest = args.directory / 'generation.json'
    generate(SimpleNamespace(output=args.output, manifest=manifest,
        correspondence=correspondence, additional_entries=Path(__file__)), paired)
    source = args.output.read_text().replace('tools/generate_physics_live_support_tu15.py',
        'tools/generate_receiver_request_tu15.py').replace(
        'halo2/physics_live_support_tu15.hpp', 'halo2/source_receiver_request_tu15.hpp').replace(
        'source_run_physics_live_support_tu15', 'source_run_receiver_request_tu15').replace(
        'source_physics_live_support_tu15_owns_entry', 'source_receiver_request_tu15_owns_entry')
    if 'SourceProjectileUpdateScope' not in source and 'source_begin_projectile_contact_effect_tu15' not in source:
        source = source.replace('#include "halo2/source_projectile_effect_observer.hpp"\n', '')
    args.output.write_text(source)
    report = json.loads(manifest.read_text())
    report.update(generated_cpp_sha256=sha(args.output), original_external_owner='1E2B0: complete source shadow-frame queue allocation',
        scope='Complete original47860 request and48E30/8FDB0 queued callbacks, preserving all source arithmetic, writes, stack cleanup and unavailable-service failures.')
    manifest.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')

if __name__ == '__main__':
    main()
