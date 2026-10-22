#!/usr/bin/env python3
"""Lower complete TU1.5 settings owners reached by authored Cairo Grunt birth.

Actual1E8BBF calls20B980; its whole1ECC80 dependency follows character parents.
All74 instructions and the original switch are retained. These bodies have no
other native owner; no gameplay return, tag value or actor record is supplied.
"""
import argparse
import json
import os
from pathlib import Path
import re
from types import SimpleNamespace
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import generate, sha

ROOT = Path(__file__).resolve().parents[1]
ENTRIES = (0x20b980, 0x1ecc80)
MODULE = 'source_campaign_ai_placement_settings_tu15_generated.cpp'


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output', type=Path, required=True)
    ap.add_argument('--directory', type=Path, required=True)
    ap.add_argument('--tables', type=Path,
        default=ROOT/'src/campaign/ai/native_tu15_campaign_ai_placement_settings_tables.inc')
    args = ap.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:
        ap.error('Run through tools/run_test.py')
    for file in (ROOT/'src').rglob('*_generated.cpp'):
        if file.name != MODULE:
            owned = {int(p, 16) for p in re.findall(r'^static void run_([0-9a-f]+)\(WorldState &s\) \{', file.read_text(), re.M)}
            assert not set(ENTRIES)&owned, f'Existing placement-settings owner: {file}'
    assert sha(NEW_DEFAULT) == NEW_PIN
    image, sites, addresses, routines = Xbe(NEW_DEFAULT), {}, {}, []
    code = _Code(image, '.text')
    assert image.read(0x1e8bbf, 5).hex() == 'e8bc2d0200'
    for entry in ENTRIES:
        body = _trace(code, entry, set(ENTRIES))
        assert body.code and all(width == 4 for _, width, _ in body.tables)
        sites[f'{entry:08x}'] = sorted(body.code)
        routines.append(dict(entry=entry,tables=[dict(disc=p,count=n) for p,_,n in body.tables],byte_tables=[]))
        for pc in sorted(body.code):
            instruction = code.at(pc)
            assert image.read(pc, instruction.size) == bytes(instruction.bytes)
            addresses[pc] = pc
            addresses[pc+instruction.size] = pc+instruction.size
            for operand in instruction.operands:
                if operand.type == 2:
                    addresses[operand.imm&0xffffffff] = operand.imm&0xffffffff
                elif operand.type == 3:
                    addresses[operand.mem.disp&0xffffffff] = operand.mem.disp&0xffffffff
    paired = dict(image='English TU1.5',original_sha256=NEW_PIN,
        entries={f'{p:08x}':p for p in ENTRIES},body_sites=sites,
        instruction_map={f'{p:08x}':p for pcs in sites.values() for p in pcs},
        address_map={f'{p:08x}':p for p in addresses},routines=routines)
    args.directory.mkdir(parents=True,exist_ok=True)
    correspondence=args.directory/'bodies.json'
    correspondence.write_text(json.dumps(paired,indent=2,sort_keys=True)+'\n')
    manifest=args.directory/'generation.json'
    generate(SimpleNamespace(output=args.output,manifest=manifest,correspondence=correspondence,
        additional_entries=Path(__file__)),paired,
        runtime_header='halo2/source_campaign_ai_placement_settings_tu15.hpp')
    source=args.output.read_text().replace('tools/generate_physics_live_support_tu15.py',
        'tools/generate_campaign_ai_placement_settings_tu15.py').replace(
        'source_run_physics_live_support_tu15','source_run_campaign_ai_placement_settings_tu15').replace(
        'source_physics_live_support_tu15_owns_entry','source_campaign_ai_placement_settings_tu15_owns_entry')
    # These generic lowering headers serve other modules' call hooks. Neither
    # complete body uses them; keep this leaf's dependency/ABI surface narrow.
    for header in ('physics_live_support_tu15','source_projectile_effect_observer',
                   'source_object_attachment_tu15'):
        source=source.replace(f'#include "halo2/{header}.hpp"\n','')
    args.output.write_text(source)
    words=tuple(int.from_bytes(image.read(0x20ba30+i*4,4),'little') for i in range(4))
    args.tables.write_text('// Authenticated complete20B980 settings switch; native code chooses every index.\n'+
        ''.join(f'share_immutable_word(memory,0x{0x20ba30+i*4:x}u,0x{value:x}u,"native TU1.5 authored campaign settings dispatch target");\n'
                for i,value in enumerate(words)))
    report=json.loads(manifest.read_text())
    report.update(generated_cpp_sha256=sha(args.output),module_generator_sha256=sha(Path(__file__)),
        scope=__doc__,actual_caller=0x1e8bbf,original_instructions=sum(map(len,sites.values())),
        tables_sha256=sha(args.tables),original_table_hex=image.read(0x20ba30,16).hex())
    manifest.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print('Complete original placement settings:',report['original_instructions'],'instructions')


if __name__=='__main__':
    main()
