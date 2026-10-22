#!/usr/bin/env python3
"""Compare native hill color/marker production with unchanged original owners.

All1488 whole RGB outputs,105 invalid marker returns and84 positive marker
records come from original retail/TU1.1/TU1.5 execution. The actual Lockout
mulg palette is re-read. Root CMake and standalone ASan/UBSan check the native
owners; projection, rendered geometry and normal King admission stay separate.
"""
import concurrent.futures,datetime,hashlib,json,os,re,shutil,subprocess,sys
from pathlib import Path
from oracle_mp_full_scoreboard_1_5 import HudCache
ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/multiplayer/game-variants/king-hud-native-20261012/verification.json'
BASE_PIN='8ba1f7bd52f15a766ecbadf279a7abccfaea21bd46df7146d47d2cd4ef7f0d43'
REPORT=ROOT/'analysis/multiplayer/game-variants/king-visual-consumers-source-20261012/complete/report.json'
REPORT_PIN='5921a37a3dc5c5e4af9539bf770f3455885222e1d08505c78473e174b887288a'
OUT=ROOT/'analysis/multiplayer/game-variants/king-visual-native-20261012'
OWNED=('include/halo2/source_king_visual.hpp','src/gameplay/multiplayer/source_king_visual.cpp','tests/king_visual_tests.cpp',
 'tests/fixtures/king_visual_team_colors_original.json','tools/verify_king_visual_native.py')
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 assert sha(BASE)==BASE_PIN and sha(REPORT)==REPORT_PIN
 prior=json.loads(BASE.read_text());base=Path(prior['source']);run=OUT/('verify-'+datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ'))
 run.mkdir(parents=True);source=run/'frozen-source';shutil.copytree(base,source,symlinks=True)
 for p in OWNED:
  target=source/p;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/p,target)
 live=(ROOT/'CMakeLists.txt').read_text();cmake=(base/'CMakeLists.txt').read_text()
 line=re.search(r'target_sources\(halo2_systems PRIVATE src/gameplay/multiplayer/source_king_visual.cpp\)\n(?:target_sources\(halo2_systems PRIVATE src/gameplay/multiplayer/source_king_boundary.cpp\)\n)?if\(MSVC\).*?endif\(\)\n',live,re.S).group()
 line=line.replace('target_sources(halo2_systems PRIVATE src/gameplay/multiplayer/source_king_boundary.cpp)\n','').replace('src/gameplay/multiplayer/source_king_visual.cpp src/gameplay/multiplayer/source_king_boundary.cpp','src/gameplay/multiplayer/source_king_visual.cpp')
 anchor='target_sources(halo2_systems PRIVATE src/gameplay/multiplayer/source_score_event.cpp)\n';assert live.count(line)==1 and cmake.count(anchor)==1 and line not in cmake
 cmake=cmake.replace(anchor,anchor+line)
 declaration=re.search(r'        add_executable\(king_visual_tests.*?king_visual_team_colors_original\.json\)\n',live,re.S).group()
 anchor='        add_executable(king_engine_tests tests/king_engine_tests.cpp src/gameplay/multiplayer/source_king.cpp src/gameplay/multiplayer/slayer.cpp)\n';assert cmake.count(anchor)==1
 cmake=cmake.replace(anchor,declaration+anchor);(source/'CMakeLists.txt').write_text(cmake);(run/'live-CMakeLists.txt').write_text(live)
 (run/'cmake-overlay.json').write_text(json.dumps(dict(base_sha256=sha(base/'CMakeLists.txt'),live_sha256=sha(run/'live-CMakeLists.txt'),
  frozen_sha256=sha(source/'CMakeLists.txt'),source_line=line,test_declaration=declaration),sort_keys=True,indent=2)+'\n')
 fixture=json.loads((source/'tests/fixtures/king_visual_team_colors_original.json').read_text());cache=HudCache(ROOT/fixture['cache'])
 assert sha(cache.path)==fixture['cache_sha256'];matg=next(t for t in cache.tags.values()if t['group']=='matg')
 mulg=cache.tags[cache.u(matg['offset']+0x16c)];universal=cache.block(mulg['offset'],0x20)[0];rows=cache.block(universal+0x10,12)
 assert fixture['matg']==matg and fixture['mulg']==mulg and fixture['universal_file_offset']==universal
 assert fixture['color_file_offsets']==rows and fixture['colors_hex']==[cache.data[r:r+12].hex()for r in rows]
 assert fixture['block_header_hex']==cache.data[universal+0x10:universal+0x18].hex()
 files=[p for p in source.rglob('*')if p.is_file()and not p.is_symlink()]
 with concurrent.futures.ThreadPoolExecutor(max_workers=8)as pool:pins=dict(zip((str(p.relative_to(source))for p in files),pool.map(sha,files)))
 (run/'source-hashes.json').write_text(json.dumps(pins,sort_keys=True,indent=2)+'\n')
 inputs={str(BASE):BASE_PIN,str(REPORT):REPORT_PIN,str(cache.path):sha(cache.path),str(base.parent/'source-hashes.json'):prior['source_hashes_sha256']}
 for p in ('include/halo2/source_king.hpp',):inputs[str(base/p)]=sha(base/p);assert sha(ROOT/p)==sha(base/p)
 (run/'input-hashes.json').write_text(json.dumps(inputs,sort_keys=True,indent=2)+'\n')
 commands=[];env={**os.environ,'PYTHONDONTWRITEBYTECODE':'1'}
 def execute(label,cmd,extra=None):
  cmd=list(map(str,cmd));log=run/(label+'.log')
  with log.open('w')as out:r=subprocess.run(cmd,cwd=source,env={**env,**(extra or {})},stdout=out,stderr=subprocess.STDOUT)
  commands.append(dict(label=label,command=cmd,exit=r.returncode,log=str(log.relative_to(OUT)),log_sha256=sha(log)))
  (run/'commands.json').write_text(json.dumps(commands,sort_keys=True,indent=2)+'\n');print(label,'exit',r.returncode,flush=True);assert r.returncode==0,log
 build=run/'build';execute('configure',['cmake','-S',source,'-B',build,'-G','Ninja','-DCMAKE_BUILD_TYPE=RelWithDebInfo','-DBUILD_TESTING=ON'])
 execute('build',['cmake','--build',build,'--target','king_visual_tests','-j2'])
 execute('ctest',['ctest','--test-dir',build,'--verbose','-R','^original_king_visual$'])
 assert re.search(r'100% tests passed.*out of 1',(run/'ctest.log').read_text())
 sanitized=run/'king_visual_tests_asan'
 execute('sanitizer-build',['/usr/bin/c++','-std=c++20','-O1','-g','-Wall','-Wextra','-Werror',
  '-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-fast-math','-ffp-contract=off','-I'+str(source/'include'), '-I'+str(source/'src'),
  source/'tests/king_visual_tests.cpp',source/'src/gameplay/multiplayer/source_king_visual.cpp','-o',sanitized])
 execute('sanitizer-test',[sanitized,REPORT,source/'tests/fixtures/king_visual_team_colors_original.json'],
  {'ASAN_OPTIONS':'detect_leaks=1:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'})
 assert all(sha(p)==v for p,v in inputs.items())
 with concurrent.futures.ThreadPoolExecutor(max_workers=8)as pool:current=list(pool.map(lambda p:sha(source/p),pins))
 assert all(pins[p]==v for p,v in zip(pins,current));assert all(sha(ROOT/p)==pins[p]for p in OWNED)
 result=dict(schema='king-visual-native-v1',scope=__doc__,base_manifest=str(BASE),base_manifest_sha256=BASE_PIN,
  source=str(source),source_hashes_sha256=sha(run/'source-hashes.json'),input_hashes_sha256=sha(run/'input-hashes.json'),
  cmake_overlay_sha256=sha(run/'cmake-overlay.json'),commands=commands,
  binaries={str(p.relative_to(OUT)):sha(p)for p in(build/'king_visual_tests',sanitized)},
  whole_color_comparisons=1488,marker_invalid_return_comparisons=105,defined_marker_prefix_comparisons=84,
  original_palette_reextracted=True,native_projection=False,rendered_hill=False,normal_king_admission=False,goal_complete=False)
 (run/'verification.json').write_text(json.dumps(result,sort_keys=True,indent=2)+'\n');shutil.copy2(run/'verification.json',OUT/'verification.json');print(OUT/'verification.json',flush=True)
if __name__=='__main__':main()
