#!/usr/bin/env python3
"""Freeze original hill CPU emission comparisons and native arithmetic checks.

Root CMake checks the22 declared original post-preparation corridors against
the native geometry owner, including all14520 vertex words, real prefix fade
locals, FISTP results and caller FP restoration. The prior RGB/marker owner is
also checked. Full standalone ASan/UBSan/leaks cover the new native owner.
Graphics preparation, SDK restore, pixels and normal admission stay separate.
"""
import concurrent.futures,datetime,hashlib,json,os,re,shutil,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/multiplayer/game-variants/king-visual-native-20261012/verification.json'
BASE_PIN='03191a1d8ee4757db0ce6676aa823a54367aeb6bcfc39f23b16e3cd7bf585816'
REPORT=ROOT/'analysis/multiplayer/game-variants/king-hill-emission-source-20261012/complete/report.json'
REPORT_PIN='96cae012aec928d6f9e23cb756e11bfcfe189e062ec70cb54e0754b0adcddcb8'
OUT=ROOT/'analysis/multiplayer/game-variants/king-boundary-native-20261012'
OWNED=('include/halo2/source_king_boundary.hpp','src/source_king_boundary.cpp','tests/king_boundary_tests.cpp','tools/verify_king_boundary_native.py')
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 assert sha(BASE)==BASE_PIN and sha(REPORT)==REPORT_PIN
 prior=json.loads(BASE.read_text());base=Path(prior['source']);run=OUT/('verify-'+datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ'))
 run.mkdir(parents=True);source=run/'frozen-source';shutil.copytree(base,source,symlinks=True)
 for p in OWNED:
  target=source/p;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/p,target)
 live=(ROOT/'CMakeLists.txt').read_text();cmake=(base/'CMakeLists.txt').read_text()
 pattern=r'target_sources\(halo2_systems PRIVATE src/source_king_visual.cpp\)\n(?:target_sources\(halo2_systems PRIVATE src/source_king_boundary.cpp\)\n)?if\(MSVC\).*?endif\(\)\n'
 old=re.search(pattern,cmake,re.S).group();new=re.search(pattern,live,re.S).group();assert 'src/source_king_boundary.cpp'in new and 'src/source_king_boundary.cpp'not in old
 cmake=cmake.replace(old,new)
 declaration=re.search(r'        add_executable\(king_boundary_tests.*?king-hill-emission-source-20261012/complete/report\.json\)\n',live,re.S).group()
 anchor='        add_executable(king_engine_tests tests/king_engine_tests.cpp src/source_king.cpp src/slayer.cpp)\n';assert cmake.count(anchor)==1
 cmake=cmake.replace(anchor,declaration+anchor);(source/'CMakeLists.txt').write_text(cmake);(run/'live-CMakeLists.txt').write_text(live)
 (run/'cmake-overlay.json').write_text(json.dumps(dict(base_sha256=sha(base/'CMakeLists.txt'),live_sha256=sha(run/'live-CMakeLists.txt'),
  frozen_sha256=sha(source/'CMakeLists.txt'),production_before=old,production_after=new,test_declaration=declaration),sort_keys=True,indent=2)+'\n')
 files=[p for p in source.rglob('*')if p.is_file()and not p.is_symlink()]
 with concurrent.futures.ThreadPoolExecutor(max_workers=8)as pool:pins=dict(zip((str(p.relative_to(source))for p in files),pool.map(sha,files)))
 (run/'source-hashes.json').write_text(json.dumps(pins,sort_keys=True,indent=2)+'\n')
 inputs={str(BASE):BASE_PIN,str(REPORT):REPORT_PIN,str(base.parent/'source-hashes.json'):prior['source_hashes_sha256']}
 for p in ('include/halo2/source_king.hpp','include/halo2/source_king_visual.hpp','src/source_king_visual.cpp'):
  inputs[str(base/p)]=sha(base/p);assert sha(ROOT/p)==sha(base/p)
 (run/'input-hashes.json').write_text(json.dumps(inputs,sort_keys=True,indent=2)+'\n')
 commands=[];env={**os.environ,'PYTHONDONTWRITEBYTECODE':'1'}
 def execute(label,cmd,extra=None):
  cmd=list(map(str,cmd));log=run/(label+'.log')
  with log.open('w')as out:r=subprocess.run(cmd,cwd=source,env={**env,**(extra or {})},stdout=out,stderr=subprocess.STDOUT)
  commands.append(dict(label=label,command=cmd,exit=r.returncode,log=str(log.relative_to(OUT)),log_sha256=sha(log)))
  (run/'commands.json').write_text(json.dumps(commands,sort_keys=True,indent=2)+'\n');print(label,'exit',r.returncode,flush=True);assert r.returncode==0,log
 build=run/'build';execute('configure',['cmake','-S',source,'-B',build,'-G','Ninja','-DCMAKE_BUILD_TYPE=RelWithDebInfo','-DBUILD_TESTING=ON'])
 execute('build',['cmake','--build',build,'--target','king_boundary_tests','king_visual_tests','-j2'])
 execute('ctest',['ctest','--test-dir',build,'--verbose','-R','^original_king_(boundary|visual)$'])
 assert re.search(r'100% tests passed.*out of 2',(run/'ctest.log').read_text())
 sanitized=run/'king_boundary_tests_asan'
 execute('sanitizer-build',['/usr/bin/c++','-std=c++20','-O1','-g','-Wall','-Wextra','-Werror',
  '-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(source/'include'),
  source/'tests/king_boundary_tests.cpp',source/'src/source_king_boundary.cpp','-o',sanitized])
 execute('sanitizer-test',[sanitized,REPORT],{'ASAN_OPTIONS':'detect_leaks=1:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'})
 assert all(sha(p)==v for p,v in inputs.items())
 with concurrent.futures.ThreadPoolExecutor(max_workers=8)as pool:current=list(pool.map(lambda p:sha(source/p),pins))
 assert all(pins[p]==v for p,v in zip(pins,current));assert all(sha(ROOT/p)==pins[p]for p in OWNED)
 result=dict(schema='king-boundary-native-v1',scope=__doc__,base_manifest=str(BASE),base_manifest_sha256=BASE_PIN,
  source=str(source),source_hashes_sha256=sha(run/'source-hashes.json'),input_hashes_sha256=sha(run/'input-hashes.json'),
  cmake_overlay_sha256=sha(run/'cmake-overlay.json'),commands=commands,
  binaries={str(p.relative_to(OUT)):sha(p)for p in(build/'king_visual_tests',build/'king_boundary_tests',sanitized)},
  native_vertex_comparisons=1452,binary32_word_comparisons=14520,original_corridor_cases=22,caller_fp_restored=True,
  original_whole_positive_draw=False,native_graphics_binding=False,rendered_pixels=False,normal_king_admission=False,goal_complete=False)
 (run/'verification.json').write_text(json.dumps(result,sort_keys=True,indent=2)+'\n');shutil.copy2(run/'verification.json',OUT/'verification.json');print(OUT/'verification.json',flush=True)
if __name__=='__main__':main()
