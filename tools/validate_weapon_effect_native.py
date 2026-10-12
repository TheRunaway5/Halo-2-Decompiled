#!/usr/bin/env python3
"""Build isolated native effect consumers against unchanged source fixtures."""
import argparse,json,os,subprocess
from pathlib import Path
from oracle_smg_effect_owner_1_1 import ROOT,sha,save,encoded
FILES=('include/halo2/weapon_effect.hpp','src/weapon_effect.cpp','tests/weapon_effect_tests.cpp','tools/export_smg_effect_fixtures.py','tools/research_weapon_effect_u5.py')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
 fixture=ROOT/'analysis/multiplayer/integration/inventory-source/smg-effect-owner/native-fixtures/fixtures.txt';results=[]
 for name,flags in(('normal',['-O2']),('sanitized',['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'])):
  exe=out/name;cmd=['c++','-std=c++20','-Wall','-Wextra','-Werror',*flags,'-I'+str(ROOT/'include'),str(ROOT/'src/weapon_effect.cpp'),str(ROOT/'tests/weapon_effect_tests.cpp'),'-o',str(exe)]
  r=subprocess.run(cmd,capture_output=True,text=True);assert r.returncode==0,r.stdout+r.stderr
  v=subprocess.run([str(exe),str(fixture)],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1'));assert v.returncode==0,v.stdout+v.stderr
  results.append(dict(build=name,command=cmd,binary_sha256=sha(exe),output=v.stdout,stderr=v.stderr,exit_code=v.returncode));save(out/(name+'.log'),(r.stdout+r.stderr+v.stdout+v.stderr).encode())
 report=dict(schema='source-weapon-effect-native-validation-v1',validator_sha256=sha(__file__),files={f:sha(ROOT/f)for f in FILES},fixture_sha256=sha(fixture),results=results,scope='Exact original60 request/direction buffers and whole190 visibility state, with measured creation/retirement lower-service callbacks. No native generic allocation/event/particle/flash rendering claim.')
 save(out/'report.json',encoded(report));print(json.dumps(dict(path=str(out/'report.json'),sha256=sha(out/'report.json'),results=[v['output'].strip()for v in results])))
if __name__=='__main__':main()
