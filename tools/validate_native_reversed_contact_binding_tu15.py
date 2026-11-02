#!/usr/bin/env python3
"""Whole public reverse parents with genuine closed original/native children.

Declared no-contact controls are not a positive public collision proof.
"""
import argparse,hashlib,os,shutil,struct,subprocess,sys
from pathlib import Path
from build_native_tu15_constraint_before_reference import build_constraint_reference,sha,save
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
from validate_native_island_regroup_actual_tu15 import HOOKS
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--stage',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],public_positive_contacts_qualified=False)
 units=['reversed_contact_process','wrapped_contact_process','compound_process','geometry'];files=[*[ROOT/f'src/physics/native_tu15_{n}.cpp'for n in units],*[ROOT/f'include/halo2/native_tu15_{n}.hpp'for n in units],ROOT/'tools/observe_native_reversed_contact_binding_tu15.cpp',ROOT/'tools/observe_native_activation_agents_tu15.cpp',Path(__file__)];pins={str(p.relative_to(ROOT)):sha(p)for p in files}
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);assert int.from_bytes(image.read(0x41ccc8+16,4),'little')==0x2f7a50;code=_Code(image,'.text');data=bytearray(0x6000);base=0x1000000
  def put(p,v):struct.pack_into('<I',data,p-base,v)
  put(base+0x200,base+0x1000);put(base+0x204,0x19876543);put(base+0x208,base+0x3000);put(base+0x100c,base+0x1100);put(base+0x500,base+0x600);put(base+0x600,base+0x4000);put(base+0x604,0);put(base+0x608,0x80000008);put(base+0x800,0x41ccc8);put(base+0x900,0x41ccc8)
  for n in range(16):struct.pack_into('<f',data,0x1010+4*n,1.0 if n in(0,5,10,15)else 0.)
  for n in range(24):struct.pack_into('<f',data,0x3000+4*n,(n-11)*.125)
  class Declared:pass
  source=Declared();source.path=scratch/'declared';source.path.mkdir();source.regions=[]
  for index,(address,raw)in enumerate([(base,bytes(data)),(0x41ccc8,image.read(0x41ccc8,24)),(0x461960,image.read(0x461960,16))]):
   name=f'input-{index}.bin';(source.path/name).write_bytes(raw);source.regions.append(dict(address=address,size=len(raw),before_file=name))
  source.receipt=dict(declared_no_contact_inputs=True,original_sha256=NEW_PIN,captured_runtime_inputs=False,source_outcomes_supplied=False);reference=build_constraint_reference(source,scratch/'reference');reference['entry']='CompletePUBLIC2F7480/2F7FB0; declared closed no-contact compound children, ordinary receiver/stack formals only';reference['code_authentication']['scope']='Pinned unchangedTU1.5 text; explicitly declared controlDATA, no capturedruntimeoroutcome';save(a.output/'reference.json',reference);save(scratch/'fixture.json',dict(regions=source.regions,declared_no_contact_inputs=True))
  from capstone import CS_AC_READ,CS_AC_WRITE
  from capstone.x86 import X86_OP_MEM
  metadata={};counts={};listings={}
  for owner in(0x2f7480,0x2f7fb0):
   body=complete_body(code,owner);counts[hex(owner)]=len(body.code);listing=[]
   for pc in sorted(body.code):
    i=code.at(pc);listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');operands=[]
    if i.mnemonic!='lea':
     for op in i.operands:
      if op.type!=X86_OP_MEM or op.mem.segment:continue
      operands.append(dict(base=i.reg_name(op.mem.base)or'',index=i.reg_name(op.mem.index)or'',scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(op.access&CS_AC_READ),write=bool(op.access&CS_AC_WRITE)))
    row=dict(size=i.size,memory=operands)
    if i.mnemonic=='call':row.update(service_arguments=4,ecx_consumed=True,result_consumed=False)
    metadata[str(pc)]=row
   listings[hex(owner)]=hashlib.sha256(''.join(listing).encode()).hexdigest()
  save(scratch/'metadata.json',metadata);project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');binding_header=a.stage.resolve()/'staged/include/halo2/native_tu15_contact_process_binding.hpp';shutil.copyfile(binding_header,project/'include/halo2/native_tu15_contact_process_binding.hpp');pins[str(binding_header.relative_to(ROOT))]=sha(binding_header)
  for unit in units:shutil.copyfile(ROOT/f'src/physics/native_tu15_{unit}.cpp',project/(unit+'.cpp'))
  prefix=HOOKS+(ROOT/'tools/observe_native_activation_agents_tu15.cpp').read_text().split('int main(int argc,char**argv)')[0];prefix=prefix.replace('if(retained(p,b.size()))ledger.push_back','if(record_native_operands&&retained(p,b.size()))ledger.push_back');includes=''.join('#include "halo2/native_tu15_'+n+'.hpp"\n'for n in('reversed_contact_process','wrapped_contact_process','contact_process_binding'));(project/'driver.cpp').write_text(includes+prefix+(ROOT/'tools/observe_native_reversed_contact_binding_tu15.cpp').read_text());text=(project/'reversed_contact_process.cpp').read_text();needle='old_count=m.word(header+4u);';assert text.count(needle)==1;(project/'wrong_old_count.cpp').write_text(text.replace(needle,'old_count=m.word(header+4u)-1u;'));(project/'negative.py').write_text('import json,subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True);j=json.load(open(sys.argv[-1]));assert r.returncode!=0 and not j.get("error")and any(not c["passed"]for c in j["cases"]),(r.returncode,j)\n')
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(reversedPublic LANGUAGES CXX)\ninclude(CTest)\nadd_library(driver OBJECT driver.cpp)\nadd_library(children OBJECT wrapped_contact_process.cpp compound_process.cpp geometry.cpp)\n'
  for target in('driver','children'):cmake+=f'target_include_directories({target} PRIVATE "{project}/include")\ntarget_compile_features({target} PRIVATE cxx_std_20)\ntarget_compile_options({target} PRIVATE -finstrument-functions -fno-fast-math -ffp-contract=off -frounding-math)\n'
  for variant in('native','wrong_old_count'):
   sourcefile='reversed_contact_process.cpp'if variant=='native'else'wrong_old_count.cpp';cmake+=f'add_executable(reversed_{variant} $<TARGET_OBJECTS:driver> $<TARGET_OBJECTS:children> {sourcefile})\ntarget_include_directories(reversed_{variant} PRIVATE "{project}/include")\ntarget_compile_features(reversed_{variant} PRIVATE cxx_std_20)\n';negative=''if variant=='native'else f'"{sys.executable}" "{project}/negative.py" ';cmake+=f'add_test(NAME reversed_{variant} COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- {negative}$<TARGET_FILE:reversed_{variant}> "{reference["elf"]}" "{scratch}/fixture.json" "{scratch}/metadata.json" "${{CMAKE_BINARY_DIR}}/{variant}.json")\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','reversed_native','--target','reversed_wrong_old_count','--filter','^reversed_','--jobs','1','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--retain-report','native.json','--retain-report','wrong_old_count.json'];r=subprocess.run(command,check=False);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  report.update(passed=all(r['exit_code']==0 for r in results)and all(sha(ROOT/p)==h for p,h in pins.items()),source_hashes=pins,compiler_results=results,reference=reference,source_sites=counts,listing_sha256=listings,native_remaining_owners=0,scope='Completepublic2F7480/2F7FB0 with originalreal2F6CA0/2F7A50/nativechildren; declaredno-contact controls. Strictparentoperand/childtypedcontracts/fullDATA/FP/RET/live80. Positivecontacts/suffix/callers remain separate.')
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
