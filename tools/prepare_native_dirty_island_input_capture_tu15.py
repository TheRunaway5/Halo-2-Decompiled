#!/usr/bin/env python3
"""Stage only a readonly actual282C10 BEFORE observation and owned helper."""
import argparse,difflib,shutil
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source-root',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);stage=a.output/'staged'
 relative='src/physics/native_tu15_physics_frame_residuals.cpp';source=a.source_root/relative;before=source.read_text();after=before;anchor='#include "halo2/native_tu15_physics_frame_residuals.hpp"';assert after.count(anchor)==1;after=after.replace(anchor,anchor+'\n#include "halo2/native_tu15_dirty_island_objects_input_observer.hpp"')
 anchor='  s.post_step=[&](auto&f){call(0x282c10u,{},{},{},0,f);};';assert after.count(anchor)==1;after=after.replace(anchor,'''  s.post_step=[&](auto&f){
    native_tu15_observe_dirty_island_objects_input(m,{f,state.x87_depth});
    call(0x282c10u,{},{},{},0,f);
  };''')
 target=stage/relative;target.parent.mkdir(parents=True,exist_ok=True);target.write_text(after)
 new=['include/halo2/native_tu15_dirty_island_objects_input_observer.hpp','tests/helpers/native_dirty_island_objects_input_capture_tu15.hpp']
 for name in new:target=stage/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,target)
 (a.output/'delta.patch').write_text(''.join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='a/'+relative,tofile='b/'+relative)))
 save(a.output/'source-pins.json',dict(qualified=False,root_reviewed=False,peer_review_pending=True,diagnostic_only=True,native_admission_added=False,source_physics_mutated=False,base_source_root=str(a.source_root.resolve()),base_hashes={relative:sha(source)},delta_files=[relative],new_files=new,staged_hashes={str(p.relative_to(stage)):sha(p)for p in stage.rglob('*')if p.is_file()},scope='Actual nativeframe post_step zero-formal282C10 BEFORE its unchanged original dispatch. Only actualFP/depth and const committed banks, no GP/SIMD/after/outcome. First source-enabled nonreentrant +AE dirty island in either current World cohort; bounded readonly admission, no flags/rows manufactured. One owned full BEFORE archive only.'))
if __name__=='__main__':main()
