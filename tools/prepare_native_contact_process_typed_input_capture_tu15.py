#!/usr/bin/env python3
"""Stage read-only typed2E5320/304350 inputs before both unchanged World call routes."""
import argparse,difflib,shutil
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source-root',required=True,type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);stage=a.output/'staged';name='src/physics/native_tu15_world_integration_residuals.cpp';source=a.source_root/name;before=source.read_text();after=before
 anchor='#include "halo2/native_tu15_world_integration_residuals.hpp"';assert after.count(anchor)==1;after=after.replace(anchor,anchor+'\n#include "halo2/native_tu15_contact_process_typed_input_observer.hpp"')
 anchor='''  remaining_contact_process.process_child=[&](const auto&child,auto&){
    const std::array args{child.first.identity,child.second.identity,''';replacement='''  remaining_contact_process.process_child=[&](const auto&child,auto&current){
    if(child.owner.identity==0x2e5320u||child.owner.identity==0x304350u)
      native_tu15_observe_contact_process_typed_input(m,{child.owner.identity,child.receiver.identity,child.first.identity,
        child.second.identity,child.configuration.identity,child.output.identity,current,state.x87_depth});
    const std::array args{child.first.identity,child.second.identity,''';assert after.count(anchor)==1;after=after.replace(anchor,replacement)
 anchor='''    const std::array args{first.identity,second.identity,configuration.identity,output.identity};
    call(owner.identity,args,agent.identity,16);''';replacement='''    if(owner.identity==0x2e5320u||owner.identity==0x304350u)
      native_tu15_observe_contact_process_typed_input(m,{owner.identity,agent.identity,first.identity,second.identity,
        configuration.identity,output.identity,context,state.x87_depth});
'''+anchor;assert after.count(anchor)==1;after=after.replace(anchor,replacement)
 target=stage/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_text(after)
 new=['include/halo2/native_tu15_contact_process_typed_input_observer.hpp','tests/helpers/native_contact_process_typed_input_capture_tu15.hpp']
 for relative in new:target=stage/relative;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/relative,target)
 (a.output/'delta.patch').write_text(''.join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='a/'+name,tofile='b/'+name)))
 save(a.output/'source-pins.json',dict(qualified=False,root_reviewed=False,diagnostic_only=True,native_admission_added=False,source_physics_mutated=False,base_source_root=str(a.source_root.resolve()),base_hashes={name:sha(source)},delta_files=[name],new_files=new,staged_hashes={str(p.relative_to(stage)):sha(p)for p in stage.rglob('*')if p.is_file()},scope='Actual2E5320/304350 top-level and native-wrapper remaining child DATA/FP/depth BEFORE unchanged owner/counters/call. No GP/SIMD, CPU-frame, after/contact/physics outcome. Per-memory/executor observer, explicit actual-owner filter, bounded ONE owned BEFORE with synchronous callback-lifetime serialization.'))
if __name__=='__main__':main()
