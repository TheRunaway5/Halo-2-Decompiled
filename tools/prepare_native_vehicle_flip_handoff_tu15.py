#!/usr/bin/env python3
"""Stage only the authenticated F24D8 native flip consumer and its generator."""
import argparse,difflib,json,os
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT
from validate_native_contact_agent_dispatch_tu15 import sha,save
HELPER='''static bool handoff_native_vehicle_flip(VehicleState&s) {
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    auto actual=s.save();
    try {
        if(!native_tu15_handoff_vehicle_flip(s.memory,{0xf24d8u,0xf1dd0u,false},actual,s.observe))return false;
    } catch(...) {s.load(actual);throw;}
    s.load(actual);return true;
}
'''
def once(text,old,new):assert text.count(old)==1,(old,text.count(old));return text.replace(old,new,1)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);cpp=Path('src/physics/vehicles/physics_vehicle_runtime_generated.cpp');gen=Path('tools/generate_vehicle_runtime_20261011.py');paths=[cpp,gen];old=[(ROOT/f).read_text()for f in paths];source,generator=old
 include='#include "halo2/source_physics_entry_observation.hpp"';extra='\n#include "halo2/native_tu15_vehicle_flip_handoff.hpp"';source=once(source,include,include+extra);source=once(source,'using namespace physics_vehicle_runtime_detail;\n','using namespace physics_vehicle_runtime_detail;\n'+HELPER)
 line='if(s.services.instruction){s.services.instruction(0xf15b8u);} {const auto target=selected(0xf0eb0u,s.image);s.push(0xf15b8u,selected(0xf15bdu,s.image));dispatch(s,target,0xf15b8u,false);}'
 source=once(source,line,line.replace('dispatch(s,target,0xf15b8u,false);','if(!handoff_native_vehicle_flip(s))dispatch(s,target,0xf15b8u,false);'))
 generator=once(generator,include,include+extra);generator=once(generator,'using namespace physics_vehicle_runtime_detail;\n','using namespace physics_vehicle_runtime_detail;\n'+HELPER)
 needle="            target=get(i,op[0]);externals[f'{p:08x}']=dict(target=i.op_str,full_guest_abi=True)\n"
 generator=once(generator,needle,needle+'''            if p==0xf15b8:
                assert op[0].type==2 and op[0].imm==0xf0eb0 and next==0xf15bd
                return '{const auto target='+target+f';s.push(0x{p:x}u,{imm(next)});if(!handoff_native_vehicle_flip(s))dispatch(s,target,0x{p:x}u,false);'+'}'
''')
 patch=[]
 for path,before,after in zip(paths,old,[source,generator]):
  target=a.output/'staged'/path;target.parent.mkdir(parents=True,exist_ok=True);target.write_text(after);patch.extend(difflib.unified_diff(before.splitlines(keepends=True),after.splitlines(keepends=True),fromfile='a/'+str(path),tofile='b/'+str(path)))
 delta=a.output/'native-vehicle-flip-consumer.diff';delta.write_text(''.join(patch));files=[ROOT/f'include/halo2/native_tu15_vehicle_flip{name}.hpp'for name in ['', '_binding','_handoff']]+[ROOT/f'src/physics/native_tu15_vehicle_flip{name}.cpp'for name in ['', '_binding','_handoff']]+[Path(__file__)]
 save(a.output/'source-pins.json',dict(schema='native-tu15-vehicle-flip-handoff-source-delta-v1',qualified=False,working_tree_modified=False,base_hashes={str(f):sha(ROOT/f)for f in paths},staged_hashes={str(f):sha(a.output/'staged'/f)for f in paths},native_source_hashes={str(f.relative_to(ROOT)):sha(f)for f in files},patch=dict(path=str(delta),sha256=sha(delta)),required_compile_units=[f'src/physics/native_tu15_vehicle_flip{name}.cpp'for name in ['', '_binding','_handoff']],retained_large_artifacts=[],owned_scratch=os.environ['HALO2_TEST_SCRATCH']));print(delta)
if __name__=='__main__':main()
