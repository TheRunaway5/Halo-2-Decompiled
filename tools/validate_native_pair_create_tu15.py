#!/usr/bin/env python3
"""Native320EF0 at genuine original child entries from actual02 typed input."""
import argparse,hashlib,inspect,json,os,shutil,subprocess,sys,textwrap,types
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
import validate_native_pair_consumer_actual_before_tu15 as parent

def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def save(p,j):p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--capture',type=Path,required=True);p.add_argument('--archive-sha256',required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--compilers',nargs='+',default=['g++','clang++']);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],supplied_original_outcomes=False)
 files=['include/halo2/native_tu15_pair_handlers.hpp','src/physics/native_tu15_pair_handlers.cpp','tools/observe_native_pair_create_tu15.cpp','tools/observe_native_pair_consumer_tu15.cpp','tools/observe_native_contact_producer_tu15.cpp','tools/validate_native_pair_consumer_actual_before_tu15.py','tools/validate_native_pair_create_tu15.py'];pins={f:sha(ROOT/f)for f in files}
 try:
  # Reuse the exact authenticated before-only loader, stopping before its
  # parent project generation. This compiles no reference outcome and changes
  # no capture bank, capacity or source-owned allocation metadata.
  source=inspect.getsource(parent.main);prefix=source.split(' try:\n',1)[1].split('  project=scratch',1)[0];environment=dict(parent.__dict__);environment.update(a=a,scratch=scratch,report=report);local=environment;exec("\n".join(line[2:]if line.startswith("  ")else line for line in prefix.splitlines()),environment);elf=local['elf'];capture=scratch/'capture.json';code=_Code(Xbe(NEW_DEFAULT),'.text');body=complete_body(code,0x320ef0);metadata={};lines=[];callback_args={0x321035:1,0x32104e:1,0x321070:2,0x321092:3,0x3210ab:0,0x3210b4:0,0x3210d3:4,0x3210f9:0,0x321111:0,0x321132:3,0x321160:0}
  for pc in sorted(body.code):
   i=code.at(pc);lines.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');ops=[]
   for op in i.operands:
    if op.type!=X86_OP_MEM or i.mnemonic=='lea':continue
    access=CS_AC_READ if i.mnemonic in('test','cmp')else op.access;ops.append(dict(base=i.reg_name(op.mem.base)or'',index=i.reg_name(op.mem.index)or'',scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(access&CS_AC_READ),write=bool(access&CS_AC_WRITE)))
   metadata[str(pc)]=dict(memory=ops)
   if pc in callback_args:metadata[str(pc)].update(arguments=callback_args[pc],next=pc+i.size)
  save(scratch/'handler-metadata.json',metadata);project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');shutil.copyfile(ROOT/'src/physics/native_tu15_pair_handlers.cpp',project/'native.cpp');driver='#include "halo2/native_tu15_pair_handlers.hpp"\n'+(ROOT/'tools/observe_native_contact_producer_tu15.cpp').read_text().split('int main(int argc,char**argv)')[0]+(ROOT/'tools/observe_native_pair_consumer_tu15.cpp').read_text().split('int main(int argc,char**argv)')[0]+(ROOT/'tools/observe_native_pair_create_tu15.cpp').read_text();(project/'driver.cpp').write_text(driver);cmake='''cmake_minimum_required(VERSION 3.20)
project(native_pair_create LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json REQUIRED)
add_executable(pair_create driver.cpp native.cpp)
target_include_directories(pair_create PRIVATE include)
target_compile_features(pair_create PRIVATE cxx_std_20)
target_compile_options(pair_create PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(pair_create PRIVATE nlohmann_json::nlohmann_json)
''';retain=[]
  for ordinal in(0,1):
   for poison in range(14):
    name=f'child{ordinal}_poison{poison}';path=name+'.json';retain.append(path);cmake+=f'add_test(NAME {name} COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- $<TARGET_FILE:pair_create> "{elf}" "{capture}" {ordinal} {poison} "{scratch}/handler-metadata.json" "${{CMAKE_BINARY_DIR}}/{path}")\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in a.compilers:
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','pair_create','--filter','^child','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1',*sum((['--retain-report',p]for p in retain),[])];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  original_kernel=(project/'native.cpp').read_text();negative=[]
  for name,needle,replacement in [('first_record_word','m.word(0x320feeu,out,first?first+0x10u:0u);','m.word(0x320feeu,out,first?(first+0x10u)^1u:0u);'),('formal_DATA_dispatch','m.word(0x3210a7u,scratch.formal_pair_data,dispatch);','m.word(0x3210a7u,scratch.formal_pair_data,dispatch^1u);'),('output_advance','m.word(0x321163u,handler+0x88u,destination+16u);','m.word(0x321163u,handler+0x88u,destination+8u);')]:
   assert original_kernel.count(needle)==1;(project/'native.cpp').write_text(original_kernel.replace(needle,replacement));label='negative-'+name;cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/label),'--target','pair_create','--filter','^child0_poison0$','--cmake-arg=-DCMAKE_CXX_COMPILER='+a.compilers[0],'--jobs','1','--test-jobs','1','--retain-report','child0_poison0.json'];r=subprocess.run(cmd);observed=json.loads((a.output/label/'evidence/child0_poison0.json').read_text());assert r.returncode!=0 and not observed.get('error') and not observed['passed'],'Original source failed to reject semantic mutation';negative.append(dict(name=name,exit_code=r.returncode,command=cmd,rejected=True));(project/'native.cpp').write_text(original_kernel)
  report.update(passed=all(r['exit_code']==0 for r in results)and all(sha(ROOT/f)==v for f,v in pins.items()),source_hashes=pins,compiler_results=results,negative_controls=negative,source_entry=0x320ef0,body_sites=len(body.code),listing_sha256=hashlib.sha256(''.join(lines).encode()).hexdigest(),original_sha256=NEW_PIN,archive_sha256=a.archive_sha256,actual_typed_parent_input=local['typed'],captured_after_consumed=False,source_child_entry_GP_consumed_by_native_kernel=False,independent_remaining_services_GP_XMM_poison=True,scope='Two genuine original create child entries after captured typed positiveparent; native standalone parent; strictformalDATA/currentpersistentbytes/readstores/FP/callbacks; remaining original service bodies are explicit')
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
