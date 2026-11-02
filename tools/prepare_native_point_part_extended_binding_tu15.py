#!/usr/bin/env python3
"""Private point-header overlay; preserve the qualified bounded production stage."""
import argparse,difflib,json,shutil
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--base',required=True,type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.base=a.base.resolve();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);stage=a.output/'staged';relative='include/halo2/native_tu15_contact_point_binding.hpp';source=a.base/'staged'/relative;text=source.read_text();original=text
 for old,new in(('"halo2/native_tu15_part_contact_binding.hpp"','"halo2/native_tu15_part_contact_binding_extended.hpp"'),('NativeTu15PartContactBindingActivity part;','NativeTu15PartContactExtendedBindingActivity part;'),('native_tu15_update_bound_part_contact_response(','native_tu15_update_bound_part_contact_response_extended(')):
  assert text.count(old)==1;text=text.replace(old,new)
 text=text.replace('unsupported authored-function kinds refuse','unsupported authored numerical profiles refuse')
 destination=stage/relative;destination.parent.mkdir(parents=True);destination.write_text(text)
 helper='include/halo2/native_tu15_part_contact_binding_extended.hpp';shutil.copyfile(ROOT/helper,stage/helper)
 (a.output/'delta.patch').write_text(''.join(difflib.unified_diff(original.splitlines(True),text.splitlines(True),fromfile='a/'+relative,tofile='b/'+relative)))
 save(a.output/'source-pins.json',dict(qualified=False,working_tree_modified=False,base_stage=str(a.base),base_point_header_sha256=sha(source),staged_hashes={relative:sha(destination),helper:sha(stage/helper)},
  scope='Only typed PART helper/activity changes to extended native function owner; all point scratch/admission/lifetime statements unchanged. No World/getter/runtime wiring yet.'))
 print(dict(prepared=True,shared_headers_modified=1));return 0
if __name__=='__main__':raise SystemExit(main())
