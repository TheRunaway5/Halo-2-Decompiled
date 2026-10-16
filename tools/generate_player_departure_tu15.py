#!/usr/bin/env python3
"""Lower complete authenticated TU1.5 ordinary-departure owners to C++."""
import argparse,json
from pathlib import Path
from types import SimpleNamespace
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import generate,sha

# 88C50's actual identity gate ->155920 ->15A860. Result remapping retains
# allocated left-player identity/stat rows. Slot7's actual Slayer RET4 owner
# and complete stack probe are included without a fabricated service return.
# Active original session cleanup uses the complete 6A9A0 row reset and
# 6A890 typed-row iterator; neither callee has an external dependency.
ENTRIES=(0x6a890,0x6a9a0,0x88c50,0x14ead0,0x155920,0x15a860,0x15a950,0x19ae70,0x19dd10,0x19dc50,0x19dbf0,0x2e4d60,0x32a660)
def departure_instructions(*,get,put,**unused):
    def emit(i):
        if i.address!=0x32a674:return None
        assert i.bytes.hex()=='94' and i.mnemonic=='xchg' and i.op_str=='esp, eax'
        left,right=i.operands
        # The original stack probe exchanges the two full registers without
        # changing flags. Both inputs must be read before either write.
        return '{const auto left='+get(i,left)+';const auto right='+get(i,right)+';'+put(i,left,'right')+put(i,right,'left')+'}'
    return emit
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,default=Path('src/source_player_departure_tu15_generated.cpp'))
    parser.add_argument('--directory',type=Path,default=Path('analysis/multiplayer/player-departure-tu15'))
    args=parser.parse_args();assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');sites={};instructions={};addresses={};routines=[]
    assert image.read(0x88c50,8).hex()=='53558b2df4544f00'
    assert image.read(0x155966,7).hex()=='80890b01000004'
    assert image.read(0x155a79,4).hex()=='804f0202'
    for entry in ENTRIES:
        body=_trace(code,entry,set(ENTRIES));assert body.code
        assert all(width in (1,4)for _,width,_ in body.tables)
        sites[f'{entry:08x}']=sorted(body.code)
        routines.append(dict(entry=entry,tables=[dict(disc=p,count=n)for p,w,n in body.tables if w==4],
            byte_tables=[dict(address=p,width=w,count=n,raw_hex=image.read(p,w*n).hex())for p,w,n in body.tables if w==1]))
        for pc in sorted(body.code):
            i=code.at(pc);assert image.read(pc,i.size)==bytes(i.bytes)
            instructions[f'{pc:08x}']=pc;addresses[pc]=pc;addresses[pc+i.size]=pc+i.size
            for operand in i.operands:
                if operand.type==2:addresses[operand.imm&0xffffffff]=operand.imm&0xffffffff
                elif operand.type==3:addresses[operand.mem.disp&0xffffffff]=operand.mem.disp&0xffffffff
    paired=dict(image='English TU1.5',original_sha256=NEW_PIN,entries={f'{p:08x}':p for p in ENTRIES},
        body_sites=sites,instruction_map=instructions,address_map={f'{p:08x}':q for p,q in addresses.items()},routines=routines)
    args.directory.mkdir(parents=True,exist_ok=True);correspondence=args.directory/'bodies.json'
    correspondence.write_text(json.dumps(paired,indent=2,sort_keys=True)+'\n')
    generate(SimpleNamespace(output=args.output,manifest=args.directory/'generation.json',correspondence=correspondence,
        additional_entries=Path(__file__)),paired,extra_instruction_factory=departure_instructions)
    source=args.output.read_text().replace('tools/generate_physics_live_support_tu15.py','tools/generate_player_departure_tu15.py')
    source=source.replace('halo2/physics_live_support_tu15.hpp','halo2/source_player_departure_tu15.hpp')
    source=source.replace('source_run_physics_live_support_tu15','source_run_player_departure_tu15')
    source=source.replace('source_physics_live_support_tu15_owns_entry','source_player_departure_tu15_owns_entry')
    args.output.write_text(source)
    report=json.loads((args.directory/'generation.json').read_text());report['generated_cpp_sha256']=sha(args.output)
    (args.directory/'generation.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
if __name__=='__main__':main()
