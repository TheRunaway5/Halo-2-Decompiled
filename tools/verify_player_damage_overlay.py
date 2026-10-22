#!/usr/bin/env python3
"""Verify final player-damage owners over a pinned complete native build."""
import argparse,datetime,hashlib,json,os,shlex,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/multiplayer/game-variants/king-damage-native-20261011'
DEFAULT_BASE=OUT/'verify-20261011T213523.415365Z'
OWNERS=('src/gameplay/units/source_player_damage.cpp','src/gameplay/units/damage_amount.cpp','src/gameplay/multiplayer/source_king.cpp','src/gameplay/multiplayer/multiplayer.cpp','src/gameplay/simulation.cpp')
HEADERS=('include/halo2/source_player_damage.hpp','include/halo2/damage_amount.hpp','include/halo2/source_king.hpp','include/halo2/multiplayer.hpp','include/halo2/simulation.hpp')
TARGETS=('player_damage_tests','king_runtime_tests','multiplayer_tests','prop_sandbox_tests','multiplayer_grenade_delivery_tests')
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--base-run',type=Path,default=DEFAULT_BASE)
 ap.add_argument('--output',type=Path,default=OUT/'verification.json');a=ap.parse_args()
 base=a.base_run.resolve();source=base/'frozen-source';build=base/'build'
 base_pins=json.loads((base/'source-hashes.json').read_text());assets=json.loads((base/'asset-hashes.json').read_text())
 assert all(sha(source/name)==digest for name,digest in base_pins.items())
 assert all(sha(ROOT/name)==base_pins[name]for name in HEADERS),'native header ABI differs from the complete base build'
 assert all((source/name).stat().st_size==row['bytes']and sha(source/name)==row['sha256']for name,row in assets.items())
 run=a.output.parent/('overlay-'+datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ'))
 overlay=run/'source';temp=run/'compiler-temp';temp.mkdir(parents=True);pins={};commands=[]
 for name in (*OWNERS,*HEADERS,'tests/king_runtime_tests.cpp','tests/player_damage_tests.cpp',
     'tests/fixtures/king_trait_consumers_original.json','tools/verify_player_damage_overlay.py'):
  target=overlay/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes((ROOT/name).read_bytes());pins[name]=sha(target)
 for name in ('assets','analysis'):(overlay/name).symlink_to(ROOT/name,target_is_directory=True)
 base_files={str(p.relative_to(base)):sha(p)for p in (*(build.glob('*.a')),*(build/t for t in TARGETS),
     base/'build.log',base/'ctest.log',base/'source-hashes.json',base/'asset-hashes.json',build/'compile_commands.json',build/'build.ninja')}
 (run/'base-files.json').write_text(json.dumps(base_files,sort_keys=True,indent=2)+'\n')
 (run/'source-hashes.json').write_text(json.dumps(pins,sort_keys=True,indent=2)+'\n')
 env={**os.environ,'TMPDIR':str(temp),'PYTHONDONTWRITEBYTECODE':'1','SDL_AUDIODRIVER':'dummy',
     'ASAN_OPTIONS':'detect_leaks=1:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'}
 def execute(label,cmd,cwd=build):
  log=run/(label+'.log')
  with log.open('w')as output:r=subprocess.run(list(map(str,cmd)),cwd=cwd,env=env,stdout=output,stderr=subprocess.STDOUT)
  commands.append(dict(label=label,command=list(map(str,cmd)),cwd=str(cwd),exit=r.returncode,
      log=str(log.relative_to(a.output.parent)),log_sha256=sha(log)))
  print(label,'exit',r.returncode,flush=True)
  if r.returncode:raise RuntimeError(f'{label} failed; see {log}')
 entries=json.loads((build/'compile_commands.json').read_text());objects={};binaries={}
 def compile_one(name,sanitized):
  entry=next(e for e in entries if Path(e['file'])==source/name);cmd=shlex.split(entry['command'])
  target=run/(name.replace('/','-')+('-asan'if sanitized else '')+'.o');cmd[cmd.index('-o')+1]=str(target)
  cmd=[str(overlay/name)if token==str(source/name)else token for token in cmd]
  cmd.insert(1,'-I'+str(overlay/'include'), '-I'+str(overlay/'src'))
  if sanitized:cmd.extend(['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'])
  execute(('asan-'if sanitized else 'normal-')+name.replace('/','-'),cmd);return target
 def link_target(target,sanitized):
  listing=subprocess.check_output(['ninja','-C',str(build),'-t','commands',target],text=True)
  line=next(s for s in reversed(listing.splitlines())if f' -o {target} 'in s);link=shlex.split(line)
  if link[:2]==[':', '&&']:link=link[2:]
  if link[-2:]==['&&', ':']:link=link[:-2]
  test='tests/'+target+'.cpp';cmd=[]
  for token in link:
   if token.startswith('-Wl,--dependency-file='):continue
   if token.endswith(target+'.cpp.o'):
    cmd.append(str(objects.get((test,sanitized),build/token)));cmd.extend(str(objects[name,sanitized])for name in OWNERS)
   else:cmd.append(token)
  binary=run/(target+('_asan'if sanitized else ''));cmd[cmd.index('-o')+1]=str(binary)
  if sanitized:cmd.extend(['-fsanitize=address,undefined','-fno-omit-frame-pointer'])
  execute(('asan-'if sanitized else 'normal-')+'link-'+target,cmd);binaries[str(binary.relative_to(a.output.parent))]=sha(binary);return binary
 for name in OWNERS:objects[name,False]=compile_one(name,False)
 objects['tests/king_runtime_tests.cpp',False]=compile_one('tests/king_runtime_tests.cpp',False)
 # The original kernel target links its source owners directly, so its base
 # executable is reused unchanged and pinned; native targets replace the actual
 # owning archive members with the freshly compiled, ABI-compatible objects.
 normal={target:link_target(target,False)for target in TARGETS if target!='player_damage_tests'}
 xbe=source/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03';maps=source/'assets/disc/maps'
 execute('original-kernel',[build/'player_damage_tests',source/'tests/fixtures/king_trait_consumers_original.json'])
 execute('native-king',[normal['king_runtime_tests'],xbe]);execute('native-slayer',[normal['multiplayer_tests']])
 execute('native-lan-startup',[normal['multiplayer_tests'],'--startup-delay'])
 execute('retail-multiplayer',[normal['multiplayer_tests'],maps])
 # Use exact configured real-map inputs, without inventing test arguments.
 ctests=json.loads(subprocess.check_output(['ctest','--test-dir',str(build),'--show-only=json-v1'],text=True))
 for test_name,target in (('live_prop_sandbox','prop_sandbox_tests'),('multiplayer_grenade_delivery','multiplayer_grenade_delivery_tests')):
  cmd=next(t['command']for t in ctests['tests']if t['name']==test_name);cmd[0]=str(normal[target]);execute(test_name,cmd)
 for name in (*OWNERS,'tests/king_runtime_tests.cpp'):objects[name,True]=compile_one(name,True)
 execute('native-king-asan',[link_target('king_runtime_tests',True),xbe])
 kernel=run/'player_damage_tests_asan'
 execute('kernel-asan-build',['c++','-std=c++20','-O1','-g','-fno-fast-math','-ffp-contract=off','-fsanitize=address,undefined','-fno-omit-frame-pointer','-I'+str(overlay/'include'), '-I'+str(overlay/'src'),'-I'+str(source/'include'), '-I'+str(source/'src'),
     overlay/'tests/player_damage_tests.cpp',*(overlay/name for name in ('src/gameplay/units/source_player_damage.cpp','src/gameplay/units/damage_amount.cpp','src/gameplay/multiplayer/source_king.cpp')),'-o',kernel])
 execute('kernel-asan-test',[kernel,overlay/'tests/fixtures/king_trait_consumers_original.json']);binaries[str(kernel.relative_to(a.output.parent))]=sha(kernel)
 assert all(sha(base/name)==digest for name,digest in base_files.items())
 assert all(sha(source/name)==digest for name,digest in base_pins.items())
 assert all(sha(overlay/name)==digest for name,digest in pins.items())
 assert all((source/name).stat().st_size==row['bytes']and sha(source/name)==row['sha256']for name,row in assets.items())
 original=base/'original';assert sha(original/'fixture.json')==pins['tests/fixtures/king_trait_consumers_original.json']
 result=dict(schema='native-player-damage-overlay-verification-v1',scope=__doc__,base=str(base),
     base_files=str((run/'base-files.json').relative_to(a.output.parent)),base_files_sha256=sha(run/'base-files.json'),
     initial_base_test_limit='King synthetic pickup fixture had NONE model/animation references; six other configured tests passed. Final corrected native tests are executed above.',
     overlay=str(overlay.relative_to(a.output.parent)),source_hashes=str((run/'source-hashes.json').relative_to(a.output.parent)),
     source_hashes_sha256=sha(run/'source-hashes.json'),commands=commands,binaries=binaries,sanitized_owners=list(OWNERS),
     original_fixture_sha256=sha(original/'fixture.json'),original_report_sha256=sha(original/'report.json'),original_damage_calls=5004,
     production_king_admission=False,king_transport_admission=False,goal_complete=False,
     live_owned_drift=[name for name,digest in pins.items()if not(ROOT/name).exists()or sha(ROOT/name)!=digest],
     base_live_checkout_drift=[name for name,digest in base_pins.items()if not(ROOT/name).exists()or sha(ROOT/name)!=digest])
 a.output.write_text(json.dumps(result,sort_keys=True,indent=2)+'\n');print(a.output,flush=True)
if __name__=='__main__':main()
