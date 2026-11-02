#!/usr/bin/env python3
"""Stage a public native17A030 wrapper; leave the qualified currentCPP fixed."""
import argparse,difflib,os
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT
from validate_native_contact_agent_dispatch_tu15 import sha,save
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False)
 path=Path('src/physics/native_tu15_constraint_runtime_handles.cpp');before=(ROOT/path).read_text()
 needle='void native_tu15_release_constraint_runtime_handle(SourcePhysicsContactMemory&m,';assert before.count(needle)==1
 wrapper='''void native_tu15_cleanup_constraint_handle_tagged_values(SourcePhysicsContactMemory&m,
    std::uint32_t record,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15ConstraintRuntimeHandleServices&services,NativeTu15ConstraintRuntimeHandleStats*stats){
    cleanup_tagged_values(m,record,fp,services,stats);
}
'''
 after=before.replace('#include "halo2/native_tu15_constraint_runtime_handles.hpp"',
  '#include "halo2/native_tu15_constraint_runtime_handles.hpp"\n#include "halo2/native_tu15_constraint_tag_cleanup.hpp"',1).replace(needle,wrapper+needle,1)
 target=a.output/'staged'/path;target.parent.mkdir(parents=True,exist_ok=True);target.write_text(after)
 patch=a.output/'native-constraint-handle-cleanup-api.diff';patch.write_text(''.join(difflib.unified_diff(before.splitlines(keepends=True),after.splitlines(keepends=True),fromfile='a/'+str(path),tofile='b/'+str(path))))
 save(a.output/'source-pins.json',dict(schema='native-tu15-constraint-handle-cleanup-api-v1',working_tree_modified=False,qualified=False,
  base_hashes={str(path):sha(ROOT/path)},staged_hashes={str(path):sha(target)},
  source_hashes={'include/halo2/native_tu15_constraint_tag_cleanup.hpp':sha(ROOT/'include/halo2/native_tu15_constraint_tag_cleanup.hpp'),str(Path(__file__).relative_to(ROOT)):sha(Path(__file__))},
  patch=dict(path=str(patch),sha256=sha(patch)),owned_scratch=os.environ['HALO2_TEST_SCRATCH'],retained_large_artifacts=[]))
 print(patch);return 0
if __name__=='__main__':raise SystemExit(main())
