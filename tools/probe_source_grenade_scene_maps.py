#!/usr/bin/env python3
"""CPU-only23-map grenade residency/owner/draw comparison; no GPU or angular claim."""
import argparse,hashlib,json,os,subprocess
from pathlib import Path
from inspect_multiplayer import ROOT,MAPS
from inventory_mp_weapon_effects import DLC

def digest(p):
 with p.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--binary',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--fixtures',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 bank=Path('/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content');image=bank/'u5English/default.xbe'
 maps=[ROOT/'assets/disc/maps'/(n+'.map') for n in MAPS]+[ROOT/'assets/multiplayer-map-pack/demos/dlc'/b/(n+'.map') for b,ns in DLC.items() for n in ns]+[bank/'c/4d53006400000004'/(n+'.map') for n in ('derelict','highplains2')]
 cases=[]
 for m in maps:
  cmd=[str(a.binary.resolve()),str(m),str(ROOT/'assets/disc/maps'),str(image),str(a.fixtures.resolve())]
  run=subprocess.run(cmd,capture_output=True,text=True);(a.output/(m.stem+'.log')).write_text(run.stdout+run.stderr)
  cases.append(dict(map=str(m),sha256=digest(m),command=cmd,returncode=run.returncode,stdout=run.stdout,stderr=run.stderr));print(m.stem,run.returncode,flush=True)
 report=dict(schema='source-grenade-scene-maps-v1',scope=__doc__,binary_sha256=digest(a.binary),image_sha256=digest(image),fixture_sha256=digest(a.fixtures),tool_sha256=digest(Path(__file__)),environment={n:os.environ.get(n) for n in ('ASAN_OPTIONS','UBSAN_OPTIONS')},cases=cases)
 (a.output/'report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');assert len(cases)==23 and all(c['returncode']==0 for c in cases)
if __name__=='__main__':main()
