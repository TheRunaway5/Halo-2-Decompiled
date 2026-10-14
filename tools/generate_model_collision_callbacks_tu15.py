#!/usr/bin/env python3
"""Build two original collision callbacks for the isolated model-light runtime.

The current checkout already includes these bodies in its combat module. This
small composition lets the older frozen graphics checkpoint reach real grenade
construction without importing its incompatible multiplayer/heap interfaces.
External geometry and physics owners remain required; no output is supplied.
"""
import argparse
import hashlib
import json
from pathlib import Path

from generate_physics_live_support_tu15 import generate
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

ENTRIES = (0x2F6EB0, 0x2F7130)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--manifest', type=Path, required=True)
    args = parser.parse_args()
    assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest() == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    code = _Code(image, '.text')
    # The first callback was reached by the live constructor through this
    # literal agent-table call. Its target is recorded in the failing run.
    assert bytes(code.at(0x2F5F19).bytes) == bytes.fromhex('ff94818c210000')
    sites, instructions, addresses, routines = {}, {}, {}, []
    for entry in ENTRIES:
        body = _trace(code, entry, {*ENTRIES, 0x2E44D0})
        assert body.code and not body.tables
        sites[f'{entry:08x}'] = sorted(body.code)
        routines.append(dict(entry=entry, original_tail_targets=sorted(body.tail_jumps), tables=[], byte_tables=[]))
        for pc in sorted(body.code):
            instruction = code.at(pc)
            assert image.read(pc, instruction.size) == bytes(instruction.bytes)
            instructions[f'{pc:08x}'] = pc
            addresses[pc] = pc
            addresses[pc + instruction.size] = pc + instruction.size
            for operand in instruction.operands:
                if operand.type == 2:
                    addresses[operand.imm & 0xFFFFFFFF] = operand.imm & 0xFFFFFFFF
                elif operand.type == 3:
                    addresses[operand.mem.disp & 0xFFFFFFFF] = operand.mem.disp & 0xFFFFFFFF
    paired = dict(image='English TU1.5', original_sha256=NEW_PIN,
                  entries={f'{entry:08x}': entry for entry in ENTRIES}, body_sites=sites,
                  instruction_map=instructions, address_map={f'{pc:08x}': value for pc, value in addresses.items()},
                  routines=routines, complete_bodies=len(ENTRIES))
    args.manifest.parent.mkdir(parents=True, exist_ok=True)
    args.correspondence = args.manifest.with_name('model-collision-bodies.json')
    args.additional_entries = args.manifest.with_name('model-collision-dispatch.json')
    args.correspondence.write_text(json.dumps(paired, indent=2) + '\n')
    args.additional_entries.write_text(json.dumps(dict(original_sha256=NEW_PIN,
        scope='Isolated graphics checkpoint; current root combat module already owns these entries',
        reached_call='002f5f19', reached_target='002f6eb0'), indent=2) + '\n')
    generate(args, paired, runtime_header='model_collision_callbacks_tu15.hpp',
             runtime_function='source_run_model_collision_callbacks_tu15')
    text = args.output.read_text().replace('source_physics_live_support_tu15_owns_entry',
                                         'source_model_collision_callbacks_tu15_owns_entry')
    text = text.replace('source_run_physics_live_support_tu15', 'source_run_model_collision_callbacks_tu15')
    # These two math/agent callbacks contain no projectile-effect observation
    # sites. The shared generator's unconditional optional include would pull
    # an unrelated, newer application interface into the frozen composition.
    assert 'SourceProjectileUpdateScope' not in text
    assert 'source_begin_projectile_contact_effect_tu15' not in text
    text = text.replace('#include "halo2/source_projectile_effect_observer.hpp"\n', '')
    args.output.write_text(text)
    args.output.with_name('model_collision_callbacks_tu15.hpp').write_text('''#pragma once
#include "halo2/physics_world_runtime.hpp"
namespace halo2 {
bool source_model_collision_callbacks_tu15_owns_entry(std::uint32_t);
void source_run_model_collision_callbacks_tu15(SourcePhysicsContactMemory&,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
}
''')
    report = json.loads(args.manifest.read_text())
    report['generated_cpp_sha256'] = hashlib.sha256(args.output.read_bytes()).hexdigest()
    report['composition_generator_sha256'] = hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
    args.manifest.write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    main()
