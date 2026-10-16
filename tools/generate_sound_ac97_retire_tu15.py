#!/usr/bin/env python3
"""Lower authentic channel teardown/shared-pool return; required frees stay external."""
import argparse,hashlib,json
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import generate

ENTRIES=[0x38f395,0x38f1a6,0x38f05c,0x38edc5,0x387a9a,0x38a8a4,0x38a768,0x383e0a]
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True);parser.add_argument('--manifest',type=Path,required=True)
    args=parser.parse_args();image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=_Code(image,'DSOUND');bodies={entry:_trace(code,entry,set(ENTRIES))for entry in ENTRIES}
    assert not any(body.tables for body in bodies.values())
    instructions,addresses={},{}
    for pc in sorted(set().union(*(body.code for body in bodies.values()))):
        op=code.at(pc);assert bytes(op.bytes)==image.read(pc,op.size)
        instructions[f'{pc:08x}']=pc;addresses[pc]=pc;addresses[pc+op.size]=pc+op.size
        for operand in op.operands:
            if operand.type==2:addresses[operand.imm&0xffffffff]=operand.imm&0xffffffff
            if operand.type==3:addresses[operand.mem.disp&0xffffffff]=operand.mem.disp&0xffffffff
    paired=dict(image='English TU1.5',original_sha256=NEW_PIN,entries={f'{e:08x}':e for e in ENTRIES},
        body_sites={f'{e:08x}':sorted(b.code)for e,b in bodies.items()},instruction_map=instructions,
        address_map={f'{p:08x}':v for p,v in addresses.items()},routines=[dict(entry=e,tables=[],byte_tables=[])for e in ENTRIES])
    args.correspondence=args.manifest.with_name('sound-ac97-retire-bodies.json');args.correspondence.parent.mkdir(parents=True,exist_ok=True)
    args.correspondence.write_text(json.dumps(paired,sort_keys=True,indent=2)+'\n');args.additional_entries=Path(__file__)
    generate(args,paired,complete_listings=True,code_override=code)
    text=args.output.read_text().replace('halo2/physics_live_support_tu15.hpp','halo2/source_sound_ac97_retire_tu15.hpp')
    text=text.replace('source_run_physics_live_support_tu15','source_run_sound_ac97_retire_tu15').replace('source_physics_live_support_tu15_owns_entry','source_sound_ac97_retire_tu15_owns_entry')
    # Original MOV writes an owned hardware action and never reads its prior
    # value. The generic store observer's preimage read is inappropriate here.
    action_site='w(s.memory,s.observe,0x38ede5u,'
    assert text.count(action_site)==1
    text=text.replace(action_site,'cold_start_stop_action(s.memory,s.observe,0x38ede5u,')
    helper="""void cold_start_stop_action(M& memory,const O& observe,std::uint32_t site,std::uint32_t address,unsigned n,std::uint64_t value){
    SourcePhysicsContactRecordStore row{site,address,{},std::vector<std::byte>(n)};
    for(unsigned i=0;i<n;++i)row.written[i]=std::byte(value>>(i*8));memory.write(address,row.written);if(observe)observe(row);
}
"""
    marker='using namespace physics_world_runtime_detail;\n'
    assert text.count(marker)==1
    text=text.replace(marker,marker+helper)
    args.output.write_text(text);report=json.loads(args.manifest.read_text())
    report.update(sound_ac97_output_generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),generated_cpp_sha256=hashlib.sha256(args.output.read_bytes()).hexdigest(),native_hardware_results_supplied=False,program_bytes_mounted=False)
    args.manifest.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');print(json.dumps(dict(bodies=len(ENTRIES),instructions=len(instructions))))
if __name__=='__main__':main()
