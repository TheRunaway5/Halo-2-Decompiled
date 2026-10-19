#!/usr/bin/env python3
"""Recover the closed original54-instruction sound-source binding composition.

Generate into owned test scratch. Integration and behavioral validation are
separate; recovered instructions alone do not establish audio or Xbox parity.
"""
import argparse
import hashlib
import json
from pathlib import Path
from generate_physics_live_support_tu15 import generate
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

ENTRIES = {0x12b1c0:16,0x129470:38}
CALLS = {0x128de8:0x12b1c0,0x12b1d8:0x129470}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--manifest', type=Path, required=True)
    args = parser.parse_args()
    assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest() == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    code = _Code(image, '.text')
    registrations = []
    for pc, target in CALLS.items():
        instruction = code.at(pc)
        assert instruction.mnemonic == 'call' and instruction.operands[0].imm == target
        registrations.append(dict(site=pc, target=target, instruction=instruction.bytes.hex()))
    sites, instructions, addresses, routines = {}, {}, {}, []
    for entry, count in ENTRIES.items():
        body = _trace(code, entry, set(ENTRIES))
        assert len(body.code) == count and not body.tail_jumps and not body.tables
        sites[f'{entry:08x}'] = sorted(body.code)
        routines.append(dict(entry=entry, original_tail_targets=[], tables=[], byte_tables=[]))
        for pc in sorted(body.code):
            instruction = code.at(pc)
            assert image.read(pc, instruction.size) == bytes(instruction.bytes)
            if instruction.mnemonic == 'call':
                assert instruction.operands[0].type == 2 and instruction.operands[0].imm in ENTRIES
            instructions[f'{pc:08x}'] = pc
            addresses[pc] = pc
            addresses[pc + instruction.size] = pc + instruction.size
            for operand in instruction.operands:
                if operand.type == 2:
                    addresses[operand.imm & 0xffffffff] = operand.imm & 0xffffffff
                if operand.type == 3:
                    addresses[operand.mem.disp & 0xffffffff] = operand.mem.disp & 0xffffffff
    paired = dict(image='English TU1.5', original_sha256=NEW_PIN,
        entries={f'{entry:08x}': entry for entry in ENTRIES}, body_sites=sites,
        instruction_map=instructions, address_map={f'{pc:08x}': value for pc, value in addresses.items()}, routines=routines)
    args.manifest.parent.mkdir(parents=True, exist_ok=True)
    args.correspondence = args.manifest.with_name('binding-bodies.json')
    args.additional_entries = args.manifest.with_name('binding-callers.json')
    args.correspondence.write_text(json.dumps(paired, indent=2) + '\n')
    args.additional_entries.write_text(json.dumps(dict(original_sha256=NEW_PIN, calls=registrations), indent=2) + '\n')
    generate(args, paired, runtime_header='halo2/source_sound_binding_tu15.hpp')
    text = args.output.read_text().replace('source_run_physics_live_support_tu15',
        'source_run_sound_binding_tu15').replace('source_physics_live_support_tu15_owns_entry',
        'source_sound_binding_tu15_owns_entry')
    args.output.write_text(text)
    manifest = json.loads(args.manifest.read_text())
    manifest.update(configuration_tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        generated_cpp_sha256=hashlib.sha256(args.output.read_bytes()).hexdigest(),
        closed_source_composition=True, complete_original_bodies=2, complete_original_instructions=54,
        production_integration_or_behavior_verified=False)
    args.manifest.write_text(json.dumps(manifest, indent=2, sort_keys=True) + '\n')


if __name__ == '__main__':
    main()
