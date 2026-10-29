#!/usr/bin/env python3
"""Disposable production native campaign camouflage/dirty-entity validation."""
import argparse,hashlib,json,os,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
FILES=['cmake/CampaignNativeAiOrderCamouflage.cmake','cmake/TestStorage.cmake','tools/run_test.py','include/halo2/physics_contact_record.hpp','include/halo2/native_campaign_ai_orders_camouflage_tu15.hpp','src/campaign/ai/native_campaign_ai_orders_camouflage_tu15.cpp','tests/native_campaign_ai_order_camouflage_tu15_tests.cpp','tests/fixtures/native_campaign_ai_order_camouflage_tu15.json']
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--compiler',default='clang++');p.add_argument('--sanitizers',action='store_true');a=p.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();project=scratch/'orders-camouflage';project.mkdir();pins={}
 for f in FILES:
  data=(ROOT/f).read_bytes();target=project/f;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data);pins[f]=hashlib.sha256(data).hexdigest()
 (project/'CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.24)
project(CampaignNativeAiOrderCamouflage LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(HALO2_SANITIZERS "Instrument campaign camouflage owner" OFF)
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
include(cmake/CampaignNativeAiOrderCamouflage.cmake)
''');a.output.mkdir(parents=True,exist_ok=True)
 command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/'normal'),'--target','native_campaign_ai_order_camouflage_tu15_tests','--filter','^original_native_campaign_ai_order_camouflage_tu15$','--jobs','2','--cmake-arg=-DCMAKE_CXX_COMPILER='+a.compiler]
 if a.sanitizers:command.append('--sanitizers')
 status=subprocess.run(command).returncode;changed=[f for f,h in pins.items() if hashlib.sha256((project/f).read_bytes()).hexdigest()!=h];negative=None
 if status==0 and not a.sanitizers:
  source=project/'src/campaign/ai/native_campaign_ai_orders_camouflage_tu15.cpp';text=source.read_text();before='if(entity!=0xffffffffu)dirty(m,entity);';assert text.count(before)==1;source.write_text(text.replace(before,'(void)entity;'));mutation=command.copy();mutation[mutation.index('--output')+1]=str(a.output/'mutation');r=subprocess.run(mutation).returncode;log=a.output/'mutation/tests.log';negative=r!=0 and log.exists() and 'ordered camouflage events differ' in log.read_text()
 report=dict(passed=status==0 and not changed and negative is not False,source_sha256=pins,source_snapshot_changed=changed,checkout_drift=[f for f,h in pins.items() if hashlib.sha256((ROOT/f).read_bytes()).hexdigest()!=h],compiler=a.compiler,sanitizers=a.sanitizers,compiled_missing_dirty_publication_mutation_rejected=negative,full_actor_order_reset_implemented=False,full_ai_set_orders_implemented=False,live_campaign_qualified=False,retained_large_artifacts=[],command=command);(a.output/'source-cohort.json').write_text(json.dumps(report,indent=2)+'\n');return 0 if report['passed'] else 1
if __name__=='__main__':raise SystemExit(main())
