#!/usr/bin/env python3
"""Disposable production native unit-animation-reset validation and mutation."""
import argparse,hashlib,json,os,re,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
FILES=['cmake/CampaignNativeUnitAnimationReset.cmake','cmake/TestStorage.cmake','tools/run_test.py','include/halo2/physics_contact_record.hpp','include/halo2/physics_world_runtime.hpp','include/halo2/native_campaign_unit_animation_reset_tu15.hpp','include/halo2/native_campaign_unit_animation_reset_tu15_binding.hpp','src/campaign/ai/native_campaign_unit_animation_reset_tu15.cpp','src/campaign/ai/native_campaign_unit_animation_reset_tu15_binding.cpp','tests/native_campaign_unit_animation_reset_tu15_tests.cpp','tests/fixtures/native_campaign_unit_animation_reset_tu15.json']
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--compiler',default='clang++');p.add_argument('--sanitizers',action='store_true');a=p.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();project=scratch/'unit-animation-reset';project.mkdir();pins={}
 files=list(FILES)
 for f in files:
  for child in re.findall(r'#include "(halo2/[^"\n]+)"',(ROOT/f).read_text()):
   header="include/"+child
   if header not in files:files.append(header)
  data=(ROOT/f).read_bytes();target=project/f;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data);pins[f]=hashlib.sha256(data).hexdigest()
 (project/'CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.24)
project(CampaignNativeUnitAnimationReset LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
option(HALO2_SANITIZERS "Instrument animation reset" OFF)
if(HALO2_SANITIZERS)
 add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
 add_link_options(-fsanitize=address,undefined)
endif()
find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_package(nlohmann_json REQUIRED)
include(CTest)
include(cmake/TestStorage.cmake)
add_library(halo2_systems STATIC)
target_include_directories(halo2_systems PRIVATE include)
include(cmake/CampaignNativeUnitAnimationReset.cmake)
''');a.output.mkdir(parents=True,exist_ok=True)
 command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/'normal'),'--target','native_campaign_unit_animation_reset_tu15_tests','--filter','^original_native_campaign_unit_animation_reset_tu15','--jobs','2','--retain-report','compile_commands.json','--cmake-arg=-DCMAKE_CXX_COMPILER='+a.compiler]
 if a.sanitizers:command.append('--sanitizers')
 status=subprocess.run(command).returncode;changed=[f for f,h in pins.items() if hashlib.sha256((project/f).read_bytes()).hexdigest()!=h];negative=None;callback_negative=None;eager_negative=None
 if status==0 and not a.sanitizers:
  variants=[('src/campaign/ai/native_campaign_unit_animation_reset_tu15.cpp','second.reset_lookup_keys();','/* missing actual second-channel lookup reset */','missing-second-lookup'),('src/campaign/ai/native_campaign_unit_animation_reset_tu15.cpp','first.set_rate(one_bits);','first.set_rate(0);','wrong-primary-rate'),('src/campaign/ai/native_campaign_unit_animation_reset_tu15.cpp','first.clear_frame();first.set_rate(one_bits);','first.set_rate(one_bits);first.clear_frame();','publication-order')]
  results=[]
  for path,before,after,name in variants:
   source=project/path;text=source.read_text();assert text.count(before)==1;source.write_text(text.replace(before,after));mutation=command.copy();mutation[mutation.index('--output')+1]=str(a.output/name);r=subprocess.run(mutation).returncode;log=a.output/name/'tests.log';results.append(r!=0 and log.exists() and 'ordered animation reset events differ' in log.read_text());source.write_text(text)
  negative,callback_negative,eager_negative=results

 compilation=json.loads((a.output/'normal/evidence/compile_commands.json').read_text());core=[x for x in compilation if x['file'].endswith('/src/campaign/ai/native_campaign_unit_animation_reset_tu15.cpp') and 'native_campaign_unit_animation_reset_tu15_tests.dir' in x.get('command',' '.join(x.get('arguments',[])))];assert len(core)==1;core_command=core[0].get('command',' '.join(core[0].get('arguments',[])));instrumented='-fsanitize=address,undefined' in core_command
 report=dict(passed=status==0 and not changed and negative is not False and callback_negative is not False and eager_negative is not False and (not a.sanitizers or instrumented),source_sha256=pins,source_snapshot_changed=changed,checkout_drift=[f for f,h in pins.items() if hashlib.sha256((ROOT/f).read_bytes()).hexdigest()!=h],compiler=a.compiler,sanitizers=a.sanitizers,actual_native_core_compile_command=core_command,actual_native_core_asan_ubsan_instrumented=instrumented,compiled_missing_second_lookup_mutation_rejected=negative,compiled_wrong_primary_rate_mutation_rejected=callback_negative,compiled_publication_order_mutation_rejected=eager_negative,full_e8e10_e8e50_closed=False,lifted_fallback=False,retained_large_artifacts=[],command=command);(a.output/'source-cohort.json').write_text(json.dumps(report,indent=2)+'\n');return 0 if report['passed'] else 1

if __name__=='__main__':raise SystemExit(main())
