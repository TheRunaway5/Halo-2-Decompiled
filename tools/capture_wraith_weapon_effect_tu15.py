#!/usr/bin/env python3
"""Capture original charge-graph allocation and stop over an actual Wraith owner.

Read-only sandbox v4 libraries/headers retain their own checkpoint boundary.
Two corrected native modules supply the documented FISTP C1 fix. No effect
allocation, location, visibility, damage/player-response or retirement return
is supplied. The actual netgame row uses source variant descriptor78 choice2.
After60 neutral source ticks, the manual108A70 call uses the actual mortar_mp
charging reference;17A0F0 then stops its returned live instance with argument0.
The mortar has behavior1, with no idle charge route. This diagnostic does not
prove gameplay trigger production, charge-to-render composition or Xbox pixels.
"""
import argparse,hashlib,json,shutil,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);ap.add_argument('--reuse-objects',type=Path,required=True);ap.add_argument('--nonlocal',dest='nonlocal_view',action='store_true');a=ap.parse_args();out=a.output.resolve();assert not out.exists();out.mkdir(parents=True)
 pinned=json.loads((a.reuse_objects/'build.json').read_text());build=ROOT/'build-sandbox-tu15-20261014-v4';snapshot=ROOT/'.local/sandbox-tu15-source-20261014-v4'
 for path,digest in {**pinned['linked_libraries_sha256'],**pinned['override_source_files'],**pinned['override_objects']}.items():assert sha(path)==digest,path
 provenance_path=ROOT/'analysis/weapons-tu15-goal-20261013/wraith-source-provenance-v1.json';provenance=json.loads(provenance_path.read_text());assert Path(provenance['snapshot'])==snapshot
 assert sha(provenance['source_checkpoint'])==provenance['source_checkpoint_sha256']
 for path,digest in provenance['header_sha256'].items():assert sha(snapshot/path)==digest,path
 source=ROOT/'tools/capture_wraith_weapon_effect_tu15.cpp';compile=['c++','-std=c++20','-O1','-I'+str(snapshot/'include'),'-isystem','/usr/include/SDL2','-c',str(source),'-o',str(out/'capture.o')]
 link=list(pinned['link']);index=next(i for i,arg in enumerate(link) if arg.endswith('/capture.o'));link[index]=str(out/'capture.o');link[link.index('-o')+1]=str(out/'capture')
 with (out/'build.log').open('w') as log:
  subprocess.run(compile,stdout=log,stderr=subprocess.STDOUT,check=True);subprocess.run(link,cwd=build,stdout=log,stderr=subprocess.STDOUT,check=True)
 metadata=dict(scope=__doc__,source_sha256=sha(source),tool_sha256=sha(__file__),binary_sha256=sha(out/'capture'),compile=compile,link=link,prior_build_sha256=sha(a.reuse_objects/'build.json'),linked_libraries_sha256=pinned['linked_libraries_sha256'],override_source_files=pinned['override_source_files'],override_objects=pinned['override_objects'],matching_headers_provenance_sha256=sha(provenance_path))
 (out/'build.json').write_text(json.dumps(metadata,indent=2,sort_keys=True)+'\n')
 xbe=Path.home()/'.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe';assert sha(xbe)=='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
 map_=ROOT/'assets/disc/maps/coagulation.map';scope=json.loads((ROOT/'analysis/weapons-tu15-goal-20261013/authored-scope.json').read_text());expected=next(m['sha256'] for m in scope['maps'] if Path(m['path'])==map_);assert sha(map_)==expected
 command=[str(out/'capture'),str(xbe),str(map_),str(out)]+(['nonlocal'] if a.nonlocal_view else [])
 shutil.copyfile(source,out/source.name);shutil.copyfile(__file__,out/Path(__file__).name)
 with (out/'native.log').open('w') as log:result=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,timeout=120)
 metadata.update(command=command,map_sha256=expected,native_returncode=result.returncode)
 for directory in (out/'stop',out):
  if not directory.exists():continue
  phase=dict(metadata,files_sha256={p.name:sha(p) for p in directory.iterdir() if p.is_file() and p.name!='build.json'})
  if (directory/'native.json').exists():phase['native_result']=json.loads((directory/'native.json').read_text())
  if directory!=out:phase['parent_case']=str(out);phase['case_phase']='stop'
  elif (out/'stop/build.json').exists():phase['nested_case_build_sha256']={'stop':sha(out/'stop/build.json')}
  (directory/'build.json').write_text(json.dumps(phase,indent=2,sort_keys=True)+'\n')
 print(json.dumps(dict(returncode=result.returncode,native_result=phase.get('native_result'))));return result.returncode
if __name__=='__main__':raise SystemExit(main())
