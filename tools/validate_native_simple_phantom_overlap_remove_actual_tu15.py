#!/usr/bin/env python3
"""Actual earlier02 phantom graph + measured06 tuple, ordinary original leaf ABI."""
import argparse,hashlib,inspect,json,os,shutil,subprocess,sys
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
import validate_native_pair_consumer_actual_before_tu15 as parent
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body

def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def save(p,j):p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--capture',type=Path,required=True);p.add_argument('--archive-sha256',required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--compilers',nargs='+',default=['g++','clang++']);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();files=['include/halo2/native_tu15_phantom_overlap_remove.hpp','src/physics/native_tu15_phantom_overlap_remove.cpp','include/halo2/native_tu15_simple_phantom_overlap_remove.hpp','src/physics/native_tu15_simple_phantom_overlap_remove.cpp','tools/observe_native_simple_phantom_overlap_remove_actual_tu15.cpp','tools/validate_native_simple_phantom_overlap_remove_actual_tu15.py','tools/validate_native_pair_consumer_actual_before_tu15.py','tools/observe_native_contact_producer_tu15.cpp','tools/observe_native_pair_consumer_tu15.cpp'];pins={f:sha(ROOT/f)for f in files};report=dict(passed=False,owned_scratch=str(scratch),source_hashes=pins,retained_large_artifacts=[],actual06_entry_capture_claimed=False,supplied_original_outcomes=False)
 try:
  source=inspect.getsource(parent.main);prefix=source.split(' try:\n',1)[1].split('  project=scratch',1)[0];needle="  args=[typed['first']";assert prefix.count(needle)==1;prefix=prefix.replace(needle,"  assembly.append('.section .leaf_stack,\"aw\"\\n.zero 65536');sections.append('.leaf_stack 0x02000000:{*(.leaf_stack)}')\n"+needle);environment=dict(parent.__dict__);environment.update(a=a,scratch=scratch,report=report);exec('\n'.join(line[2:]if line.startswith('  ')else line for line in prefix.splitlines()),environment);elf=environment['elf'];code=_Code(Xbe(NEW_DEFAULT),'.text');metadata={};counts=[];listing=[]
  for owner in(0x315100,0x314fe0):
   b=complete_body(code,owner);counts.append(len(b.code))
   for pc in sorted(b.code):
    i=code.at(pc);listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');ops=[]
    for op in i.operands:
     if op.type!=X86_OP_MEM or i.mnemonic=='lea':continue
     access=CS_AC_READ if i.mnemonic in('cmp','test')else op.access;ops.append(dict(base=i.reg_name(op.mem.base)or'',index=i.reg_name(op.mem.index)or'',scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(access&CS_AC_READ),write=bool(access&CS_AC_WRITE)))
    metadata[str(pc)]=dict(memory=ops)
  save(scratch/'leaf-metadata.json',metadata);project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');shutil.copyfile(ROOT/'src/physics/native_tu15_simple_phantom_overlap_remove.cpp',project/'native.cpp');shutil.copyfile(ROOT/'src/physics/native_tu15_phantom_overlap_remove.cpp',project/'support.cpp');driver='#include "halo2/native_tu15_simple_phantom_overlap_remove.hpp"\n'+(ROOT/'tools/observe_native_contact_producer_tu15.cpp').read_text().split('int main(int argc,char**argv)')[0]+(ROOT/'tools/observe_native_pair_consumer_tu15.cpp').read_text().split('int main(int argc,char**argv)')[0]+(ROOT/'tools/observe_native_simple_phantom_overlap_remove_actual_tu15.cpp').read_text();(project/'driver.cpp').write_text(driver);(project/'CMakeLists.txt').write_text('cmake_minimum_required(VERSION 3.20)\nproject(actual_overlap LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json REQUIRED)\nadd_executable(overlap driver.cpp native.cpp support.cpp)\ntarget_include_directories(overlap PRIVATE include)\ntarget_compile_features(overlap PRIVATE cxx_std_20)\ntarget_link_libraries(overlap PRIVATE nlohmann_json::nlohmann_json)\nadd_test(NAME actual_overlap COMMAND "'+sys.executable+'" "'+str(ROOT/'tools/run_test.py')+'" -- $<TARGET_FILE:overlap> "'+str(elf)+'" "'+str(scratch/'capture.json')+'" "'+str(scratch/'leaf-metadata.json')+'" "${CMAKE_BINARY_DIR}/actual.json")\n');results=[]
  for compiler in a.compilers:
   cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','overlap','--filter','^actual_overlap$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1','--retain-report','actual.json'];r=subprocess.run(cmd);results.append(dict(compiler=compiler,exit_code=r.returncode,command=cmd))
  report.update(passed=all(r['exit_code']==0 for r in results)and all(sha(ROOT/f)==v for f,v in pins.items()),results=results,original_sha256=NEW_PIN,archive_sha256=a.archive_sha256,source_body_sites=counts,listing_sha256=hashlib.sha256(''.join(listing).encode()).hexdigest(),scope='Actual02 BEFORE graph using measured06receiver/other; sourcefoundcount5 index4/listeners0; no actual06entry claimed. Declared ordinaryRET4 leafABI independent newprivate stack, only9B initializedDATApacket compared; everyactualcapturedbank byte/FP/readstores compared; no GPpostimages/outcomes.')
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
