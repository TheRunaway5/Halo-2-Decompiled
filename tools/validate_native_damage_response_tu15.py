#!/usr/bin/env python3
"""Verify native response selection against the five immutable TU1.5 entries.

This proves the small typed decision, not the surrounding damage owner. The
original XBE operands and continuation blocks supply expectations; no emulator
or Xbox table supplies production outcomes. Private compiler outputs are owned.
"""
import argparse,hashlib,json,os,shutil,struct,subprocess,sys
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']);header=ROOT/'include/halo2/native_tu15_damage_response.hpp';pins={str(header.relative_to(ROOT)):sha(header),str(Path(__file__).relative_to(ROOT)):sha(Path(__file__))};report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],source_hashes=pins)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
  assert image.read(0xdcb71,4)==bytes.fromhex('0fb74b02')
  assert image.read(0xdcb75,7)==bytes.fromhex('ff248d98cd0d00')
  table=list(struct.unpack('<5I',image.read(0xdcd98,20)))
  # These labels are tied to the authenticated continuation blocks and their
  # existing semantic consumers, independently of the source kind ordering.
  destinations={'Type0':0xdcbea,'Type1':0xdcbd1,'Type2':0xdcba5,'Stop':0xdcd81,'Type4':0xdcbbb}
  assert set(table)==set(destinations.values())
  for target in table:assert target in range(0xdc660,0xdcd88);code.at(target)
  project=scratch/'project';project.mkdir();shutil.copyfile(header,project/'native.hpp');wrong=header.read_text().replace('case 4:return NativeTu15DamageResponseRoute::Type4;','case 4:return NativeTu15DamageResponseRoute::Type2;');assert wrong!=header.read_text();(project/'wrong.hpp').write_text(wrong)
  cases=','.join(hex(value)+'u'for value in table)
  driver='''#include <array>
#include <iostream>
#ifdef WRONG_ROUTE
#include "wrong.hpp"
#else
#include "native.hpp"
#endif
int main(){using namespace halo2;const std::array<unsigned,5> original{'''+cases+'''};for(unsigned kind=0;kind<5;++kind){unsigned destination{};switch(native_tu15_damage_response_route(kind)){'''+''.join(f'case NativeTu15DamageResponseRoute::{name}:destination=0x{target:x}u;break;'for name,target in destinations.items())+'''}if(destination!=original[kind])return 1;}for(unsigned kind:{5u,255u,65535u}){bool refused{};try{(void)native_tu15_damage_response_route(kind);}catch(const std::runtime_error&){refused=true;}if(!refused)return 2;}std::cout<<"All five authenticated TU1.5 response kinds and explicit domain refusals pass\\n";}
'''
  (project/'driver.cpp').write_text(driver);(project/'reject.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:]);assert r.returncode==1,r.returncode\n')
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(NativeDamageResponse LANGUAGES CXX)\ninclude(CTest)\n'
  for name in ['native','wrong']:
   cmake+=f'add_executable(response_{name} driver.cpp)\ntarget_compile_features(response_{name} PRIVATE cxx_std_20)\n'
   if name=='wrong':cmake+='target_compile_definitions(response_wrong PRIVATE WRONG_ROUTE=1)\n'
   command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '+(''if name=='native'else f'"{sys.executable}" "{project}/reject.py" ')+f'$<TARGET_FILE:response_{name}>';cmake+=f'add_test(NAME response_{name} COMMAND {command})\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in ['g++','clang++']:
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','response_native','--target','response_wrong','--filter','^response_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];result=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=result.returncode,command=command))
  report.update(passed=all(r['exit_code']==0 for r in results)and all(sha(ROOT/p)==pin for p,pin in pins.items()),compiler_results=results,image_sha256=NEW_PIN,original_table_address=0xdcd98,original_table_bytes=image.read(0xdcd98,20).hex(),original_continuations=table,original_kinds=5,negative_control='Kind4 incorrectly routes to kind2; the independent original table rejects it',remaining=['Whole damage-response parent native conversion','Visible composed multiplayer damage and destruction','Unauthored response kinds are explicit refusals'])
 finally:
  report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
