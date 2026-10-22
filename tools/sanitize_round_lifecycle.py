#!/usr/bin/env python3
"""Instrument changed round/match owners and their direct shared state writers.

Uses the configured Ninja compiler/include/link arguments. Other static/system
libraries remain ordinary builds; this is scoped ASan/UBSan, not whole-engine
instrumentation. Run after the ordinary build has completed.
"""
import argparse,json,os,shlex,subprocess
from pathlib import Path
from oracle_round_lifecycle import ROOT,sha

def no_active_build(build):
 for p in Path('/proc').iterdir():
  if not p.name.isdigit():continue
  try:
   argv=(p/'cmdline').read_bytes().split(b'\0')
   if argv and Path(os.fsdecode(argv[0])).name in ('ninja','make'):
    assert (p/'cwd').resolve()!=build,'Wait for the active shared build before reading archives'
  except (FileNotFoundError,PermissionError,ProcessLookupError):pass

def main():
 ap=argparse.ArgumentParser(description=__doc__)
 ap.add_argument('--build',type=Path,default=ROOT/'build')
 ap.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer/game-variants/round-lifecycle-native/sanitizers')
 a=ap.parse_args();build=a.build.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
 no_active_build(build)
 targets=('round_lifecycle_tests','game_end_native_tests')
 lines=[]
 for target in targets:lines.extend(subprocess.check_output(['ninja','-C',str(build),'-t','commands',target],text=True).splitlines())
 common=['src/gameplay/multiplayer/source_round_lifecycle.cpp','src/gameplay/multiplayer/source_game_end.cpp','src/gameplay/multiplayer/source_result_statistics.cpp',
  'src/gameplay/units/source_player_lives.cpp','src/gameplay/weapons/source_netgame_weapon.cpp','src/gameplay/multiplayer/source_game_over.cpp','src/audio/sound_classes.cpp']
 sources=common+['src/gameplay/multiplayer/multiplayer.cpp','tests/round_lifecycle_tests.cpp','tests/game_end_native_tests.cpp']
 objects={};commands=[]
 flags=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-O1']
 with (out/'build.log').open('w')as log:
  for source in sources:
   command=next(shlex.split(line)for line in lines if ' -c '+str(ROOT/source)in line)
   filtered=[];n=0
   while n<len(command):
    word=command[n]
    if word in ('-MT','-MF','-o'):n+=2;continue
    if word=='-MD' or word.startswith('-O'):n+=1;continue
    filtered.append(word);n+=1
   obj=out/(Path(source).stem+'.o');objects[source]=str(obj)
   command=filtered+flags+['-o',str(obj)];commands.append(command)
   subprocess.run(command,cwd=build,stdout=log,stderr=subprocess.STDOUT,check=True)
  for target in targets:
   line=next(line for line in reversed(lines)if ' -o '+target+' 'in line)
   command=shlex.split(next(part for part in line.split(' && ')if ' -o '+target+' 'in part))
   test='tests/'+target+'.cpp';instrumented=common+(['src/gameplay/multiplayer/multiplayer.cpp']if target=='game_end_native_tests'else[])+[test]
   filtered=[];n=0
   while n<len(command):
    word=command[n]
    if word=='-o':filtered+=['-o',str(out/target)];n+=2;continue
    if word.startswith('-Wl,--dependency-file='):n+=1;continue
    if word.endswith('.cpp.o'):
     filtered.extend(objects[source]for source in instrumented);n+=1;continue
    filtered.append(word);n+=1
   command=filtered+flags;commands.append(command);no_active_build(build)
   subprocess.run(command,cwd=build,stdout=log,stderr=subprocess.STDOUT,check=True)
 no_active_build(build);results=[]
 env=dict(os.environ,ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
 args={targets[0]:[ROOT/'tests/fixtures/round_lifecycle_original.json',ROOT/'tests/fixtures/round_lifecycle_cap_original.json'],
  targets[1]:[ROOT/'assets/disc/maps/lockout.map',ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03']}
 for target in targets:
  command=[str(out/target)]+[str(p)for p in args[target]]
  result=subprocess.run(command,cwd=ROOT,env=env,text=True,capture_output=True)
  (out/(target+'.log')).write_text(result.stdout+result.stderr)
  results.append(dict(command=command,returncode=result.returncode,stdout=result.stdout,stderr=result.stderr,
   binary_sha256=sha(out/target)))
 report=dict(scope=__doc__,instrumented_sources={p:sha(ROOT/p)for p in sources},commands=commands,results=results,
  ordinary_libraries={p.name:sha(p)for p in build.glob('libhalo2*.a')},tool_sha256=sha(__file__))
 (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
 assert all(r['returncode']==0 for r in results),'Scoped sanitizer run failed; see retained logs'
 print(json.dumps(dict(scoped_asan_ubsan_pass=True,tests=len(results),instrumented_translation_units=len(sources))))
if __name__=='__main__':main()
