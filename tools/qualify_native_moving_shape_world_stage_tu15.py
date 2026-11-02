#!/usr/bin/env python3
"""Owned exact1480 reconstruction delegates both staged World CMake models."""
import argparse,hashlib,json,os,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);base=Path(os.environ['HALO2_FROZEN_SOURCE']);manifest=json.loads((a.stage/'source-pins.json').read_text());r={'passed':False,'owned_scratch':str(scratch),'source_root':str(base),'stage_source_sha256':manifest['source_sha256'],'results':[],'retained_large_artifacts':[],'scope':'Exact final1480 plus narrow moving stage default/positive factory/merge World source models; no whole original/live assertion.'}
 try:
  for name,pin in manifest['base_hashes'].items():
   q=base/name;assert (hashlib.sha256(q.read_bytes()).hexdigest()if q.exists()else None)==pin,name
  for cc in ('g++','clang++'):
   out=a.output/cc;command=[sys.executable,str(ROOT/'tools/run_test.py'),'--scratch-root',str(scratch/'models'),'--',sys.executable,str(ROOT/'tools/validate_native_moving_shape_world_coherent_tu15.py'),'--stage',str(a.stage),'--source-root',str(base),'--compiler',cc,'--output',str(out)];execution=subprocess.run(command);r['results'].append({'compiler':cc,'exit_code':execution.returncode,'command':command});assert execution.returncode==0,cc+' World model failed'
  r['passed']=True
 except BaseException as e:r['error']=str(e)or repr(e);raise
 finally:
  r['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());(a.output/'report.json').write_text(json.dumps(r,indent=2)+'\n')
 return 0 if r['passed']else 1
if __name__=='__main__':raise SystemExit(main())
