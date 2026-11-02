#!/usr/bin/env python3
"""Readonly31D930 typed BEFORE hook, no native admission or caller outcomes."""
import argparse,difflib,shutil
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source-root',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);stage=a.output/'staged';name='src/physics/native_tu15_constraint_step_residuals.cpp';source=a.source_root/name;before=source.read_text();after=before;anchor='#include "halo2/native_tu15_constraint_step_residuals.hpp"';assert after.count(anchor)==1;after=after.replace(anchor,anchor+'\n#include "halo2/native_tu15_constraint_frame_build_input_observer.hpp"')
 anchor='''        const std::array args{jacobian_input,packet};
        call(owner.identity,args,constraint.identity,8,context);''';assert after.count(anchor)==1
 after=after.replace(anchor,'''        if(owner.identity==0x31d930u)
          native_tu15_observe_constraint_frame_build_input(m,{constraint.identity,jacobian_input,packet,
            {word(m,packet),word(m,packet+4u),word(m,packet+8u),word(m,packet+12u)},context,depth});
'''+anchor);target=stage/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_text(after)
 new=['include/halo2/native_tu15_constraint_frame_build_input_observer.hpp','tests/helpers/native_constraint_frame_build_input_capture_tu15.hpp']
 for relative in new:target=stage/relative;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/relative,target)
 (a.output/'delta.patch').write_text(''.join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='a/'+name,tofile='b/'+name)))
 save(a.output/'source-pins.json',dict(qualified=False,root_reviewed=False,peer_review_pending=True,diagnostic_only=True,native_admission_added=False,source_physics_mutated=False,base_source_root=str(a.source_root.resolve()),base_hashes={name:sha(source)},delta_files=[name],new_files=new,staged_hashes={str(p.relative_to(stage)):sha(p)for p in stage.rglob('*')if p.is_file()},scope='Actual31D930 native constraint-step formals/FP/depth plusfourcurrent packet words BEFORE unchanged originalcall. No GP/SIMD/after/rows/outcomes. Per-memory/executor observer, bounded owned oneBEFORE capture; borrowed spans copied/serialized only inside callbacks.'))
if __name__=='__main__':main()
