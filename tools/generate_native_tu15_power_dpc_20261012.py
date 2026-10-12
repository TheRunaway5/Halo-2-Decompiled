#!/usr/bin/env python3
"""Compile the complete authenticated original TU1.5 power callback."""
import argparse,hashlib,json
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--cpp',type=Path,required=True);ap.add_argument('--manifest',type=Path,required=True);a=ap.parse_args()
 assert sha(NEW_DEFAULT)==NEW_PIN
 image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');trace=_trace(code,0x2e009d,{0x2e009d})
 expected={0x2e009d:('cmp','dword ptr [0x56ee08], 0'),0x2e00a4:('je','0x2e00ac'),0x2e00a6:('call','dword ptr [0x41b704]'),0x2e00ac:('ret','0x10')}
 assert set(trace.code)==set(expected)
 rows={}
 for pc,pair in expected.items():
  i=code.at(pc);assert (i.mnemonic,i.op_str)==pair and image.read(pc,len(i.bytes))==i.bytes
  rows[f'{pc:08x}']=dict(raw=i.bytes.hex(),mnemonic=i.mnemonic,operand=i.op_str)
 assert image.read(0x41b704,4).hex()=='68010080'
 a.cpp.write_text('''// Complete authenticated TU1.5 power DPC; no production decoder.
#include "physics_world_runtime_internal.hpp"
#include "halo2/native_tu15_startup_timer.hpp"
namespace halo2 {
void source_run_tu15_power_dpc(SourcePhysicsContactMemory &memory,SourcePhysicsContactRuntimeAbi &abi,
    SourcePhysicsFactoryFloatingPoint fp,const SourcePhysicsContactRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &observe) {
    using namespace physics_world_runtime_detail;
    FpScope scope(fp);WorldState s{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};s.load(abi);
    try {
        if(services.instruction)services.instruction(0x2e009du);s.flags.minus(r(memory,0x56ee08u,4),0,4);
        if(services.instruction)services.instruction(0x2e00a4u);
        if(!s.flags.zf) {
            if(services.instruction)services.instruction(0x2e00a6u);
            s.push(0x2e00a6u,0x2e00acu);s.required(0x2e00a6u,r(memory,0x41b704u,4),false);
        }
        if(services.instruction)services.instruction(0x2e00acu);s.pop();s.gp[4]+=16u;
    }catch(...) {abi=s.save();throw;}
    abi=s.save();
}
}
''')
 m=dict(schema=1,original_sha256=NEW_PIN,generator_sha256=sha(__file__),generated_sha256=sha(a.cpp),complete_bodies=1,sites=4,
  source_instructions=rows,production_decoder=False,production_original_code_execution=False,shutdown_owner_still_required=True)
 a.manifest.write_text(json.dumps(m,sort_keys=True,indent=2)+'\n');print(sha(a.manifest))
if __name__=='__main__':main()
