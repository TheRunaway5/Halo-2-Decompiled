#!/usr/bin/env python3
"""Lower the complete original object-material query and its private type test."""
import argparse
import json
import re
from pathlib import Path
from types import SimpleNamespace
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import generate, sha


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--reference', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--directory', type=Path, required=True)
    args = parser.parse_args()
    reference = json.loads((args.reference / 'report.json').read_text())
    hardware = json.loads((args.reference / 'hardware/report.json').read_text())
    assert reference['completed'] and reference['entry'] == 0x18b4c0
    assert reference['original_xbe_sha256'] == NEW_PIN == sha(NEW_DEFAULT)
    assert not reference['instruction_patches'] and not reference['supplied_game_services']
    assert reference['source_context']['actual_nested_caller'] == 0xd1878
    assert hardware['emulator_retained_state_matches_hardware']
    assert hardware['original_report_sha256'] == sha(args.reference / 'report.json')
    image = Xbe(NEW_DEFAULT)
    code = _Code(image, '.text')
    call = code.at(0xd1878)
    assert call.mnemonic == 'call' and call.operands[0].imm == 0x18b4c0
    entries = {0x18b4c0, 0xbcdf0}
    sites, instructions, addresses, routines = {}, {}, {}, []
    for entry in sorted(entries):
        body = _trace(code, entry, entries)
        assert body.code and not body.tables
        assert set(body.calls) <= entries
        sites[f'{entry:08x}'] = sorted(body.code)
        routines.append(dict(entry=entry, tables=[], byte_tables=[]))
        for pc in sorted(body.code):
            instruction = code.at(pc)
            assert image.read(pc, instruction.size) == instruction.bytes
            instructions[f'{pc:08x}'] = pc
            addresses[pc] = pc
            addresses[pc + instruction.size] = pc + instruction.size
            for operand in instruction.operands:
                if operand.type == 2:
                    addresses[operand.imm & 0xffffffff] = operand.imm & 0xffffffff
                elif operand.type == 3:
                    addresses[operand.mem.disp & 0xffffffff] = operand.mem.disp & 0xffffffff
    paired = dict(image='English TU1.5', original_sha256=NEW_PIN,
                  entries={f'{entry:08x}': entry for entry in entries}, body_sites=sites,
                  instruction_map=instructions,
                  address_map={f'{pc:08x}': value for pc, value in addresses.items()},
                  routines=routines)
    args.directory.mkdir(parents=True, exist_ok=False)
    correspondence = args.directory / 'bodies.json'
    correspondence.write_text(json.dumps(paired, indent=2, sort_keys=True) + '\n')
    generate(SimpleNamespace(output=args.output, manifest=args.directory / 'generation.json',
                             correspondence=correspondence, additional_entries=Path(__file__)),
             paired)
    source = args.output.read_text()
    source = source.replace('tools/generate_physics_live_support_tu15.py',
                            'tools/generate_object_material_tu15.py')
    source = source.replace('halo2/physics_live_support_tu15.hpp',
                            'halo2/source_object_material_tu15.hpp')
    source = source.replace('source_run_physics_live_support_tu15',
                            'source_run_object_material_tu15')
    source = source.replace('source_physics_live_support_tu15_owns_entry',
                            'source_object_material_tu15_owns_entry')
    source, count = re.subn(r'bool source_object_material_tu15_owns_entry\(std::uint32_t p\) \{[^\n]+',
                            'bool source_object_material_tu15_owns_entry(std::uint32_t p) {return p==0x18b4c0u;}',
                            source)
    assert count == 1
    args.output.write_text(source)
    report_path = args.directory / 'generation.json'
    report = json.loads(report_path.read_text())
    report.update(generated_cpp_sha256=sha(args.output), module_generator_sha256=sha(Path(__file__)),
                  original_reference_report_sha256=sha(args.reference / 'report.json'),
                  hardware_report_sha256=sha(args.reference / 'hardware/report.json'),
                  exported_entries=['0018b4c0'], private_helpers=['000bcdf0'],
                  scope=__doc__, full_weapon_parity=False)
    report_path.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    print('Complete object-material query and private helper:', len(instructions), 'instructions')


if __name__ == '__main__':
    main()
