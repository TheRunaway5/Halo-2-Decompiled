#!/usr/bin/env python3
"""Build copied accounting CPU owners with actual storage/synchronization."""
import argparse,json,os,shutil,subprocess
from pathlib import Path
from oracle_mp_title_update_display import ROOT,sha,save,encoded
INPUTS=('src/audio/sound_capabilities.cpp','tests/sound_capabilities_tests.cpp','src/audio/native_sound_storage.cpp','src/platform/native_synchronization.cpp')
def run(out,fixture):
 assert not out.exists();out.mkdir(parents=True)
 dep=subprocess.run(['g++','-std=c++20','-Iinclude', '-Isrc','-MM',*INPUTS],cwd=ROOT,capture_output=True,text=True,check=True);save(out/'dependencies.txt',dep.stdout.encode())
 names=sorted(set(q for q in dep.stdout.replace('\\\n',' ').split()if not q.endswith(':')));pins={}
 for name in names:
  source=ROOT/name;before=sha(source);target=out/'source'/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,target);assert sha(source)==sha(target)==before;pins[name]=before
 commands=[]
 def command(argv,label,env=None):
  r=subprocess.run(argv,cwd=ROOT,env=dict(os.environ,**(env or{})),capture_output=True,text=True);save(out/(label+'.stdout'),r.stdout.encode());save(out/(label+'.stderr'),r.stderr.encode());commands.append(dict(argv=argv,cwd=str(ROOT),environment_overrides=env or{},exit_code=r.returncode,label=label));save(out/(label+'.command.json'),encoded(commands[-1]));assert r.returncode==0,(label,r.stdout[-2000:],r.stderr[-4000:]);return r
 command(['g++','--version'],'compiler-version')
 for mode,flags in(('normal',['-O2']),('sanitizer',['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'])):
  binary=out/f'sound_capabilities_tests_{mode}'
  command(['g++','-std=c++20','-pthread','-Wall','-Wextra','-Wpedantic','-Werror',*flags,'-I'+str(out/'source/include'), '-I'+str(out/'source/src'),*[str(out/'source'/n)for n in INPUTS],'-o',str(binary)],mode+'-build')
  env=dict(ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')if mode=='sanitizer'else{}
  r=command([str(binary),str(fixture)],mode+'-test',env);print(mode,r.stdout.strip(),flush=True)
 for name,h in pins.items():assert sha(ROOT/name)==sha(out/'source'/name)==h,name
 save(out/'commands.json',encoded(commands));save(out/'report.json',encoded(dict(schema='sound-capabilities-native-validation-v1',validator_sha256=sha(__file__),observed_source_snapshots=pins,fixture_sha256=sha(fixture),commands_sha256=sha(out/'commands.json'),normal_and_actual_asan_ubsan=True,scope='Fresh copied isolated integer CPU module plus actual OS raw storage and actual native recursive synchronization. Recorded source IRQL is explicit test input; no live Xbox/native thread or device/readiness/capability substitution is claimed. No shared archive/full-tree build claim.')))
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--fixtures',type=Path,required=True);a=p.parse_args();run(a.output.resolve(),a.fixtures.resolve())
