#!/usr/bin/env python3
"""Lower complete original actor behavior/scalar owners and missing callees.

The authored Marine reaches1B7240 then1B7000 through1B0128; Johnson reaches
1EA360 fromE57EE. Preserve their missing nested closure and every CFG branch.
The shared tick35 failure26CB18 ->267750 also admits the missing dependencies
of its existing whole Frame owner26C8B0; that parent itself is reused.
The Marine tick45 call2059F3 ->105F70 admits the missing nested dependencies
of the existing204AA0 owner, including its original scalar/CRT branches.
The current-source Marine tick63 call1B8612 ->1B7D20 admits the whole
decision-workspace owner and its missing nested planning/query dependencies.
The composed tick283 call1B8938 ->200C00 admits the original accumulated
path-distance owner, reusing its existing218780 reference-frame transform.
The tick288 indirect call1B0195 admits descriptor27's original1BD1E0
callback and1ED100 character-data owner, reusing its existing record iterator.
Reuse existing whole Frame owners and keep physical/indirect services required.
Only authenticated readonly operands are exported, never actor decisions,
mutable descriptor/context fields or successful gameplay returns.
"""
import argparse
import json
import os
from pathlib import Path
import re
from types import SimpleNamespace
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86 import X86_OP_MEM
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import generate,sha
from export_final_startup_storage_data_tu15 import regions

ROOT=Path(__file__).resolve().parents[1]
ROOTS={0x1b7000,0x1b7240,0x1ea360,0x1b7d20,0x200c00,0x1bd1e0}
FRAME_ROOTS={0x26c8b0,0x204aa0}
MODULE='source_campaign_ai_actor_behavior_tu15_generated.cpp'


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=Path,required=True);ap.add_argument('--data-output',type=Path,required=True)
    ap.add_argument('--directory',type=Path,required=True);args=ap.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    assert args.output.resolve().is_relative_to(scratch) and args.data_output.resolve().is_relative_to(scratch)
    assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
    owners={};sources={}
    for path in (ROOT/'src').rglob('*_generated.cpp'):
        if path.name==MODULE:continue
        source=path.read_text();sources[path.name]=source
        for pc in re.findall(r'^static void run_([0-9a-f]+)\(WorldState &s\) \{',source,re.M):
            owners.setdefault(int(pc,16),[]).append(path.name)
    assert not ROOTS&set(owners)
    assert code.at(0x1b0128).mnemonic=='call'
    assert code.at(0xe57ee).mnemonic=='call' and code.at(0xe57ee).operands[0].imm==0x1ea360
    assert code.at(0x26cb18).mnemonic=='call' and code.at(0x26cb18).operands[0].imm==0x267750
    assert code.at(0x2059f3).mnemonic=='call' and code.at(0x2059f3).operands[0].imm==0x105f70
    assert code.at(0x1b8612).mnemonic=='call' and code.at(0x1b8612).operands[0].imm==0x1b7d20
    assert code.at(0x1b8938).mnemonic=='call' and code.at(0x1b8938).operands[0].imm==0x200c00
    assert code.at(0x1b0195).mnemonic=='call' and code.at(0x1b0195).op_str=='eax'
    assert int.from_bytes(image.read(0x47b478+27*4,4),'little')==0x48abe0
    assert int.from_bytes(image.read(0x48abe0+0x10,4),'little')==0x1bd1e0
    entries=set(ROOTS);reused=set(FRAME_ROOTS);external=set();frame=sources['source_ai_frame_tu15_generated.cpp']
    for target in FRAME_ROOTS:
        assert 'source_ai_frame_tu15_generated.cpp' in owners[target]
        assert f'case 0x{target:x}u:run_{target:x}(s);break;' in frame
    visited=set()
    while True:
        more=set()
        for entry in (entries|reused)-visited:
            body=_trace(code,entry,set(owners)|entries|reused);visited.add(entry)
            for target in set(body.calls)|set(body.tail_jumps):
                if target in entries:continue
                if 'source_ai_frame_tu15_generated.cpp' in owners.get(target,[]):
                    assert f'case 0x{target:x}u:run_{target:x}(s);break;' in frame;reused.add(target)
                elif target in owners:
                    external.add(target)
                elif code.inside(target):more.add(target)
                else:external.add(target)
        entries.update(more)
        if not (entries|reused)-visited:break
    assert len(entries)==93
    sites,addresses,routines,points,reads,mutable_reads={},{},[],set(),[],[]
    switches=[]
    for entry in sorted(entries):
        body=_trace(code,entry,entries);tables=[]
        for address,width,count in body.tables:
            assert entry==0x1d2100 and address==0x1d216c and width==4 and count==9
            targets=[int.from_bytes(image.read(address+i*4,4),'little') for i in range(count)]
            assert set(targets)<=set(body.code)
            points.update(range(address,address+width*count));addresses[address]=address
            tables.append(dict(disc=address,count=count));switches.append(dict(entry=entry,address=address,count=count,targets=targets))
        sites[f'{entry:08x}']=sorted(body.code);routines.append(dict(entry=entry,tables=tables,byte_tables=[]))
        for pc in sorted(body.code):
            ins=code.at(pc);assert image.read(pc,ins.size)==bytes(ins.bytes)
            addresses[pc]=pc;addresses[pc+ins.size]=pc+ins.size
            for op in ins.operands:
                if op.type==2:addresses[op.imm&0xffffffff]=op.imm&0xffffffff
                elif op.type==X86_OP_MEM:
                    addresses[op.mem.disp&0xffffffff]=op.mem.disp&0xffffffff
                    if not op.mem.base and not op.mem.index:
                        sec=next((s for s in image.sections if s.va<=op.mem.disp<op.mem.disp+op.size<=s.va+s.vsize),None)
                        row=dict(pc=pc,address=op.mem.disp,bytes=op.size,instruction_hex=bytes(ins.bytes).hex())
                        if sec and sec.name=='.rdata':
                            assert op.access&CS_AC_READ and not op.access&CS_AC_WRITE
                            points.update(range(op.mem.disp,op.mem.disp+op.size));reads.append(row)
                        elif sec and op.access&CS_AC_READ:mutable_reads.append(row)
    paired=dict(image='English TU1.5',original_sha256=NEW_PIN,entries={f'{p:08x}':p for p in entries},
        body_sites=sites,instruction_map={f'{p:08x}':p for pcs in sites.values() for p in pcs},
        address_map={f'{p:08x}':p for p in addresses},routines=routines)
    args.directory.mkdir(parents=True,exist_ok=True);correspondence=args.directory/'bodies.json'
    correspondence.write_text(json.dumps(paired,indent=2,sort_keys=True)+'\n');manifest=args.directory/'generation.json'
    def original_exception_clear(ins):
        if ins.mnemonic!='fnclex':return None
        assert ins.address==0x32ece1 and bytes(ins.bytes)==bytes.fromhex('dbe2')
        # Clear the real backend's sticky flags and the retained SF/ES/B bits.
        # Logical condition codes and TOP retain their original ownership.
        return 'asm volatile("fnclex");s.fp.status&=0x7f00u;'
    generate(SimpleNamespace(output=args.output,manifest=manifest,correspondence=correspondence,
        additional_entries=Path(__file__)),paired,runtime_header='halo2/source_campaign_ai_actor_behavior_tu15.hpp',
        extra_instruction=original_exception_clear)
    source=args.output.read_text().replace('tools/generate_physics_live_support_tu15.py',
        'tools/generate_campaign_ai_actor_behavior_tu15.py').replace('source_run_physics_live_support_tu15',
        'source_run_campaign_ai_actor_behavior_body_tu15').replace('source_physics_live_support_tu15_owns_entry',
        'source_campaign_ai_actor_behavior_tu15_owns_entry')
    for header in ('physics_live_support_tu15','source_projectile_effect_observer','source_object_attachment_tu15'):
        source=source.replace(f'#include "halo2/{header}.hpp"\n','')
    source=source.replace('#include "physics/runtime/physics_world_runtime_internal.hpp"',
        '#include "physics/runtime/physics_world_runtime_internal.hpp"\n#include "halo2/source_ai_frame_tu15.hpp"')
    source+='''
namespace halo2 {
void source_run_campaign_ai_actor_behavior_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &abi,SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsWorldRuntimeServices &services,const SourcePhysicsContactRecordObserver &observe) {
    auto nested=services;
    nested.required_owner=[&](auto &bank,const auto &call,auto &state,const auto &stores) {
        const auto control=state.floating_point.value_or(fp);
        if(source_campaign_ai_actor_behavior_tu15_owns_entry(call.target)) {
            source_run_campaign_ai_actor_behavior_body_tu15(bank,call.target,state,control,nested,stores);return;
        }
        if('''+' || '.join(f'call.target==0x{p:x}u' for p in sorted(reused))+''') {
            source_run_ai_frame_tu15(bank,call.target,state,control,nested,stores);return;
        }
        if(services.required_owner){services.required_owner(bank,call,state,stores);return;}
        throw std::runtime_error("Required original campaign actor-behavior dependency unavailable");
    };
    source_run_campaign_ai_actor_behavior_body_tu15(memory,entry,abi,fp,nested,observe);
}
}
'''
    args.output.write_text(source)
    operands=[[p,image.read(p,n).hex()] for p,n in regions(points)]
    lines=['// Original actor behavior/scalar readonly operands; no live state or completed results.']
    for address,bits in operands:
        raw=bytes.fromhex(bits);lines.append(f'literal(memory,0x{address:x}u,std::array<std::uint8_t,{len(raw)}>{{'+
            ','.join(f'0x{b:02x}' for b in raw)+'});')
    args.data_output.write_text('\n'.join(lines)+'\n')
    report=json.loads(manifest.read_text());report.update(scope=__doc__,original_xbe_sha256=NEW_PIN,
        generated_cpp_sha256=sha(args.output),generator_sha256=sha(Path(__file__)),data_sha256=sha(args.data_output),
        entry_instructions={k:len(v) for k,v in sites.items()},original_instructions=sum(map(len,sites.values())),
        readonly_operands=operands,readonly_bytes=len(points),mutable_fixed_reads=mutable_reads,
        switch_tables=switches,
        reused_frame_entries=sorted(reused),required_external_entries=sorted(external),
        reached_frame_roots=sorted(FRAME_ROOTS),
        required_external_owners={f'{p:08x}':owners.get(p,[]) for p in sorted(external)},
        reused_frame_sha256=sha(ROOT/'src/campaign/ai/source_ai_frame_tu15_generated.cpp'))
    manifest.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(functions=len(entries),original_instructions=report['original_instructions'],readonly_bytes=len(points))))


if __name__=='__main__':main()
