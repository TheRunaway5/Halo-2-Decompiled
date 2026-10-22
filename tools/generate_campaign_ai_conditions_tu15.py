#!/usr/bin/env python3
"""Lower whole original conditions reached by actual29ACAF in the AI frame.

Whole29AFD0 and three previously absent predicates retain every branch. Existing
AI-frame, iterator and HS owners are called with their real ABI and same bank.
Missing nested owners still fail. Original switch bytes are immutable inputs;
no condition, actor, path, script outcome or gameplay return is supplied.
"""
from source_layout import source_file
import argparse
import json
import os
from pathlib import Path
import re
from types import SimpleNamespace
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import generate, sha

ROOT=Path(__file__).resolve().parents[1]
ENTRIES=(0x29afd0,0x20b500,0x20b570,0x2802f0,0x203b60)
FRAME=(0x1494f0,0x211750,0x29ac70,0x29b380,0x27caa0,0x27cb30,0xc1e90,0x213d30)
CAMPAIGN=(0x20d070,0x20d0d0)
MODULE='source_campaign_ai_conditions_tu15_generated.cpp'


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=Path,required=True)
    ap.add_argument('--directory',type=Path,required=True)
    ap.add_argument('--tables',type=Path,default=ROOT/'src/campaign/ai/native_tu15_campaign_ai_conditions_tables.inc')
    args=ap.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:ap.error('Run through tools/run_test.py')
    for file in (ROOT/'src').rglob('*_generated.cpp'):
        if file.name!=MODULE:
            owned={int(p,16) for p in re.findall(r'^static void run_([0-9a-f]+)\(WorldState &s\) \{',file.read_text(),re.M)}
            assert not set(ENTRIES)&owned,f'Existing condition owner: {file}'
    for name,entries in (('source_ai_frame_tu15_generated.cpp',FRAME),('source_campaign_runtime_tu15_generated.cpp',CAMPAIGN)):
        source=(source_file(ROOT, name)).read_text()
        for entry in entries:
            assert f'static void run_{entry:x}(WorldState &s) {{' in source
            assert f'case 0x{entry:x}u:run_{entry:x}(s);break;' in source
    assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
    assert image.read(0x29acaf,5).hex()=='e81c030000'
    assert image.read(0x20af85,5).hex()=='e8d68bffff'
    sites,addresses,routines={},{},[]
    for entry in ENTRIES:
        body=_trace(code,entry,set(ENTRIES));assert body.code
        sites[f'{entry:08x}']=sorted(body.code)
        routines.append(dict(entry=entry,tables=[dict(disc=p,count=n) for p,w,n in body.tables if w==4],
            byte_tables=[dict(address=p,width=w,count=n,raw_hex=image.read(p,w*n).hex()) for p,w,n in body.tables if w==1]))
        for pc in sorted(body.code):
            instruction=code.at(pc);assert image.read(pc,instruction.size)==bytes(instruction.bytes)
            addresses[pc]=pc;addresses[pc+instruction.size]=pc+instruction.size
            for operand in instruction.operands:
                if operand.type==2:addresses[operand.imm&0xffffffff]=operand.imm&0xffffffff
                elif operand.type==3:addresses[operand.mem.disp&0xffffffff]=operand.mem.disp&0xffffffff
    paired=dict(image='English TU1.5',original_sha256=NEW_PIN,entries={f'{p:08x}':p for p in ENTRIES},
        body_sites=sites,instruction_map={f'{p:08x}':p for pcs in sites.values() for p in pcs},
        address_map={f'{p:08x}':p for p in addresses},routines=routines)
    args.directory.mkdir(parents=True,exist_ok=True)
    correspondence=args.directory/'bodies.json';correspondence.write_text(json.dumps(paired,indent=2,sort_keys=True)+'\n')
    manifest=args.directory/'generation.json'
    generate(SimpleNamespace(output=args.output,manifest=manifest,correspondence=correspondence,
        additional_entries=Path(__file__)),paired,runtime_header='halo2/source_campaign_ai_conditions_tu15.hpp')
    source=args.output.read_text().replace('tools/generate_physics_live_support_tu15.py',
        'tools/generate_campaign_ai_conditions_tu15.py').replace('source_run_physics_live_support_tu15',
        'source_run_campaign_ai_conditions_body_tu15').replace('source_physics_live_support_tu15_owns_entry',
        'source_campaign_ai_conditions_tu15_owns_entry')
    for header in ('physics_live_support_tu15','source_projectile_effect_observer','source_object_attachment_tu15'):
        source=source.replace(f'#include "halo2/{header}.hpp"\n','')
    source=source.replace('#include "physics/runtime/physics_world_runtime_internal.hpp"',
        '#include "physics/runtime/physics_world_runtime_internal.hpp"\n#include "halo2/source_ai_frame_tu15.hpp"\n#include "halo2/source_campaign_runtime_tu15.hpp"')
    source+='''
namespace halo2 {
void source_run_campaign_ai_conditions_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &abi,SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsWorldRuntimeServices &services,const SourcePhysicsContactRecordObserver &observe) {
    auto nested=services;
    nested.required_owner=[&](auto &bank,const auto &call,auto &state,const auto &stores) {
        const auto control=state.floating_point.value_or(fp);
        if(source_campaign_ai_conditions_tu15_owns_entry(call.target)) {
            source_run_campaign_ai_conditions_body_tu15(bank,call.target,state,control,nested,stores);return;
        }
        if('''+' || '.join(f'call.target==0x{p:x}u' for p in FRAME)+''') {
            source_run_ai_frame_tu15(bank,call.target,state,control,nested,stores);return;
        }
        if('''+' || '.join(f'call.target==0x{p:x}u' for p in CAMPAIGN)+''') {
            source_run_campaign_runtime_tu15(bank,call.target,state,control,nested,stores);return;
        }
        if(services.required_owner) {services.required_owner(bank,call,state,stores);return;}
        throw std::runtime_error("Required original campaign AI condition dependency unavailable");
    };
    source_run_campaign_ai_conditions_body_tu15(memory,entry,abi,fp,nested,observe);
}
}
'''
    args.output.write_text(source)
    #29AC70 already has its native body; publish its actual tables alongside
    #the new callee's tables so recursion retains original immutable owners.
    tables={p:(w,n) for entry in (0x29afd0,0x29ac70) for p,w,n in _trace(code,entry,{entry}).tables}
    words={p+i:int.from_bytes(image.read(p+i,4),'little') for p,(w,n) in tables.items()
           for i in range(0,((w*n+3)//4)*4,4)}
    args.tables.write_text('// Authenticated complete29AFD0/29AC70 switches, including original byte-table padding.\n'+
        ''.join(f'share_immutable_word(memory,0x{p:x}u,0x{value:x}u,"native TU1.5 authored AI condition dispatch data");\n'
                for p,value in sorted(words.items())))
    report=json.loads(manifest.read_text())
    report.update(generated_cpp_sha256=sha(args.output),generator_sha256=sha(Path(__file__)),scope=__doc__,
        original_instructions=sum(map(len,sites.values())),tables_sha256=sha(args.tables),
        reused_frame_entries=list(FRAME),reused_campaign_entries=list(CAMPAIGN),
        reused_modules_sha256={name:sha(source_file(ROOT, name)) for name in (
            'source_ai_frame_tu15_generated.cpp','source_campaign_runtime_tu15_generated.cpp')})
    manifest.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print('Complete original AI conditions:',report['original_instructions'],'instructions')


if __name__=='__main__':main()
