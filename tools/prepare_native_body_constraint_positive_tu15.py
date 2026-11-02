#!/usr/bin/env python3
"""Stage the source1F3460 DATA classification read without touching production."""
import argparse,difflib,json
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
ROOT=Path(__file__).resolve().parent.parent
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 relative=Path('src/physics/native_tu15_body_constraint_update.cpp');source=ROOT/relative;text=source.read_text()
 needle='''        return{model+offsets[type],strides[type]};'''
 replacement='''        // Source1F3460 reads this authenticated classification DATA before
        // entering its native enum branch. Never execute the stored code ID.
        constexpr std::array<std::uint32_t,6>targets{
            0x1f346au,0x1f3476u,0x1f3482u,0x1f348eu,0x1f349au,0x1f34a6u};
        if(word(0x1f34b4u+std::uint32_t(type)*4u)!=targets[type]){
            if(stats_)++stats_->unknown_definition_kinds;
            throw std::runtime_error("Native TU1.5 model constraint definition has an unknown authored classification");
        }
        return{model+offsets[type],strides[type]};'''
 assert text.count(needle)==1;staged=a.output/'staged'/relative;staged.parent.mkdir(parents=True);staged.write_text(text.replace(needle,replacement))
 patch=a.output/'native-body-constraint-classification.diff';patch.write_text(''.join(difflib.unified_diff(text.splitlines(True),staged.read_text().splitlines(True),fromfile='a/'+str(relative),tofile='b/'+str(relative))))
 save(a.output/'source-pins.json',dict(qualified=False,working_tree_modified=False,base_hashes={str(relative):sha(source)},staged_hashes={str(relative):sha(staged)},patch=dict(path=str(patch),sha256=sha(patch)),source_hashes={str(Path(__file__).relative_to(ROOT)):sha(Path(__file__))},remaining=['Positive whole-parent physical qualification','Production application and live native invocation']))
 print(json.dumps({'staged':str(staged),'sha256':sha(staged)}));return 0
if __name__=='__main__':raise SystemExit(main())
