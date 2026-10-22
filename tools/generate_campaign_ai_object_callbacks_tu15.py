#!/usr/bin/env python3
"""Lower both complete original object-callback families and missing callees.

456818/456878 name the callbacks, not a captured path or selected result.
Preserve every CFG branch and recover the missing direct dependency closure.
Reuse existing whole Frame owners; retained physics services remain required.
Export exact immutable operands, bounded switches and the second interface's
typed fields, never live object/context contents or successful callbacks.
"""
import argparse
import json
import os
from pathlib import Path
import re
import struct
from types import SimpleNamespace
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86 import X86_OP_MEM
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import generate,sha
from export_final_startup_storage_data_tu15 import regions

ROOT=Path(__file__).resolve().parents[1]
MODULE='source_campaign_ai_object_callbacks_tu15_generated.cpp'
PHYSICS={0xd2b60,0xd2bd0}
QUERY_CALLBACKS={0x267a40}
FRAME_QUERIES={0x267a30}


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=Path,required=True);ap.add_argument('--directory',type=Path,required=True)
    ap.add_argument('--data-output',type=Path,required=True);args=ap.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    assert args.output.resolve().is_relative_to(scratch) and args.data_output.resolve().is_relative_to(scratch)
    assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
    sources={p.name:p.read_text() for p in (ROOT/'src').rglob('*_generated.cpp') if p.name!=MODULE}
    owners={}
    for name,source in sources.items():
        for p in re.findall(r'^static void run_([0-9a-f]+)\(WorldState &s\) \{',source,re.M):
            owners.setdefault(int(p,16),[]).append(name)
    frame=sources['source_ai_frame_tu15_generated.cpp'];tables=[];callbacks=set();points=set()
    for base in (0x456818,0x456878):
        rows=[]
        for index in range(64):
            at=base+index*8;mask,padding,target=struct.unpack('<HHI',image.read(at,8))
            if base==0x456878:
                points.update(range(at+4,at+8))
                if target:points.update(range(at,at+2))
            rows.append(dict(address=at,mask=mask,target=target))
            if not target:break
            assert code.inside(target);callbacks.add(target)
        assert target==0;tables.append(dict(base=base,rows=rows))
    assert [len(t['rows'])-1 for t in tables]==[11,6]
    assert image.read(0x26acce,5).hex()=='bf7c684500'
    assert image.read(0x26acaf,7).hex()=='66893574d85200'
    assert image.read(0x26acb6,6).hex()=='891d78d85200'
    # Original1E7E00 registers this actual equality callback;140DD9 reaches
    # it during the native MCC query. Its six instructions retain RET8.
    assert image.read(0x1e7e00,10).hex()=='68407a260068307a2600'
    assert not set(owners)&QUERY_CALLBACKS
    for entry in FRAME_QUERIES:
        assert 'source_ai_frame_tu15_generated.cpp' in owners[entry]
        assert f'case 0x{entry:x}u:run_{entry:x}(s);break;' in frame
        original=_trace(code,entry,{entry})
        existing=frame.split(f'static void run_{entry:x}(WorldState &s) {{',1)[1].split('\n}',1)[0]
        assert len(original.code)==4
        assert {int(p,16) for p in re.findall(r'services\.instruction\(0x([0-9a-f]+)u?\)',existing)}==set(original.code)
    roots={p for p in callbacks if p not in owners}
    assert len(roots)==14
    entries=set(roots)|QUERY_CALLBACKS;reused=set(FRAME_QUERIES);external=set()
    while True:
        additional=set()
        for entry in sorted(entries):
            body=_trace(code,entry,entries)
            for target in set(body.calls)|set(body.tail_jumps):
                if target in entries:continue
                names=owners.get(target,[])
                if 'source_ai_frame_tu15_generated.cpp' in names:
                    assert f'case 0x{target:x}u:run_{target:x}(s);break;' in frame
                    reused.add(target)
                elif target in PHYSICS:
                    assert 'physics_live_support_tu15_generated.cpp' in names;external.add(target)
                elif names:
                    raise AssertionError(f'Reuse required existing owner {target:x}: {names}')
                elif code.inside(target):additional.add(target)
                else:external.add(target)
        if additional<=entries:break
        entries.update(additional)
    assert len(entries)==22
    sites,addresses,routines,reads={},{},[],[]
    for entry in sorted(entries):
        body=_trace(code,entry,entries);sites[f'{entry:08x}']=sorted(body.code)
        routines.append(dict(entry=entry,tables=[dict(disc=p,count=n) for p,w,n in body.tables if w==4],
            byte_tables=[dict(address=p,width=w,count=n,raw_hex=image.read(p,w*n).hex()) for p,w,n in body.tables if w==1]))
        for at,width,count in body.tables:
            if width==4:assert all(int.from_bytes(image.read(at+i,4),'little') in body.code for i in range(0,width*count,4))
            points.update(range(at,at+width*count))
        for pc in sorted(body.code):
            ins=code.at(pc);assert image.read(pc,ins.size)==bytes(ins.bytes)
            addresses[pc]=pc;addresses[pc+ins.size]=pc+ins.size
            for op in ins.operands:
                if op.type==2:addresses[op.imm&0xffffffff]=op.imm&0xffffffff
                elif op.type==X86_OP_MEM:
                    addresses[op.mem.disp&0xffffffff]=op.mem.disp&0xffffffff
                    if not op.mem.base and not op.mem.index:
                        sec=next((s for s in image.sections if s.va<=op.mem.disp<op.mem.disp+op.size<=s.va+s.vsize),None)
                        if sec is not None and sec.name=='.rdata':
                            assert op.access&CS_AC_READ and not op.access&CS_AC_WRITE
                            points.update(range(op.mem.disp,op.mem.disp+op.size))
                            reads.append(dict(pc=pc,address=op.mem.disp,bytes=op.size,instruction_hex=bytes(ins.bytes).hex()))
    paired=dict(image='English TU1.5',original_sha256=NEW_PIN,entries={f'{p:08x}':p for p in entries},
        body_sites=sites,instruction_map={f'{p:08x}':p for pcs in sites.values() for p in pcs},
        address_map={f'{p:08x}':p for p in addresses},routines=routines)
    args.directory.mkdir(parents=True,exist_ok=True)
    correspondence=args.directory/'bodies.json';correspondence.write_text(json.dumps(paired,indent=2,sort_keys=True)+'\n')
    manifest=args.directory/'generation.json'
    generate(SimpleNamespace(output=args.output,manifest=manifest,correspondence=correspondence,
        additional_entries=Path(__file__)),paired,runtime_header='halo2/source_campaign_ai_object_callbacks_tu15.hpp')
    source=args.output.read_text().replace('tools/generate_physics_live_support_tu15.py',
        'tools/generate_campaign_ai_object_callbacks_tu15.py').replace('source_run_physics_live_support_tu15',
        'source_run_campaign_ai_object_callbacks_body_tu15').replace('source_physics_live_support_tu15_owns_entry',
        'source_campaign_ai_object_callbacks_body_tu15_owns_entry')
    for header in ('physics_live_support_tu15','source_projectile_effect_observer','source_object_attachment_tu15'):
        source=source.replace(f'#include "halo2/{header}.hpp"\n','')
    source=source.replace('#include "physics/runtime/physics_world_runtime_internal.hpp"',
        '#include "physics/runtime/physics_world_runtime_internal.hpp"\n#include "halo2/source_ai_frame_tu15.hpp"')
    source+='''
namespace halo2 {
bool source_campaign_ai_object_callbacks_tu15_owns_entry(std::uint32_t entry) {
    return entry==0x267a30u || source_campaign_ai_object_callbacks_body_tu15_owns_entry(entry);
}
void source_run_campaign_ai_object_callbacks_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &abi,SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsWorldRuntimeServices &services,const SourcePhysicsContactRecordObserver &observe) {
    if(entry==0x267a30u) {
        source_run_ai_frame_tu15(memory,entry,abi,fp,services,observe);return;
    }
    auto nested=services;
    nested.required_owner=[&](auto &bank,const auto &call,auto &state,const auto &stores) {
        const auto control=state.floating_point.value_or(fp);
        if(source_campaign_ai_object_callbacks_body_tu15_owns_entry(call.target)) {
            source_run_campaign_ai_object_callbacks_body_tu15(bank,call.target,state,control,nested,stores);return;
        }
        if('''+' || '.join(f'call.target==0x{p:x}u' for p in sorted(reused))+''') {
            source_run_ai_frame_tu15(bank,call.target,state,control,nested,stores);return;
        }
        if(services.required_owner) {services.required_owner(bank,call,state,stores);return;}
        throw std::runtime_error("Required original campaign AI object-callback dependency unavailable");
    };
    source_run_campaign_ai_object_callbacks_body_tu15(memory,entry,abi,fp,nested,observe);
}
}
'''
    args.output.write_text(source)
    ranges=regions(points);lines=['// Original second object-callback interface, operands and bounded switches; no live state.']
    for address,count in ranges:
        raw=image.read(address,count)
        lines.append(f'literal(memory,0x{address:x}u,std::array<std::uint8_t,{count}>{{'+','.join(f'0x{v:02x}' for v in raw)+'});')
    args.data_output.write_text('\n'.join(lines)+'\n')
    report=json.loads(manifest.read_text());report.update(scope=__doc__,original_xbe_sha256=NEW_PIN,
        generated_cpp_sha256=sha(args.output),generator_sha256=sha(Path(__file__)),
        original_instructions=sum(map(len,sites.values())),entry_instructions={k:len(v) for k,v in sites.items()},
        callback_roots=sorted(roots),query_callbacks=sorted(QUERY_CALLBACKS),
        forwarded_frame_queries=sorted(FRAME_QUERIES),
        dependency_entries=sorted(entries-roots-QUERY_CALLBACKS),callback_tables=tables,
        reused_frame_entries=sorted(reused),required_external_entries=sorted(external),
        immutable_operand_reads=reads,immutable_ranges=ranges,immutable_bytes=sum(n for _,n in ranges),
        data_sha256=sha(args.data_output),reused_frame_sha256=sha(ROOT/'src/campaign/ai/source_ai_frame_tu15_generated.cpp'),
        source_behavior_results_supplied=False)
    manifest.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(functions=len(entries),original_instructions=report['original_instructions'],immutable_bytes=report['immutable_bytes'])))


if __name__=='__main__':main()
