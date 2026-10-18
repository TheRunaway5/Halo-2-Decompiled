#!/usr/bin/env python3
"""Lower complete original row13 level-handle new-map bodies.

An authenticated actual callback selects complete routines and full CFGs.
No pool header, source result, readiness byte or game service is supplied.
The separate registered constructor remains in its existing owner.
"""
import argparse
import json
import re
import struct
from pathlib import Path
from types import SimpleNamespace
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import generate, sha


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--reference', type=Path, required=True)
    parser.add_argument('--entry', type=lambda value: int(value, 0), choices=(0x1a59d0,), default=0x1a59d0)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--directory', type=Path, required=True)
    args = parser.parse_args()
    reference = args.reference.resolve()
    report = json.loads((reference / 'report.json').read_text())
    hardware = json.loads((reference / 'hardware/report.json').read_text())
    assert report['completed'] and report['entry'] == args.entry
    assert report['source_context']['actual_registered_new_map_callback']
    assert report['original_xbe_sha256'] == sha(NEW_DEFAULT) == NEW_PIN
    assert not report['instruction_patches'] and not report['supplied_game_services']
    assert hardware['original_report_sha256'] == sha(reference / 'report.json')
    assert not report.get('native_platform_services')
    assert hardware['emulator_retained_state_matches_hardware'] and hardware['entire_exit_equal_except_platform_IF']
    observed = None
    image = Xbe(NEW_DEFAULT)
    code = _Code(image, '.text')
    assert sha(reference / 'original-trace.bin') == report['files_sha256']['original-trace.bin']
    trace = [pc for pc, in struct.iter_unpack('<I', (reference / 'original-trace.bin').read_bytes())]
    reached = set(trace)
    entries = {report['entry'], *(int(pc, 16) for pc in report['original_functions'])}
    indirect = set()
    for pc, target in zip(trace, trace[1:]):
        op = code.at(pc)
        assert op.bytes.hex() == report['executed_instruction_bytes'][f'{pc:08x}']
        if op.mnemonic == 'call':
            assert code.inside(target)
            entries.add(target)
        elif op.mnemonic == 'jmp' and op.operands[0].type != 2:
            indirect.add((pc, target))
    while True:
        additional = set()
        for entry in entries:
            body = _trace(code, entry, entries)
            additional.update(target for target in body.tail_jumps if target in reached)
            tables = {address for address, width, count in body.tables if width == 4}
            for pc, target in indirect:
                if pc in body.code and code.at(pc).operands[0].mem.disp not in tables:
                    additional.add(target)
        if additional <= entries:
            break
        entries.update(additional)
    sites, instructions, addresses, routines = {}, {}, {}, []
    for entry in sorted(entries):
        body = _trace(code, entry, entries)
        assert body.code and all(width in (1, 4) for _, width, _ in body.tables)
        sites[f'{entry:08x}'] = sorted(body.code)
        routines.append(dict(entry=entry,
            tables=[dict(disc=p, count=n) for p, w, n in body.tables if w == 4],
            byte_tables=[dict(address=p, width=w, count=n, raw_hex=image.read(p, w*n).hex())
                         for p, w, n in body.tables if w == 1]))
        for pc in sorted(body.code):
            op = code.at(pc)
            assert image.read(pc, op.size) == op.bytes
            instructions[f'{pc:08x}'] = pc
            addresses[pc] = pc
            addresses[pc + op.size] = pc + op.size
            for operand in op.operands:
                if operand.type == 2:
                    addresses[operand.imm & 0xffffffff] = operand.imm & 0xffffffff
                elif operand.type == 3:
                    addresses[operand.mem.disp & 0xffffffff] = operand.mem.disp & 0xffffffff
    paired = dict(image='English TU1.5', original_sha256=NEW_PIN,
        entries={f'{entry:08x}': entry for entry in entries}, body_sites=sites,
        instruction_map=instructions,
        address_map={f'{pc:08x}': value for pc, value in addresses.items()}, routines=routines)
    args.directory.mkdir(parents=True, exist_ok=False)
    correspondence = args.directory / 'bodies.json'
    correspondence.write_text(json.dumps(paired, indent=2, sort_keys=True) + '\n')
    generate(SimpleNamespace(output=args.output, manifest=args.directory / 'generation.json',
        correspondence=correspondence, additional_entries=Path(__file__)), paired)
    source = args.output.read_text().replace('halo2/physics_live_support_tu15.hpp', 'halo2/source_level_handles_new_map_tu15.hpp')
    source = source.replace('source_run_physics_live_support_tu15', 'source_run_level_handles_new_map_tu15')
    source = source.replace('source_physics_live_support_tu15_owns_entry', 'source_level_handles_new_map_tu15_owns_entry')
    source, count = re.subn(r'bool source_level_handles_new_map_tu15_owns_entry\(std::uint32_t p\) \{[^\n]+',
        f'bool source_level_handles_new_map_tu15_owns_entry(std::uint32_t p) {{return p==0x{args.entry:x}u;}}', source)
    assert count == 1
    source = '// Complete original row13 new-map owner; constructor remains separate.\n' + source
    args.output.write_text(source)
    manifest = args.directory / 'generation.json'
    generated = json.loads(manifest.read_text())
    generated.update(scope=__doc__, tool_sha256=sha(Path(__file__)),
        generated_cpp_sha256=sha(args.output), original_reference_report_sha256=sha(reference / 'report.json'),
        hardware_report_sha256=sha(reference / 'hardware/report.json'), observed_hardware_reference=observed,
        production_owner_changed=False, exported_original_entry=args.entry, full_weapon_parity=False)
    manifest.write_text(json.dumps(generated, indent=2, sort_keys=True) + '\n')
    print('Complete level-handle new-map owner:', len(entries), 'routines,', len(instructions), 'instructions')


if __name__ == '__main__':
    main()
