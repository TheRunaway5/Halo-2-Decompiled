#!/usr/bin/env python3
"""Both compiler contexts for private writer/domain union; no semantic claim."""
from pathlib import Path
import hashlib,json,os,subprocess
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 stage=ROOT/'analysis/physics/native-moving-shape-writer-observation-stage-20261102';interface=ROOT/'analysis/physics/native-simplex-retained-observation-interface-qualified-20261102';base=ROOT/'analysis/physics/native-moving-shape-process-qualified-actual1479-20261102/source';out=ROOT/'analysis/physics/native-moving-shape-writer-observation-context-20261102-first';out.mkdir(exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);pins=json.loads((stage/'source-pins.json').read_text());r={'passed':False,'owned_scratch':str(scratch),'source_sha256':pins['source_sha256'],'interface_manifest_sha256':sha(interface/'source-pins.json'),'results':[],'retained_large_artifacts':[]}
 try:
  for name,pin in pins['source_sha256'].items():assert sha(stage/'staged'/name)==pin
  for cc in ('g++','clang++'):
   command=[cc,'-std=c++20','-fsyntax-only','-I'+str(interface/'source/include'),'-I'+str(base/'include'),'-I'+str(interface/'source/src/physics'),'-I'+str(base/'src/physics'),*[str(stage/'staged'/n)for n in pins['source_sha256']]]
   run=subprocess.run(command,capture_output=True,text=True);(out/(cc+'.log')).write_text(run.stdout+run.stderr);r['results'].append({'compiler':cc,'exit_code':run.returncode});assert run.returncode==0,run.stderr[-6000:]
  r['passed']=all(sha(stage/'staged'/n)==v for n,v in pins['source_sha256'].items())
 finally:r['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());(out/'report.json').write_text(json.dumps(r,indent=2)+'\n')
 return 0 if r['passed']else 1
if __name__=='__main__':raise SystemExit(main())
