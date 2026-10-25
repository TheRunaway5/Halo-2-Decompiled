#!/usr/bin/env python3
"""Lower the complete original BV-agent owners reached by the Midship witness.

Generation is offline. Production receives compiled arithmetic and branches,
with no executable blob, opcode decoder or supplied collision result.
"""
import argparse
import csv
import json
import os
from pathlib import Path
from types import SimpleNamespace

from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import generate,sha

ROOT=Path(__file__).resolve().parents[1]
ENTRIES=(0x1cdfa0,0x2f1790,0x2f18d0,0x2fe0f0,0x30b960)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--directory',type=Path,required=True)
    parser.add_argument('--fixture',type=Path,required=True)
    parser.add_argument('--fixture-sha256',required=True)
    parser.add_argument('--report-output',type=Path,required=True)
    args=parser.parse_args()
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    if not args.directory.resolve().is_relative_to(owner):parser.error('Intermediate recovery must stay in owned scratch')
    assert sha(NEW_DEFAULT)==NEW_PIN and sha(args.fixture)==args.fixture_sha256
    witness=json.loads(args.fixture.read_text());case=witness['cases'][0]
    assert witness['image']=='1.5' and case['entry']==0x2f18d0 and case['complete']
    assert len(case['instructions'])==453 and len(case['writes'])==83
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');edges=[]
    for pc,target in zip(case['instructions'],case['instructions'][1:]):
        instruction=code.at(pc)
        if instruction.mnemonic=='call':
            edges.append(dict(site=pc,target=target,raw_hex=instruction.bytes.hex(),operands=instruction.op_str))
    assert set(ENTRIES)=={0x2f18d0}|{row['target'] for row in edges}-{0x31ede0,0x312620}
    inventory=ROOT/'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv'
    starts={int(row['address'],16) for row in csv.DictReader(inventory.open(),delimiter='\t')}
    args.directory.mkdir(parents=True,exist_ok=False)
    sites={};instructions={};addresses={};routines=[]
    for entry in ENTRIES:
        body=_trace(code,entry,starts|set(ENTRIES));assert body.code
        assert all(width in (1,4) for _,width,_ in body.tables)
        sites[f'{entry:08x}']=sorted(body.code)
        (args.directory/f'{entry:08x}.asm').write_text(''.join(
            f'{pc:08x} {code.at(pc).bytes.hex()} {code.at(pc).mnemonic} {code.at(pc).op_str}\n' for pc in sorted(body.code)))
        routines.append(dict(entry=entry,tables=[dict(disc=p,count=n) for p,width,n in body.tables if width==4],
            byte_tables=[dict(address=p,width=width,count=n,raw_hex=image.read(p,width*n).hex()) for p,width,n in body.tables if width==1]))
        for pc in sorted(body.code):
            instruction=code.at(pc);assert image.read(pc,instruction.size)==bytes(instruction.bytes)
            instructions[f'{pc:08x}']=pc;addresses[pc]=pc;addresses[pc+instruction.size]=pc+instruction.size
            for operand in instruction.operands:
                if operand.type==2:addresses[operand.imm&0xffffffff]=operand.imm&0xffffffff
                elif operand.type==3:addresses[operand.mem.disp&0xffffffff]=operand.mem.disp&0xffffffff
    paired=dict(image='English TU1.5',original_sha256=NEW_PIN,entries={f'{pc:08x}':pc for pc in ENTRIES},
        body_sites=sites,instruction_map=instructions,address_map={f'{pc:08x}':value for pc,value in addresses.items()},routines=routines)
    correspondence=args.directory/'bodies.json';correspondence.write_text(json.dumps(paired,indent=2,sort_keys=True)+'\n')
    manifest=args.directory/'generation.json'
    generate(SimpleNamespace(output=args.output,manifest=manifest,correspondence=correspondence,additional_entries=Path(__file__)),paired)
    source=args.output.read_text().replace('tools/generate_physics_live_support_tu15.py','tools/generate_vehicle_bv_agent_tu15.py')
    source=source.replace('halo2/physics_live_support_tu15.hpp','halo2/source_vehicle_bv_agent_tu15.hpp')
    source=source.replace('source_run_physics_live_support_tu15','source_run_vehicle_bv_agent_tu15')
    source=source.replace('source_physics_live_support_tu15_owns_entry','source_vehicle_bv_agent_tu15_owns_entry')
    args.output.write_text(source)
    report=json.loads(manifest.read_text());report.update(generated_cpp_sha256=sha(args.output),
        generator_sha256=sha(Path(__file__)),original_witness_sha256=sha(args.fixture),
        lowerer_sha256=sha(ROOT/'tools/generate_physics_live_support_tu15.py'),
        original_reached_call_edges=edges,existing_native_callbacks=[0x31ede0,0x312620],
        native_expected_outputs=False,production_emulator=False,full_vehicle_parity=False)
    args.report_output.parent.mkdir(parents=True,exist_ok=True)
    args.report_output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(native_entries=len(ENTRIES),instructions=len(instructions),report=str(args.report_output))),flush=True)


if __name__=='__main__':main()
