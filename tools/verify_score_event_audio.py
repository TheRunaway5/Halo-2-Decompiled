#!/usr/bin/env python3
"""Join actual native King threshold scoring to authored native announcer audio.

Reuse the verified full HUD/native runtime build without changing its files.
An immutable overlay supplies the shared event entry point and real game-score
consumer test. Ordinary and scoped ASan/UBSan executions use actual Xbox/PC
assets and Manual PCM delivery; network and hill notices are observed services.
"""
import concurrent.futures,datetime,hashlib,json,os,re,shlex,shutil,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/multiplayer/game-variants/king-hud-native-20261012/verification.json'
BASE_PIN='8ba1f7bd52f15a766ecbadf279a7abccfaea21bd46df7146d47d2cd4ef7f0d43'
OUT=ROOT/'analysis/multiplayer/game-variants/score-event-audio-native-20261012'
OWNED=('include/halo2/native_vista_announcer.hpp','src/ui/vista/native_vista_announcer.cpp','tests/score_event_audio_tests.cpp','tools/verify_score_event_audio.py')
U5='/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 assert sha(BASE)==BASE_PIN;prior=json.loads(BASE.read_text());source=Path(prior['source']);build=source.parent/'build'
 assert all(c['exit']==0 for c in prior['commands']);assert all(sha(BASE.parent/c['log'])==c['log_sha256']for c in prior['commands'])
 run=OUT/('verify-'+datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ'));run.mkdir(parents=True);overlay=run/'overlay';overlaid={}
 for p in OWNED:
  target=overlay/p;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/p,target);overlaid[p]=sha(target)
 (run/'overlay-hashes.json').write_text(json.dumps(overlaid,sort_keys=True,indent=2)+'\n')
 (overlay/'Halo 2 Vista installer').symlink_to(ROOT/'Halo 2 Vista installer',target_is_directory=True)
 # Capture the actual root CMake declaration, rather than silently inventing
 # an asset configuration for the independently compiled bounded test.
 cmake=(ROOT/'CMakeLists.txt').read_text();declaration=re.search(r'  add_executable\(score_event_audio_tests.*?set_tests_properties\(native_score_event_audio[^\n]*\n',cmake,re.S).group()
 (run/'root-test-declaration.cmake').write_text(declaration)
 entries=json.loads((build/'compile_commands.json').read_text())
 listing=subprocess.check_output(['ninja','-C',str(build),'-t','commands','king_recording_runtime_tests'],text=True)
 link=shlex.split(next(line for line in reversed(listing.splitlines())if ' -o king_recording_runtime_tests 'in line))
 if link[:2]==[':','&&']:link=link[2:]
 if link[-2:]==['&&',':']:link=link[:-2]
 artifacts={str((BASE.parent/p).resolve()):h for p,h in prior['binaries'].items()}
 for token in link:
  p=Path(token)if Path(token).is_absolute()else build/token
  if p.is_file()and token.endswith(('.a','.so','.o')):artifacts[str(p.resolve())]=sha(p)
 (run/'base-artifact-hashes.json').write_text(json.dumps(artifacts,sort_keys=True,indent=2)+'\n')
 inputs={str(BASE):BASE_PIN,str(source.parent/'source-hashes.json'):prior['source_hashes_sha256']}
 names=('assets/disc/maps/lockout.map','assets/disc/maps/shared.map','assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03',
  'tests/fixtures/king_presentation_original.json','tests/fixtures/vista_announcer_events_original.json',
  'src/gameplay/multiplayer/source_score_event.cpp','include/halo2/source_score_event.hpp','src/audio/vista_announcer_events.cpp','src/audio/vista_announcer_queue.cpp',U5)
 for name in names:inputs[str(source/name)]=sha(source/name)
 for name in ('ascension.map','shared.map','mainmenu.map'):
  p=overlay/'Halo 2 Vista installer/maps'/name;inputs[str(p)]=sha(p)
 (run/'input-hashes.json').write_text(json.dumps(inputs,sort_keys=True,indent=2)+'\n')
 commands=[];env={**os.environ,'SDL_AUDIODRIVER':'dummy','PYTHONDONTWRITEBYTECODE':'1'}
 def execute(label,cmd,cwd=source,extra=None):
  cmd=list(map(str,cmd));log=run/(label+'.log')
  with log.open('w')as out:r=subprocess.run(cmd,cwd=cwd,env={**env,**(extra or {})},stdout=out,stderr=subprocess.STDOUT)
  commands.append(dict(label=label,command=cmd,cwd=str(cwd),exit=r.returncode,log=str(log.relative_to(OUT)),log_sha256=sha(log)))
  (run/'commands.json').write_text(json.dumps(commands,sort_keys=True,indent=2)+'\n');print(label,'exit',r.returncode,flush=True);assert r.returncode==0,log
 def compile_owner(name,label,sanitized,template=None):
  row=next(e for e in entries if Path(e['file'])==source/(template or name));cmd=shlex.split(row['command']);obj=run/(label+'.o')
  actual=overlay/name if name in overlaid else source/name
  cmd=[str(actual)if token==row['file']else token for token in cmd]
  cmd[cmd.index('-o')+1]=str(obj);cmd.insert(1,'-I'+str(overlay/'include'), '-I'+str(overlay/'src'))
  for option in ('-MF','-MT'):
   if option in cmd:cmd[cmd.index(option)+1]=str(obj)+('.d'if option=='-MF'else'')
  if sanitized:cmd.extend(['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'])
  execute(label,cmd,Path(row['directory']));return obj
 def link_test(objects,label,sanitized):
  cmd=[]
  for token in link:
   if token.startswith('-Wl,--dependency-file='):continue
   if token.endswith('king_recording_runtime_tests.cpp.o'):cmd.extend(map(str,objects))
   else:cmd.append(token)
  binary=run/label;cmd[cmd.index('-o')+1]=str(binary)
  if sanitized:cmd.extend(['-fsanitize=address,undefined','-fno-omit-frame-pointer'])
  execute('link-'+label,cmd,build);return binary
 args=(overlay/'Halo 2 Vista installer/maps',r'scenarios\multi\ascension\ascension',source/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03',
  source/'assets/disc/maps/lockout.map',source/'assets/disc/maps',U5)
 production=compile_owner('src/ui/vista/native_vista_announcer.cpp','compile-announcer',False)
 test=compile_owner('tests/score_event_audio_tests.cpp','compile-score-audio-test',False,'tests/king_recording_runtime_tests.cpp')
 binary=link_test([production,test],'score_event_audio_tests',False);execute('score-audio-test',[binary,*args])
 regression=compile_owner('tests/native_vista_announcer_tests.cpp','compile-announcer-test',False,'tests/king_recording_runtime_tests.cpp')
 reg_binary=link_test([production,regression],'native_vista_announcer_tests',False)
 execute('announcer-regression',[reg_binary,*args[:3],source/'tests/fixtures/king_presentation_original.json'])
 execute('score-producer-regression',[build/'score_event_tests',source/'analysis/multiplayer/game-variants/score-events-20261012/complete/report.json'])
 owners=('src/gameplay/multiplayer/source_score_event.cpp','src/ui/vista/native_vista_announcer.cpp','src/audio/vista_announcer_events.cpp','src/audio/vista_announcer_queue.cpp','tests/score_event_audio_tests.cpp')
 objects=[compile_owner(p,'sanitizer-compile-'+str(i),True,'tests/king_recording_runtime_tests.cpp'if p.startswith('tests/')else None)for i,p in enumerate(owners)]
 sanitized=link_test(objects,'score_event_audio_tests_asan',True)
 execute('sanitizer-test',[sanitized,*args],extra={'ASAN_OPTIONS':'detect_leaks=1:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'})
 assert all(sha(p)==h for p,h in inputs.items())and all(sha(p)==h for p,h in artifacts.items())
 assert all(sha(overlay/p)==h and sha(ROOT/p)==h for p,h in overlaid.items())
 pins=json.loads((source.parent/'source-hashes.json').read_text())
 with concurrent.futures.ThreadPoolExecutor(max_workers=8)as pool:current=list(pool.map(lambda p:sha(source/p),pins))
 assert all(pins[p]==v for p,v in zip(pins,current))
 result=dict(schema='score-event-audio-native-v1',scope=__doc__,base_manifest=str(BASE),base_manifest_sha256=BASE_PIN,
  source=str(source),overlay=str(overlay),overlay_hashes_sha256=sha(run/'overlay-hashes.json'),
  input_hashes_sha256=sha(run/'input-hashes.json'),base_artifact_hashes_sha256=sha(run/'base-artifact-hashes.json'),
  root_test_declaration_sha256=sha(run/'root-test-declaration.cmake'),commands=commands,
  binaries={str(p.relative_to(OUT)):sha(p)for p in(binary,reg_binary,sanitized)},scoped_sanitized_owners=list(owners),ordinary_linked_runtime=True,
  actual_king_scoring_audio=True,native_event_transport=False,caption_delivery=False,normal_king_admission=False,complete_match_flows=False,goal_complete=False)
 (run/'verification.json').write_text(json.dumps(result,sort_keys=True,indent=2)+'\n');shutil.copy2(run/'verification.json',OUT/'verification.json');print(OUT/'verification.json',flush=True)
if __name__=='__main__':main()
