#!/usr/bin/env python3
"""Verify King event/PCM consumers against a pinned, previously built native base.

This copies changed owners and tests into an immutable overlay over the complete
native King source snapshot. All changed event/asset/audio owners, the shared
queue and test translation units receive ASan/UBSan. Other linked runtime
libraries retain the base's ordinary build. No menu or transport admission.
"""
import argparse,datetime,hashlib,json,os,shlex,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
DEFAULT=ROOT/'analysis/multiplayer/game-variants/king-presentation-native-20261011'
CHANGED=('src/vista_announcer_events.cpp','src/vista_announcer_assets.cpp','src/native_vista_announcer.cpp',
 'src/source_king.cpp','include/halo2/source_king.hpp','src/native_vista_flow.cpp',
 'include/halo2/vista_announcer_events.hpp','include/halo2/native_vista_announcer.hpp',
 'tests/king_presentation_tests.cpp','tests/native_vista_announcer_tests.cpp',
 'tests/fixtures/king_presentation_original.json','tests/fixtures/king_event_producers_original.json',
 'tools/oracle_king_presentation.py','tools/oracle_king_event_producers.py',
 'tools/verify_king_presentation.py','CMakeLists.txt')
BASE_FILES=('src/vista_announcer_queue.cpp','tests/vista_announcer_events_tests.cpp',
 'tests/fixtures/vista_announcer_events_original.json','tests/fixtures/vista_announcer_publication_original.json',
 'tools/oracle_vista_announcer_events.py','tools/oracle_vista_pcr_screen.py','tools/export_vista_announcer_cues.py')
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__)
 ap.add_argument('--base',type=Path,default=ROOT/'analysis/multiplayer/game-variants/king-native-20261011/verification.json')
 ap.add_argument('--output',type=Path,default=DEFAULT/'verification.json');a=ap.parse_args()
 base=json.loads(a.base.read_text());base_source=a.base.parent/base['frozen_source'];build=base_source.parent/'build'
 assert base['commands']and all(r['exit']==0 for r in base['commands'])
 base_pins=json.loads((a.base.parent/base['source_hashes']).read_text())
 assert all(sha(base_source/name)==digest for name,digest in base_pins.items())
 assert all(sha(a.base.parent/name)==digest for name,digest in base['binaries'].items())
 run=a.output.parent/('verify-'+datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ'))
 overlay=run/'overlay';temp=run/'compiler-temp';temp.mkdir(parents=True);pins={};commands=[]
 for origin,names in ((ROOT,CHANGED),(base_source,BASE_FILES)):
  for name in names:
   target=overlay/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes((origin/name).read_bytes());pins[name]=sha(target)
 for name in ('assets','Halo 2 Vista installer','analysis'):(overlay/name).symlink_to(ROOT/name,target_is_directory=True)
 assets={}
 for name in ('Halo 2 Vista installer/halo2.exe','Halo 2 Vista installer/maps/ascension.map',
    'Halo 2 Vista installer/maps/mainmenu.map','Halo 2 Vista installer/maps/shared.map',
    'Halo 2 Vista installer/maps/single_player_shared.map',
    'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03','assets/disc/maps/lockout.map'):
  p=overlay/name
  if p.exists():assets[name]=dict(bytes=p.stat().st_size,sha256=sha(p))
 (run/'source-hashes.json').write_text(json.dumps(pins,sort_keys=True,indent=2)+'\n')
 (run/'asset-hashes.json').write_text(json.dumps(assets,sort_keys=True,indent=2)+'\n')
 env={**os.environ,'TMPDIR':str(temp),'PYTHONDONTWRITEBYTECODE':'1','SDL_AUDIODRIVER':'dummy',
      'PYTHONPATH':str(base_source/'tools'),
      'ASAN_OPTIONS':'detect_leaks=1:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'}
 def execute(label,cmd,cwd=overlay):
  log=run/(label+'.log')
  with log.open('w')as output:r=subprocess.run(list(map(str,cmd)),cwd=cwd,env=env,stdout=output,stderr=subprocess.STDOUT)
  commands.append(dict(label=label,command=list(map(str,cmd)),cwd=str(cwd),exit=r.returncode,
      log=str(log.relative_to(a.output.parent)),log_sha256=sha(log)))
  print(label,'exit',r.returncode,flush=True)
  if r.returncode:raise RuntimeError(f'{label} failed; see {log}')
 original=run/'original'
 execute('original',[sys.executable,overlay/'tools/oracle_king_presentation.py','--output',original/'report.json','--fixture',original/'fixture.json'])
 assert sha(original/'fixture.json')==sha(overlay/'tests/fixtures/king_presentation_original.json')
 report=json.loads((original/'report.json').read_text())
 assert report==json.loads((ROOT/'analysis/multiplayer/game-variants/king-presentation-20261011/report.json').read_text())
 execute('original-start',[sys.executable,overlay/'tools/oracle_king_event_producers.py','--output',original/'producer-report.json'])
 assert sha(original/'producer-report.json')==sha(overlay/'tests/fixtures/king_event_producers_original.json')
 assert json.loads((original/'producer-report.json').read_text())==json.loads((ROOT/'analysis/multiplayer/game-variants/king-presentation-20261011/producer-report.json').read_text())
 entries=json.loads((build/'compile_commands.json').read_text());objects={}
 owners=('src/vista_announcer_events.cpp','src/vista_announcer_assets.cpp','src/native_vista_announcer.cpp','src/vista_announcer_queue.cpp','src/source_king.cpp')
 tests=('tests/king_presentation_tests.cpp','tests/native_vista_announcer_tests.cpp','tests/vista_announcer_events_tests.cpp')
 for index,name in enumerate((*owners,*tests,'src/native_vista_flow.cpp')):
  template='tests/vista_announcer_events_tests.cpp'if name=='tests/king_presentation_tests.cpp'else name
  entry=next(e for e in entries if Path(e['file'])==base_source/template);cmd=shlex.split(entry['command'])
  cmd=[str(overlay/name)if token==str(base_source/template)else token for token in cmd]
  obj=run/(f'owner-{index}.o');cmd[cmd.index('-o')+1]=str(obj)
  # Avoid overwriting the base build's ordinary dependency files.
  for option in ('-MF','-MT'):
   if option in cmd:cmd[cmd.index(option)+1]=str(obj)+('.d'if option=='-MF'else'')
  cmd[1:1]=['-I'+str(overlay/'include')]
  cmd.extend(['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'])
  execute('compile-'+str(index),cmd,Path(entry['directory']));objects[name]=obj
 listing=subprocess.check_output(['ninja','-C',str(build),'-t','commands','native_vista_announcer_tests'],text=True)
 line=next(line for line in reversed(listing.splitlines())if' -o native_vista_announcer_tests 'in line)
 link=shlex.split(line)
 if link[:2]==[':','&&']:link=link[2:]
 if link[-2:]==['&&',':']:link=link[:-2]
 binaries={}
 for name in tests:
  stem=Path(name).stem;binary=run/stem;cmd=[]
  required=owners if stem=='native_vista_announcer_tests'else ('src/vista_announcer_events.cpp','src/vista_announcer_queue.cpp','src/source_king.cpp')
  if stem=='vista_announcer_events_tests':required=(*required,'src/vista_announcer_assets.cpp')
  for token in link:
   if token.startswith('-Wl,--dependency-file='):continue
   if token.endswith('native_vista_announcer_tests.cpp.o'):
    cmd.extend(str(objects[k])for k in (*required,name));continue
   cmd.append(token)
  cmd[cmd.index('-o')+1]=str(binary);cmd.extend(['-fsanitize=address,undefined','-fno-omit-frame-pointer'])
  execute('link-'+stem,cmd,build);binaries[str(binary.relative_to(a.output.parent))]=sha(binary)
  if stem=='king_presentation_tests':args=[overlay/'tests/fixtures/king_presentation_original.json',original/'producer-report.json']
  elif stem=='native_vista_announcer_tests':args=[overlay/'Halo 2 Vista installer/maps',r'scenarios\multi\ascension\ascension',
      overlay/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03',overlay/'tests/fixtures/king_presentation_original.json']
  else:args=[overlay/'tests/fixtures/vista_announcer_events_original.json',overlay/'Halo 2 Vista installer/maps/ascension.map',
      overlay/'tests/fixtures/vista_announcer_publication_original.json']
  execute('test-'+stem,[binary,*args])
 assert all(sha(overlay/name)==digest for name,digest in pins.items())
 assert all(sha(base_source/name)==digest for name,digest in base_pins.items())
 assert all(sha(a.base.parent/name)==digest for name,digest in base['binaries'].items())
 assert all((overlay/name).stat().st_size==row['bytes']and sha(overlay/name)==row['sha256']for name,row in assets.items())
 result=dict(schema='native-king-presentation-verification-v1',scope=__doc__,base=str(a.base),base_sha256=sha(a.base),
  overlay=str(overlay.relative_to(a.output.parent)),source_hashes=str((run/'source-hashes.json').relative_to(a.output.parent)),
  source_hashes_sha256=sha(run/'source-hashes.json'),asset_hashes=str((run/'asset-hashes.json').relative_to(a.output.parent)),
  asset_hashes_sha256=sha(run/'asset-hashes.json'),commands=commands,binaries=binaries,
  original_calls=report['whole_delivery_and_queue_calls'],original_start_calls=54,sanitized_owners=list(owners),
  compiled_only_owners=['src/native_vista_flow.cpp'],
  production_king_admission=False,king_transport_admission=False,goal_complete=False,
  live_overlay_drift=[name for name in CHANGED if not(ROOT/name).exists()or sha(ROOT/name)!=pins[name]])
 a.output.write_text(json.dumps(result,sort_keys=True,indent=2)+'\n');print(a.output,flush=True)
if __name__=='__main__':main()
