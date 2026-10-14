#!/usr/bin/env python3
"""Capture an actual source-owned pistol effect over a coherent native world.

Read-only sandbox v4 libraries/headers retain their own checkpoint boundary.
Two corrected native modules supply the documented FISTP C1 fix. No effect
allocation, location, visibility, damage/player-response or retirement return
is supplied. The standalone callback ABI after60 neutral source ticks remains
an explicit diagnostic stimulus, not a complete charge-to-render sequence.
"""
import argparse,hashlib,json,shlex,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);ap.add_argument('--reuse-objects',type=Path,required=True);ap.add_argument('--nonlocal',dest='nonlocal_view',action='store_true');a=ap.parse_args();out=a.output.resolve();assert not out.exists();out.mkdir(parents=True)
 pinned=json.loads((a.reuse_objects/'build.json').read_text());build=ROOT/'build-sandbox-tu15-20261014-v4';snapshot=ROOT/'.local/sandbox-tu15-source-20261014-v4'
 for path,digest in {**pinned['linked_libraries_sha256'],**pinned['override_source_files'],**pinned['override_objects']}.items():assert sha(path)==digest,path
 source=ROOT/'tools/capture_charged_effect_lifetime_tu15.cpp';compile=['c++','-std=c++20','-O1','-I'+str(snapshot/'include'),'-isystem','/usr/include/SDL2','-c',str(source),'-o',str(out/'capture.o')]
 link=list(pinned['link']);index=next(i for i,arg in enumerate(link) if arg.endswith('/capture.o'));link[index]=str(out/'capture.o');link[link.index('-o')+1]=str(out/'capture')
 with (out/'build.log').open('w') as log:
  subprocess.run(compile,stdout=log,stderr=subprocess.STDOUT,check=True);subprocess.run(link,cwd=build,stdout=log,stderr=subprocess.STDOUT,check=True)
 metadata=dict(scope=__doc__,source_sha256=sha(source),tool_sha256=sha(__file__),binary_sha256=sha(out/'capture'),compile=compile,link=link,prior_build_sha256=sha(a.reuse_objects/'build.json'),linked_libraries_sha256=pinned['linked_libraries_sha256'],override_source_files=pinned['override_source_files'],override_objects=pinned['override_objects'])
 (out/'build.json').write_text(json.dumps(metadata,indent=2,sort_keys=True)+'\n')
 xbe=Path.home()/'.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe';assert sha(xbe)=='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
 command=[str(out/'capture'),str(xbe),str(ROOT/'assets/disc/maps/lockout.map'),str(out)]+(['nonlocal'] if a.nonlocal_view else [])
 with (out/'native.log').open('w') as log:subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,check=True,timeout=120)
 metadata['command']=command;metadata['native_result']=json.loads((out/'native.json').read_text());metadata['files_sha256']={p.name:sha(p) for p in out.iterdir() if p.is_file() and p.name!='build.json'}
 (out/'build.json').write_text(json.dumps(metadata,indent=2,sort_keys=True)+'\n');print(json.dumps(metadata['native_result']))
if __name__=='__main__':main()
