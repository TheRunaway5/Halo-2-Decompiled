#!/usr/bin/env python3
"""Verify native camouflage snapshots over an immutable, complete native build.

Every compiled translation unit depending on a changed header is rebuilt.
This is a coherent header/ABI overlay, not a few replacement source objects.
Unrelated concurrent checkout changes are excluded and recorded separately.
"""
import argparse,concurrent.futures,datetime,hashlib,json,os,shlex,shutil,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/multiplayer/game-variants/camouflage-native-20261011'
BASE=OUT/'verify-20261011T221616.197051Z'
OWNED=('include/halo2/multiplayer.hpp','include/halo2/multiplayer_datagrams.hpp',
       'src/multiplayer.cpp','src/multiplayer_network.cpp','tests/camouflage_runtime_tests.cpp')
TARGETS=('camouflage_tests','camouflage_runtime_tests','player_damage_tests','king_runtime_tests',
         'multiplayer_tests','prop_sandbox_tests','multiplayer_grenade_delivery_tests')
CHECKS=('original_camouflage','native_camouflage_runtime','original_player_damage','native_king_runtime',
        'native_multiplayer','native_lan_startup','retail_multiplayer','live_prop_sandbox','multiplayer_grenade_delivery')
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--base-run',type=Path,default=BASE)
 ap.add_argument('--output',type=Path,default=OUT/'snapshot-verification.json');a=ap.parse_args()
 base=a.base_run.resolve();source=base/'frozen-source';build=base/'build'
 pins=json.loads((base/'source-hashes.json').read_text());assets=json.loads((base/'asset-hashes.json').read_text())
 assert all(sha(source/n)==digest for n,digest in pins.items())
 assert all(sha(source/n)==v['sha256']for n,v in assets.items())
 run=a.output.parent/('snapshots-'+datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ'))
 overlay=run/'source';temp=run/'compiler-temp';temp.mkdir(parents=True);commands=[];overlay_pins={}
 for name in (*OWNED,'tools/verify_camouflage_snapshots.py'):
  p=overlay/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes((ROOT/name).read_bytes());overlay_pins[name]=sha(p)
 (run/'source-hashes.json').write_text(json.dumps(overlay_pins,sort_keys=True,indent=2)+'\n')
 base_files={str(p.relative_to(base)):sha(p)for p in (*(build.glob('*.a')),*(build/t for t in TARGETS),
     build/'compile_commands.json',build/'build.ninja',build/'.ninja_deps',build/'CTestTestfile.cmake',
     base/'source-hashes.json',base/'asset-hashes.json')}
 (run/'base-hashes.json').write_text(json.dumps(base_files,sort_keys=True,indent=2)+'\n')
 env={**os.environ,'TMPDIR':str(temp),'SDL_AUDIODRIVER':'dummy',
      'ASAN_OPTIONS':'detect_leaks=1:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'}
 def execute(label,cmd,cwd=build):
  log=run/(label+'.log')
  with log.open('w')as out:r=subprocess.run(list(map(str,cmd)),cwd=cwd,env=env,stdout=out,stderr=subprocess.STDOUT)
  commands.append(dict(label=label,command=list(map(str,cmd)),cwd=str(cwd),exit=r.returncode,
      log=str(log.relative_to(a.output.parent)),log_sha256=sha(log)))
  print(label,'exit',r.returncode,flush=True)
  if r.returncode:raise RuntimeError(f'{label} failed; see {log}')
 raw=subprocess.check_output(['ninja','-C',str(build),'-t','deps'],text=True)
 (run/'base-dependencies.txt').write_text(raw);dependencies={};current=None
 for line in raw.splitlines():
  if ': #deps 'in line:
   current=line.split(': #deps ',1)[0];dependencies[current]=set()
   assert '(VALID)'in line,'Stale dependency record: '+line
  elif line.startswith('    ')and current:dependencies[current].add(line.strip())
 changed={str(source/n)for n in OWNED};entries=json.loads((build/'compile_commands.json').read_text())
 selected={}
 for entry in entries:
  obj=str(Path(entry['output']).relative_to(build))
  if not(build/obj).exists():continue
  assert obj in dependencies,'Compiled object lacks a dependency record: '+obj
  if dependencies[obj]&changed:selected[obj]=entry
 assert all(any(str(source/n)in dependencies[o]for o in selected)for n in OWNED),'An owned file has no rebuilt consumer'
 (run/'recompiled-dependencies.json').write_text(json.dumps({o:sorted(dependencies[o]&changed)for o in selected},sort_keys=True,indent=2)+'\n')
 def compile_one(obj,entry,sanitized=False):
  cmd=shlex.split(entry['command']);result=run/('asan-objects'if sanitized else 'objects')/obj
  result.parent.mkdir(parents=True,exist_ok=True);cmd[cmd.index('-o')+1]=str(result)
  name=str(Path(entry['file']).relative_to(source))
  if name in OWNED:cmd=[str(overlay/name)if t==str(source/name)else t for t in cmd]
  cmd.insert(1,'-I'+str(overlay/'include'));dep=result.with_suffix('.d');cmd.extend(['-MMD','-MF',str(dep)])
  if sanitized:cmd.extend(['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'])
  execute(('asan-'if sanitized else 'normal-')+obj.replace('/','-'),cmd)
  # All changed project headers must resolve to the overlay. A relative or
  # absolute include bypassing that search path would leave the ABI mixed.
  text=dep.read_text()
  assert all(str(source/n)not in text for n in OWNED if n.startswith('include/')),'Changed header bypassed overlay'
  return obj,result
 objects={}
 with concurrent.futures.ThreadPoolExecutor(max_workers=4)as pool:
  futures=[pool.submit(compile_one,o,e)for o,e in selected.items()]
  for future in concurrent.futures.as_completed(futures):o,p=future.result();objects[o]=p
 archives={}
 for obj,p in objects.items():
  target=Path(obj).parts[1].removesuffix('.dir');archive=build/('lib'+target+'.a')
  if archive.exists():archives.setdefault(archive,[]).append(p)
 replaced_archives={}
 for archive,parts in archives.items():
  target=run/archive.name;shutil.copyfile(archive,target)
  execute('archive-'+archive.stem,['ar','r',target,*sorted(parts)]);replaced_archives[archive.name]=target
 ctests=json.loads(subprocess.check_output(['ctest','--test-dir',str(build),'--show-only=json-v1'],text=True))
 binaries={}
 def link_target(target,asan_objects=None):
  listing=subprocess.check_output(['ninja','-C',str(build),'-t','commands',target],text=True)
  line=next(s for s in reversed(listing.splitlines())if f' -o {target} 'in s);cmd=shlex.split(line)
  if cmd[:2]==[':', '&&']:cmd=cmd[2:]
  if cmd[-2:]==['&&', ':']:cmd=cmd[:-2]
  mapped=[]
  for token in cmd:
   if token.startswith('-Wl,--dependency-file='):continue
   if token in objects:mapped.append(str((asan_objects or {}).get(token,objects[token])))
   elif token in replaced_archives:
    if asan_objects and token=='libhalo2_systems.a':
     mapped.extend(str(p)for o,p in asan_objects.items()if o.startswith('CMakeFiles/halo2_systems.dir/'))
    mapped.append(str(replaced_archives[token]))
   else:mapped.append(token)
  binary=run/(target+('_asan'if asan_objects else ''));mapped[mapped.index('-o')+1]=str(binary)
  if asan_objects:mapped.extend(['-fsanitize=address,undefined','-fno-omit-frame-pointer'])
  execute(('asan-'if asan_objects else 'normal-')+'link-'+target,mapped)
  binaries[str(binary.relative_to(a.output.parent))]=sha(binary);return binary
 normal={t:link_target(t)for t in TARGETS if t not in ('camouflage_tests','player_damage_tests')}
 for name in CHECKS:
  cmd=next(t['command']for t in ctests['tests']if t['name']==name).copy();target=Path(cmd[0]).name
  if target in normal:cmd[0]=str(normal[target])
  execute(name,cmd)
 sanitizer={}
 for obj,entry in selected.items():
  name=str(Path(entry['file']).relative_to(source))
  if name in ('src/multiplayer.cpp','src/multiplayer_network.cpp','tests/camouflage_runtime_tests.cpp'):
   o,p=compile_one(obj,entry,True);sanitizer[o]=p
 assert len(sanitizer)==3
 binary=link_target('camouflage_runtime_tests',sanitizer)
 cmd=next(t['command']for t in ctests['tests']if t['name']=='native_camouflage_runtime').copy();cmd[0]=str(binary)
 execute('native-camouflage-asan',cmd)
 assert all(sha(base/n)==digest for n,digest in base_files.items())
 assert all(sha(source/n)==digest for n,digest in pins.items())
 assert all(sha(source/n)==v['sha256']for n,v in assets.items())
 assert all(sha(overlay/n)==digest for n,digest in overlay_pins.items())
 result=dict(schema='native-camouflage-snapshot-verification-v1',boundary=__doc__,base=str(base),
     base_hashes_sha256=sha(run/'base-hashes.json'),source_hashes_sha256=sha(run/'source-hashes.json'),
     dependency_selection_sha256=sha(run/'recompiled-dependencies.json'),dependency_records_sha256=sha(run/'base-dependencies.txt'),
     overlay=str(overlay.relative_to(a.output.parent)),recompiled_objects=sorted(objects),
     replaced_archives={n:sha(p)for n,p in replaced_archives.items()},binaries=binaries,commands=commands,
     sanitizer_scope='Both changed production translation units and the camouflage runtime test; every affected ABI consumer rebuilt normally.',
     native_protocol=34,native_snapshot_binding=True,original_xbox_replication=False,renderer_binding=False,
     production_king_admission=False,goal_complete=False,
     live_owned_drift=[n for n,digest in overlay_pins.items()if sha(ROOT/n)!=digest],
     unrelated_live_drift=[n for n,digest in pins.items()if n not in OWNED and (not(ROOT/n).exists()or sha(ROOT/n)!=digest)])
 a.output.write_text(json.dumps(result,sort_keys=True,indent=2)+'\n');print(a.output,flush=True)
if __name__=='__main__':main()
