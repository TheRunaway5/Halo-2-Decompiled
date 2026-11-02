#!/usr/bin/env python3
"""Stage genuine Host18A2A0 typed BEFORE observation; no native admission."""
import argparse,difflib,shutil
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source-root',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);stage=a.output/'staged';name='src/physics_live_world_tu15.cpp';source=a.source_root/name;before=source.read_text();after=before
 anchor='#include "halo2/native_tu15_look_controller_input_observer.hpp"';assert after.count(anchor)==1;after=after.replace(anchor,anchor+'\n#include "halo2/native_tu15_look_reorientation_input_observer.hpp"')
 anchor='        auto a=frame({});a.gp[0]=forward;a.gp[1]=0;run_actual(0x18a2a0u,a);';assert after.count(anchor)==1
 hook='''        auto a=frame({});a.gp[0]=forward;a.gp[1]=0;
        (void)native_tu15_observe_look_reorientation_input(bank,{0u,forward,
          {Bytes(completed_forward).u32(0),Bytes(completed_forward).u32(4),Bytes(completed_forward).u32(8)},
          {a.floating_point->x87_control_word,a.x87_status,a.floating_point->mxcsr},
          a.x87_depth,player,unit});
        run_actual(0x18a2a0u,a);''';after=after.replace(anchor,hook);target=stage/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_text(after)
 new=['include/halo2/native_tu15_look_reorientation_input_observer.hpp','tests/helpers/native_look_reorientation_input_capture_tu15.hpp']
 for relative in new:target=stage/relative;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/relative,target)
 (a.output/'delta.patch').write_text(''.join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='a/'+name,tofile='b/'+name)))
 save(a.output/'source-pins.json',dict(qualified=False,root_reviewed=False,peer_review_pending=True,diagnostic_only=True,native_admission_added=False,source_physics_mutated=False,base_source_root=str(a.source_root.resolve()),base_hashes={name:sha(source)},delta_files=[name],new_files=new,staged_hashes={str(p.relative_to(stage)):sha(p)for p in stage.rglob('*')if p.is_file()},scope='Actual Host reorient_retained_input18A2A0 formal index0/forward and genuine completed_forward DATA/currentFP/depth/caller identities BEFORE unchanged original call. No GP/SIMD/after/results. Per-memory/executor observer, bounded owned oneBEFORE capture, synchronous callback-span lifetime.'))
if __name__=='__main__':main()
