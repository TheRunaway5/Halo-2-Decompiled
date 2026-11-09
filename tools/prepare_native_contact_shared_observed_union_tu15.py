#!/usr/bin/env python3
"""Merge reviewed observer plumbing with the native candidate/query DATA owners."""
import argparse,hashlib,json,re,shutil,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
ANALYSIS=ROOT/'analysis/physics'
OURS=ANALYSIS/'native-contact-producer-bound-helper-stage-v2-20261102'
THEIRS=ANALYSIS/'native-moving-shape-retained-world-stage-v2-20261102'
BASE=ANALYSIS/'native-moving-shape-process-qualified-actual1479-20261102'
CONFLICTS={'native_tu15_array_allocator.cpp':2,'native_tu15_array_growth.cpp':2,
 'native_tu15_array_reserve.cpp':2,'native_tu15_contact_cache.cpp':2,
 'native_tu15_contact_query.cpp':2,'native_tu15_contact_shape.cpp':4,
 'native_tu15_convex_contact_query.cpp':2,'native_tu15_geometry.cpp':2,
 'native_tu15_inverse_geometry.cpp':3}
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False)
 ours=json.loads((OURS/'source-pins.json').read_text());theirs=json.loads((THEIRS/'source-pins.json').read_text());base=json.loads((BASE/'source-pins.json').read_text())
 assert all(sha(OURS/'staged'/n)==v['sha256']for n,v in ours['files'].items())
 assert all(sha(THEIRS/'staged'/n)==v for n,v in theirs['source_sha256'].items())
 assert all(sha(BASE/'source'/n)==v for n,v in base['source_sha256'].items())
 stage=a.output/'staged';shutil.copytree(OURS/'staged',stage);merges=[]
 for name in ours['files']:
  ours_file=OURS/'staged'/name;theirs_file=THEIRS/'staged'/name
  if not theirs_file.is_file()or ours_file.read_bytes()==theirs_file.read_bytes():continue
  ancestor=BASE/'source'/name;assert ancestor.is_file(),name
  r=subprocess.run(['git','merge-file','-p','--diff3',str(ours_file),str(ancestor),str(theirs_file)],capture_output=True,text=True)
  assert 0<=r.returncode<128,(name,r.stderr)
  conflicts=[]
  def resolve(match):
   left,common,right=match.groups()
   # Every reviewed overlap is an include or writer-observer/preimage block.
   # Preserve the typed observer's effective target AND its domain decision;
   # unrelated retained-extent checks merge outside these overlapping lines.
   assert ('native_tu15_retained_store_observer.hpp'in right or
    ('native_tu15_effective_store_observer'in right and'native_tu15_observes_retained_store'in right)or
    'if(observing_'in right),(name,right)
   assert not any(token in left+right for token in ('asm volatile','_mm_','return native_tu15_')),(name,left,right)
   conflicts.append(dict(parent=left,ancestor=common,moving=right,resolution='typed effective observer and retained-domain filter'))
   return right
  merged=re.sub(r'^<<<<<<<[^\n]*\n(.*?)^\|\|\|\|\|\|\|[^\n]*\n(.*?)^=======\n(.*?)^>>>>>>>[^\n]*\n',resolve,r.stdout,flags=re.M|re.S)
  assert len(conflicts)==CONFLICTS.get(Path(name).name,0),(name,len(conflicts))
  assert '<<<<<<<'not in merged and'>>>>>>>'not in merged
  (stage/name).write_text(merged)
  merges.append(dict(file=name,parent_sha256=sha(ours_file),moving_sha256=sha(theirs_file),ancestor_sha256=sha(ancestor),merged_sha256=sha(stage/name),conflicts=conflicts))
 files={str(p.relative_to(stage)):{'sha256':sha(p)}for p in stage.rglob('*')if p.is_file()}
 report=dict(qualified=False,production_applied=False,files=files,units=ours['units'],preparer_sha256=sha(Path(__file__)),inputs={str(x.relative_to(ROOT))+'/source-pins.json':sha(x/'source-pins.json')for x in (OURS,THEIRS,BASE)},merges=merges,remaining=['Same immutable union original2E5320 and304350 ordered/bank/FP replay under both compilers','Mutation and same-domain/default controls','Independent union/source peer','World route/resource and visible imported-MCC multiplayer acceptance'])
 (a.output/'source-pins.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(dict(files=len(files),merges=len(merges),resolved_writer_hunks=sum(len(r['conflicts'])for r in merges),source_pins_sha256=sha(a.output/'source-pins.json')))
if __name__=='__main__':main()
