#!/usr/bin/env python3
"""Lower the complete original127B20 scalar query and228220 class getter."""
import argparse
import hashlib
import json
from pathlib import Path
from generate_physics_live_support_tu15 import generate
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--manifest', type=Path, required=True)
    args = parser.parse_args()
    assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest() == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    code = _Code(image, '.text')
    calls = [(0x129846, 0x127b20, 'e8d5e2ffff'), (0x127b4e, 0x228220, 'e8cd061000')]
    for site, target, raw in calls:
        instruction = code.at(site)
        assert instruction.mnemonic == 'call' and instruction.operands[0].imm == target and instruction.bytes.hex() == raw
    entries = (0x127b20, 0x228220)
    sites = {}
    instructions = {}
    addresses = {entry: entry for entry in entries}
    for entry, count in zip(entries, (23, 17)):
        body = _trace(code, entry, set(entries))
        assert len(body.code) == count and not body.tail_jumps and not body.tables
        sites[f'{entry:08x}'] = sorted(body.code)
        for pc in sorted(body.code):
            instruction = code.at(pc)
            assert image.read(pc, instruction.size) == bytes(instruction.bytes)
            instructions[f'{pc:08x}'] = pc
            addresses[pc] = pc
            addresses[pc + instruction.size] = pc + instruction.size
            for operand in instruction.operands:
                if operand.type == 2:
                    addresses[operand.imm & 0xffffffff] = operand.imm & 0xffffffff
                if operand.type == 3:
                    addresses[operand.mem.disp & 0xffffffff] = operand.mem.disp & 0xffffffff
    paired = dict(image='English TU1.5', original_sha256=NEW_PIN,
        entries={f'{entry:08x}': entry for entry in entries}, body_sites=sites,
        instruction_map=instructions, address_map={f'{pc:08x}': value for pc, value in addresses.items()},
        routines=[dict(entry=entry, original_tail_targets=[], tables=[], byte_tables=[]) for entry in entries])
    args.manifest.parent.mkdir(parents=True, exist_ok=True)
    args.correspondence = args.manifest.with_name('sound-scalar-bodies.json')
    args.correspondence.write_text(json.dumps(paired, indent=2) + '\n')
    args.additional_entries = args.manifest.with_name('sound-scalar-callers.json')
    args.additional_entries.write_text(json.dumps(dict(original_sha256=NEW_PIN,
        calls=[dict(site=site, target=target, instruction=raw) for site, target, raw in calls]), indent=2) + '\n')
    generate(args, paired, runtime_header='halo2/source_sound_scalar_tu15.hpp')
    generated = args.output.read_text().replace('source_run_physics_live_support_tu15',
        'source_run_sound_scalar_tu15').replace('source_physics_live_support_tu15_owns_entry',
        'source_sound_scalar_tu15_owns_entry')
    args.output.write_text(generated)
    report = json.loads(args.manifest.read_text())
    report.update(scalar_generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        generated_cpp_sha256=hashlib.sha256(args.output.read_bytes()).hexdigest(), renamed_exports_only=True)
    args.manifest.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')


if __name__ == '__main__':
    main()
