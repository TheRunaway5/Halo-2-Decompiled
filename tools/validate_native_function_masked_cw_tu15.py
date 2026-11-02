#!/usr/bin/env python3
"""Private masked-CW overlay, full parent on one actual x87 precision policy."""
import argparse,json,os,shutil,sys
from pathlib import Path
import validate_native_function_evaluation_extended_tu15 as parent
from build_native_tu15_constraint_before_reference import sha,save
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',required=True,type=Path);p.add_argument('--precision',required=True,type=int,choices=[0,2,3]);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.stage=a.stage.resolve();a.output=a.output.resolve();owner=Path(os.environ['HALO2_TEST_SCRATCH']);source=owner/'source';source.mkdir();shutil.copytree(ROOT/'include',source/'include')
 names=['src/core/native_tu15_function_evaluation_extended.cpp','src/core/native_tu15_function_curves.cpp','src/core/native_tu15_function_control_point.cpp',
  'tools/observe_native_function_evaluation_extended_tu15.cpp','tools/observe_native_function_evaluation_tu15.cpp','tools/observe_native_activation_agents_tu15.cpp','tools/validate_native_function_evaluation_extended_tu15.py']
 for name in names:
  candidate=a.stage/'staged'/name;candidate=candidate if candidate.exists()else ROOT/name;target=source/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(candidate,target)
 driver=source/'tools/observe_native_function_evaluation_extended_tu15.cpp';text=driver.read_text();needle='if(kind>=9u&&(mode!=0u||(parameter==0xbf000000u&&(kind==10u?authored!=6u:authored==0u))))continue;';assert text.count(needle)==1;text=text.replace(needle,'if(kind>=9u&&(parameter==0xbf000000u&&(kind==10u?authored!=6u:authored==0u)))continue;')
 text=text.replace('before.cwd=0x27fu|(mode<<10u)','before.cwd='+hex(0x7f|(a.precision<<8))+'u|(mode<<10u)').replace('out<<report.dump(2)','out<<report.dump()');driver.write_text(text)
 bounded=source/'tools/observe_native_function_evaluation_tu15.cpp';text=bounded.read_text();assert 'before.cwd=0x27fu|(c.mode<<10u)'in text;bounded.write_text(text.replace('before.cwd=0x27fu|(c.mode<<10u)','before.cwd='+hex(0x7f|(a.precision<<8))+'u|(c.mode<<10u)'))
 inner=owner/'probe';inner.mkdir();before_root,before_file,before_args,before_scratch=parent.ROOT,parent.__file__,sys.argv,os.environ['HALO2_TEST_SCRATCH'];parent.ROOT=source;parent.__file__=str(source/'tools/validate_native_function_evaluation_extended_tu15.py');sys.argv=[parent.__file__,'--output',str(a.output)];os.environ['HALO2_TEST_SCRATCH']=str(inner)
 try:return parent.main()
 finally:
  parent.ROOT,parent.__file__,sys.argv=before_root,before_file,before_args;os.environ['HALO2_TEST_SCRATCH']=before_scratch
  path=a.output/'report.json'
  if path.exists():
   r=json.loads(path.read_text());r.update(owned_scratch=str(owner),temporary_allocated_bytes_before_cleanup=sum(p.stat().st_blocks*512 for p in owner.rglob('*')if p.is_file()),
    private_source_root=str(source),masked_precision=a.precision,private_overlay=str(a.stage),private_overlay_pins_sha256=sha(a.stage/'source-pins.json'),wrapper_sha256=sha(Path(__file__)),
    root_frozen_evaluator_unchanged=sha(ROOT/'src/core/native_tu15_function_evaluation_extended.cpp')==json.loads((a.stage/'source-pins.json').read_text())['base_hashes']['src/core/native_tu15_function_evaluation_extended.cpp'],
    scope='Private nativeCWnormalization overlay,wholeall11kinds atdeclaredPC'+str(a.precision)+'/fourrounding/DAZ/FTZ/depth0,4,5 plus504boundededges. NoRootproduction or capturedruntime claim. Source332192 precision-mask audit; unmasked/error/range/domain profiles stayexplicit.')
   save(path,r)
if __name__=='__main__':raise SystemExit(main())
