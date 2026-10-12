#!/usr/bin/env python3
"""Read-only verification of the isolated released grenade scene checkpoint."""
import argparse,hashlib,json,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
def digest(p):
 with p.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--manifest',type=Path,required=True);p.add_argument('--owned-inputs',action='store_true');p.add_argument('--output',type=Path,required=True);a=p.parse_args();r=json.loads(a.manifest.read_text());base=a.manifest.parent;bad=[]
 for row in r['artifacts']:
  path=base/row['path']
  if not path.is_file() or path.stat().st_size!=row['size'] or digest(path)!=row['sha256']:bad.append(str(path))
 for row in r['prior_pins']:
  path=Path(row['path'])
  if not path.is_file() or digest(path)!=row['sha256']:bad.append(str(path))
 if a.owned_inputs:
  for row in r['owned_inputs']:
   path=Path(row['path'])
   if not path.is_file() or path.stat().st_size!=row['size'] or digest(path)!=row['sha256']:bad.append(str(path))
 evidence=base/'evidence'
 for name,repeat in [('measured-final','measured-repeat'),('constructor','constructor-repeat'),('attach-flags','attach-flags-repeat')]:
  first=evidence/name;second=evidence/repeat
  assert {x.name:x.read_bytes() for x in first.iterdir() if x.is_file()}=={x.name:x.read_bytes() for x in second.iterdir() if x.is_file()},name
 source=json.loads((evidence/'measured-final/report.json').read_text());assert len(source['cases'])==8 and len(source['pairs'])==12
 assert sum(c['bounds_returned'] for c in source['cases'])==4
 assert all(all(b['kind']=='source-world-unregistration' for b in c['boundary']) for c in source['cases'])
 images=[Xbe(Path(i['path'])) for i in source['images']];instructions=0
 for directory in ('measured-final','measured-repeat','attach-flags','attach-flags-repeat'):
  for path in (evidence/directory).glob('*.asm'):
   image=images[0 if path.stem.endswith('1.1') else 1]
   for line in path.read_text().splitlines():
    address,raw,*_=line.split();raw=bytes.fromhex(raw);assert image.read(int(address,16),len(raw))==raw;instructions+=1
 reports=[json.loads((evidence/n/'report.json').read_text()) for n in ('maps-production-normal','maps-production-normal-repeat','maps-production-sanitizer','maps-production-sanitizer-repeat')]
 assert all(len(x['cases'])==23 and all(c['returncode']==0 and c['stderr']=='' for c in x['cases']) for x in reports)
 for report in reports[1:]:
  assert [(c['map'],c['sha256'],c['stdout']) for c in report['cases']]==[(c['map'],c['sha256'],c['stdout']) for c in reports[0]['cases']]
 assert reports[0]['binary_sha256']==digest(base/'normal/source_grenade_scene_tests')
 assert reports[2]['binary_sha256']==digest(base/'sanitizer/source_grenade_scene_tests')
 for target in ('halo2_cache','halo2_systems','halo2_renderer','source_grenade_scene_tests'):
  flags=(base/'sanitizer/build'/target/'flags.make').read_text();assert '-fsanitize=address,undefined' in flags
 assert reports[2]['environment']=={'ASAN_OPTIONS':'detect_leaks=1','UBSAN_OPTIONS':'halt_on_error=1'}
 result=dict(schema='grenade-scene-audit-v1',manifest_sha256=digest(a.manifest),artifacts=len(r['artifacts']),prior_pins=len(r['prior_pins']),owned_inputs_checked=len(r['owned_inputs']) if a.owned_inputs else 0,mismatches=bad,source_controls=8,complete_bound_returns=4,retained_world_unregister_stops=4,complete_body_pairs=16,instruction_bytes_checked=instructions,independent_original_repeats_equal=True,production_maps=23,owner_view_comparisons_per_map=20,normal_sanitizer_repeats_equal=True,scope='Released-only CPU geometry/lighting/LOD ownership and exact pinned evidence. No original held visibility, complete source registration, released angular physics or GPU/FPS claim.')
 a.output.parent.mkdir(parents=True,exist_ok=True)
 with a.output.open('x') as f:f.write(json.dumps(result,sort_keys=True,indent=2)+'\n')
 assert not bad,bad
 print(json.dumps(result,sort_keys=True))
if __name__=='__main__':main()
