#!/usr/bin/env python3
"""Native3190D0 parent with genuine typed remaining children on actual09 DATA."""
import argparse,hashlib,os,shutil,subprocess,sys
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
from build_native_tu15_constraint_before_reference import BeforeInput,build_constraint_reference,sha,save
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--capture',type=Path,required=True);p.add_argument('--archive-sha256',required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--compilers',nargs='+',default=['g++','clang++']);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False)
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 kernel=ROOT/'src/physics/native_tu15_island_merge.cpp';header=ROOT/'include/halo2/native_tu15_island_merge.hpp';observer=ROOT/'tools/observe_native_island_merge_actual_before_tu15.cpp'
 files=[kernel,header,observer,Path(__file__),ROOT/'tools/observe_native_activation_agents_tu15.cpp',ROOT/'tools/build_native_tu15_constraint_before_reference.py'];pins={str(f.relative_to(ROOT)):sha(f)for f in files}
 try:
  source=BeforeInput(a.capture,a.archive_sha256,scratch/'before');reference=build_constraint_reference(source,scratch/'reference');reference['entry']='Declared original3190D0 ECXworld, two stack island arguments; frame-level BEFORE only';save(a.output/'reference.json',reference)
  from capstone import CS_AC_READ,CS_AC_WRITE
  from capstone.x86 import X86_OP_MEM
  assert sha(NEW_DEFAULT)==NEW_PIN;code=_Code(Xbe(NEW_DEFAULT),'.text');body=complete_body(code,0x3190d0);metadata={};listing=[]
  for pc in sorted(body.code):
   i=code.at(pc);listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');operands=[]
   if i.mnemonic!='lea':
    for op in i.operands:
     if op.type!=X86_OP_MEM or op.mem.segment:continue
     operands.append(dict(base=i.reg_name(op.mem.base)or'',index=i.reg_name(op.mem.index)or'',scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,
      bytes=op.size,read=bool(op.access&CS_AC_READ)or i.mnemonic=='test',write=bool(op.access&CS_AC_WRITE)))
   row=dict(bytes=i.bytes.hex(),size=i.size,memory=operands)
   if i.mnemonic=='call':
    target=i.operands[0].imm if i.operands[0].type==2 else None;row.update(service_arguments=3 if target==0x2e31e0 else 1,ecx_consumed=target in[0x316d40,None],result_consumed=False)
   metadata[str(pc)]=row
  save(scratch/'instructions.json',metadata)
  prefix=(ROOT/'tools/observe_native_activation_agents_tu15.cpp').read_text().split('int main(int argc,char**argv)')[0]
  needle='{"this",std::uint32_t(entered.rcx)}';assert prefix.count(needle)==1;prefix=prefix.replace(needle,needle+',{ "formal_ESI",std::uint32_t(entered.rsi)}')
  driver=scratch/'driver.cpp';driver.write_text('#include "halo2/native_tu15_island_merge.hpp"\n'+prefix+observer.read_text())
  project=scratch/'project';project.mkdir();shutil.copyfile(kernel,project/'merge.cpp');text=kernel.read_text();needle='store(0x319162u,body+0x44u,a)';assert text.count(needle)==1
  (project/'wrong_body_island.cpp').write_text(text.replace(needle,'store(0x319162u,body+0x48u,a)'))
  (project/'negative.py').write_text('import json,subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nj=json.load(open(sys.argv[-1]));assert r.returncode!=0 and not j.get("error")and any(not x["ordered_parent_operands_equal"]for x in j["cases"]),(r.returncode,j)\nprint("Original3190D0 rejects wrong native body island publication")\n')
  cmake=f'''cmake_minimum_required(VERSION 3.20)
project(actual_island_merge LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json REQUIRED)
add_library(driver OBJECT "{driver}")
target_include_directories(driver PRIVATE "{ROOT}/include")
target_compile_features(driver PRIVATE cxx_std_20)
target_link_libraries(driver PRIVATE nlohmann_json::nlohmann_json)
'''
  for variant in ['native','wrong_body_island']:
   cpp='merge.cpp'if variant=='native'else'wrong_body_island.cpp';name='merge_'+variant
   cmake+=f'add_executable({name} $<TARGET_OBJECTS:driver> {cpp})\ntarget_include_directories({name} PRIVATE "{ROOT}/include")\ntarget_compile_features({name} PRIVATE cxx_std_20)\ntarget_compile_options({name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
   command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '+(''if variant=='native'else f'"{sys.executable}" "{project}/negative.py" ')+f'$<TARGET_FILE:{name}> "{reference["elf"]}" "{source.path}/capture-native-before.json" "{scratch}/instructions.json" "${{CMAKE_BINARY_DIR}}/{variant}.json"'
   cmake+=f'add_test(NAME {name} COMMAND {command})\nset_tests_properties({name} PROPERTIES TIMEOUT 240)\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in a.compilers:
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','merge_native','--target','merge_wrong_body_island','--filter','^merge_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1','--retain-report','native.json','--retain-report','wrong_body_island.json'];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  unchanged=all(sha(ROOT/f)==h for f,h in pins.items());report.update(passed=all(r['exit_code']==0 for r in results)and unchanged,current_sources_unchanged=unchanged,source_hashes=pins,compiler_results=results,
   reference=reference,source_sites=len(body.code),listing_sha256=hashlib.sha256(''.join(listing).encode()).hexdigest(),original_sha256=NEW_PIN,
   captured_guest_ABI=False,genuine_queued_pair_from_frame=True,swapped_and_identical_pairs_are_declared_formal_inputs=True,native_parent_falls_back=False,
   remaining_typed_children_execute_original_CPU=True,remaining=['Native318DE0 contact-record merge and its lifetime helpers','Native322100/3213A0 island destructor','Compose native reserve/activation service binding','Live World selection and positive wake/listener'])
 finally:
  report['temporary_allocated_bytes_before_cleanup']=sum(f.stat().st_blocks*512 for f in scratch.rglob('*')if f.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
