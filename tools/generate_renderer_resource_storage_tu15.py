#!/usr/bin/env python3
"""Lower authenticated TU1.5 renderer resource-storage reset1D640."""
import argparse
import hashlib
import json
from pathlib import Path

from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import generate


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--manifest', type=Path, required=True)
    args = parser.parse_args()
    assert sha(NEW_DEFAULT) == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    code = _Code(image, '.text')
    entries = [0x1D640]
    bodies = {entry: _trace(code, entry, set(entries)) for entry in entries}
    instructions, addresses = {}, {}
    assert not any(body.tables for body in bodies.values())
    for pc in sorted(set().union(*(body.code for body in bodies.values()))):
        instruction = code.at(pc)
        assert bytes(instruction.bytes) == image.read(pc, instruction.size)
        instructions[f'{pc:08x}'] = pc
        addresses[pc] = pc
        addresses[pc + instruction.size] = pc + instruction.size
        for operand in instruction.operands:
            if operand.type == 2:
                addresses[operand.imm & 0xFFFFFFFF] = operand.imm & 0xFFFFFFFF
            if operand.type == 3:
                addresses[operand.mem.disp & 0xFFFFFFFF] = operand.mem.disp & 0xFFFFFFFF
    paired = dict(image='English TU1.5', original_sha256=NEW_PIN,
        entries={f'{entry:08x}': entry for entry in entries},
        body_sites={f'{entry:08x}': sorted(body.code) for entry, body in bodies.items()},
        instruction_map=instructions,
        address_map={f'{pc:08x}': value for pc, value in addresses.items()},
        routines=[dict(entry=entry, tables=[], byte_tables=[]) for entry in entries])
    args.correspondence = args.manifest.with_name('renderer-resource-storage-bodies.json')
    args.correspondence.parent.mkdir(parents=True, exist_ok=True)
    args.correspondence.write_text(json.dumps(paired, sort_keys=True, indent=2) + '\n')
    args.additional_entries = Path(__file__)
    generate(args, paired, complete_listings=True)
    text = args.output.read_text().replace('halo2/physics_live_support_tu15.hpp',
        'halo2/source_renderer_resource_storage_tu15.hpp').replace('source_run_physics_live_support_tu15',
        'source_run_renderer_pool_tu15').replace('source_physics_live_support_tu15_owns_entry',
        'source_renderer_resource_storage_tu15_owns_entry')
    args.output.write_text(text)
    report = json.loads(args.manifest.read_text())
    report.update(renderer_resource_storage_generator_sha256=sha(__file__), generated_cpp_sha256=sha(args.output))
    args.manifest.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(dict(bodies=len(entries), instructions=len(instructions))))


if __name__ == '__main__':
    main()
