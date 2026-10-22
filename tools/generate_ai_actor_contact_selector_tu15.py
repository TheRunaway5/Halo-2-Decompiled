#!/usr/bin/env python3
"""Lower the authenticated complete272200 selector; reuse the native267410 lookup."""
import argparse,json
from pathlib import Path
from types import SimpleNamespace
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import generate,sha

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=Path,required=True)
    ap.add_argument('--directory',type=Path,required=True)
    a=ap.parse_args();assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');entry=0x272200
    assert image.read(entry,28).hex()=='8b883803000083f9ff740de80052ffff85c07404668b00c36633c0c3'
    assert image.read(0x267410,59).hex()=='8b15f0ee500081e1ffff00006bc93c56034a448b491433c083f9ff741c8b15ecee50008b724481e1ffff000069c92401000003ce74038d41705ec3'
    body=_trace(code,entry,{entry});sites=sorted(body.code)
    assert sites==[0x272200,0x272206,0x272209,0x27220b,0x272210,0x272212,0x272214,0x272217,0x272218,0x27221b]
    addresses={};instructions={}
    for pc in sites:
        i=code.at(pc);assert image.read(pc,i.size)==bytes(i.bytes)
        instructions[f'{pc:08x}']=pc;addresses[pc]=pc;addresses[pc+i.size]=pc+i.size
        for op in i.operands:
            if op.type==2:addresses[op.imm&0xffffffff]=op.imm&0xffffffff
            elif op.type==3:addresses[op.mem.disp&0xffffffff]=op.mem.disp&0xffffffff
    paired=dict(image='English TU1.5',original_sha256=NEW_PIN,entries={f'{entry:08x}':entry},
        body_sites={f'{entry:08x}':sites},instruction_map=instructions,
        address_map={f'{pc:08x}':pc for pc in addresses},routines=[dict(entry=entry,tables=[],byte_tables=[])])
    a.directory.mkdir(parents=True,exist_ok=True);correspondence=a.directory/'bodies.json'
    correspondence.write_text(json.dumps(paired,indent=2,sort_keys=True)+'\n')
    manifest=a.directory/'generation.json'
    generate(SimpleNamespace(output=a.output,manifest=manifest,correspondence=correspondence,
        additional_entries=Path(__file__)),paired,
        runtime_header='halo2/source_ai_actor_contact_selector_tu15.hpp')
    source=a.output.read_text().replace('tools/generate_physics_live_support_tu15.py',
        'tools/generate_ai_actor_contact_selector_tu15.py').replace(
        'source_run_physics_live_support_tu15','source_run_ai_actor_contact_selector_body_tu15').replace(
        'source_physics_live_support_tu15_owns_entry','source_ai_actor_contact_selector_tu15_owns_entry')
    source=source.replace('#include "physics/runtime/physics_world_runtime_internal.hpp"',
        '#include "physics/runtime/physics_world_runtime_internal.hpp"\n#include "halo2/source_ai_frame_tu15.hpp"')
    source+='''
namespace halo2 {
void source_run_ai_actor_contact_selector_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &abi,
    SourcePhysicsFactoryFloatingPoint fp,const SourcePhysicsWorldRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &observe) {
    auto nested=services;
    nested.required_owner=[&](SourcePhysicsContactMemory &callee_memory,
        const SourcePhysicsContactRuntimeCall &call,SourcePhysicsContactRuntimeAbi &callee_abi,
        const SourcePhysicsContactRecordObserver &callee_observe) {
        if(call.target==0x267410u) {
            source_run_ai_frame_tu15(callee_memory,call.target,callee_abi,
                callee_abi.floating_point.value_or(fp),services,callee_observe);
            return;
        }
        if(services.required_owner) {
            services.required_owner(callee_memory,call,callee_abi,callee_observe);
            return;
        }
        throw std::runtime_error("Required original AI contact selector callee unavailable");
    };
    source_run_ai_actor_contact_selector_body_tu15(memory,entry,abi,fp,nested,observe);
}
} // namespace halo2
'''
    a.output.write_text(source);report=json.loads(manifest.read_text())
    report.update(generated_cpp_sha256=sha(a.output),generator_sha256=sha(Path(__file__)),
        scope=__doc__,original_selector_bytes=28,reused_lookup_entry=0x267410,
        original_selector_sha256='70cb65c4fe0983f081baa4160ff4ca2b834953be5d114c2d38490213a0afb554',
        original_lookup_sha256='0989e2758936f32971c6a3985aa6a534c7dcec82c928b6c8327f527d343b7287')
    manifest.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')

if __name__=='__main__':main()
