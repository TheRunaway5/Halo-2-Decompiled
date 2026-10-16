#!/usr/bin/env python3
"""Lower whole codec/channel constructor; native hardware-only leaves remain explicit."""
import argparse,hashlib,json
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import generate

ENTRIES=[0x38e484,0x38f45f,0x38edad,0x38ed90]
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
    args.correspondence=args.manifest.with_name('sound-codec-initialization-bodies.json');args.correspondence.parent.mkdir(parents=True,exist_ok=True)
    args.correspondence.write_text(json.dumps(paired,sort_keys=True,indent=2)+'\n');args.additional_entries=Path(__file__)
    generate(args,paired,complete_listings=True,code_override=code)
    text=args.output.read_text().replace('halo2/physics_live_support_tu15.hpp','halo2/source_sound_codec_initialization_tu15.hpp')
    text=text.replace('source_run_physics_live_support_tu15','source_run_sound_codec_initialization_tu15').replace('source_physics_live_support_tu15_owns_entry','source_sound_codec_initialization_tu15_owns_entry')
    for pc in ['38f49e','38eda2']:
        assert f'w(s.memory,s.observe,0x{pc}u,' in text
        text=text.replace(f'w(s.memory,s.observe,0x{pc}u,',f'codec_action_write(s.memory,s.observe,0x{pc}u,')
    insert='''void codec_action_write(M &m,const O &o,std::uint32_t site,std::uint32_t address,unsigned n,std::uint64_t value) {
    // Original MOV is a pure native action write, never a physical readback.
    SourcePhysicsContactRecordStore row{site,address,{},{}};row.written.resize(n);
    for(unsigned i=0;i<n;++i)row.written[i]=std::byte(value>>(8*i));
    m.write(address,row.written);if(o)o(row);
}
'''
    text=text.replace('using WorldState=LiveState;', 'using WorldState=LiveState;\n'+insert)
    args.output.write_text(text);report=json.loads(args.manifest.read_text())
    report.update(sound_codec_initialization_generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),generated_cpp_sha256=hashlib.sha256(args.output.read_bytes()).hexdigest(),native_functional_abi_entries=[0x38ed2c,0x38ecff],unknown_action_preimage_sites=[0x38f49e,0x38eda2],native_hardware_results_supplied=False,program_bytes_mounted=False)
    args.manifest.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');print(json.dumps(dict(bodies=len(ENTRIES),instructions=len(instructions))))
if __name__=='__main__':main()
