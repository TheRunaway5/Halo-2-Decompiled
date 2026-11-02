#!/usr/bin/env python3
"""Readonly232D20 service BEFORE hook; unchanged native refusal/admission."""
import argparse,difflib,shutil
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source-root',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--fix-context',action='store_true');a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);stage=a.output/'staged';name='src/physics/native_tu15_world_integration_residuals.cpp';source=a.source_root/name;before=source.read_text();after=before
 if a.fix_context:
  anchor='request.data,request.bytes,request.memory_class,context,depth});';assert after.count(anchor)==1;after=after.replace(anchor,'request.data,request.bytes,request.memory_class,context,state.x87_depth});')
 else:
  anchor='#include "halo2/native_tu15_world_integration_residuals.hpp"';assert after.count(anchor)==1;after=after.replace(anchor,anchor+'\n#include "halo2/native_tu15_pair_pool_release_input_observer.hpp"')
  anchor='''  pair_heap.release_owner=[&](const auto&request,auto&){
    if(request.owner!=0x233420u)''';assert after.count(anchor)==1
  after=after.replace(anchor,'''  pair_heap.release_owner=[&](const auto&request,auto&context){
    if(request.owner==0x232d20u)
      native_tu15_observe_pair_pool_release_input(m,{request.receiver,request.owner,
        request.data,request.bytes,request.memory_class,context,state.x87_depth});
    if(request.owner!=0x233420u)''')
 target=stage/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_text(after)
 new=['include/halo2/native_tu15_pair_pool_release_input_observer.hpp','tests/helpers/native_pair_pool_release_input_capture_tu15.hpp']
 for relative in new:target=stage/relative;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/relative,target)
 (a.output/'delta.patch').write_text(''.join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='a/'+name,tofile='b/'+name)))
 save(a.output/'source-pins.json',dict(qualified=False,root_reviewed=False,peer_review_pending=True,diagnostic_only=True,native_admission_added=False,source_physics_mutated=False,base_source_root=str(a.source_root.resolve()),base_hashes={name:sha(source)},delta_files=[name],new_files=new,staged_hashes={str(p.relative_to(stage)):sha(p)for p in stage.rglob('*')if p.is_file()},scope='Actual232D20 pool service INPUT allocator/owner/data/bytes/tag/FP/depth BEFORE unchanged refusal; no nested owner guessed. No GP/SIMD/after/outcomes. Per-memory/executor observer and bounded one owned BEFORE bankset; all serialization inside committed-span callback lifetimes.'))
if __name__=='__main__':main()
