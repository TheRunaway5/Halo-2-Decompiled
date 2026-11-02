#!/usr/bin/env python3
"""Qualify native282870 and316200 against actual mutable TU1.5 worlds.

Only declared BEFORE inputs change. Every remaining service executes genuine
original instructions against the current mutable bank. No after-state, GP/SSE
postimage, contact result or instruction patch is supplied to the native parent.
"""
import argparse,hashlib,json,os,re,shutil,subprocess,sys
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
from validate_native_contact_agent_dispatch_tu15 import sha,save

def metadata():
 from capstone import CS_AC_READ,CS_AC_WRITE
 from capstone.x86 import X86_OP_MEM
 code=_Code(Xbe(NEW_DEFAULT),'.text');rows={};counts={};hashes={}
 services={0x2828ef:(3,True,True),0x28292d:(3,True,True),0x28294c:(0,True,True),0x282955:(0,True,True),0x28297d:(4,False,True),0x2829d5:(0,True,True),0x2829f1:(0,True,True),0x282a17:(3,True,True),0x282a4e:(0,True,False),0x282a8b:(0,True,True),0x282a9b:(1,True,False)}
 for entry in(0x282870,0x316200):
  body=complete_body(code,entry);counts[hex(entry)]=len(body.code);listing=[]
  for pc in sorted(body.code):
   i=code.at(pc);listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');operands=[]
   if i.mnemonic!='lea':
    for op in i.operands:
     if op.type!=X86_OP_MEM:continue
     assert not op.mem.segment
     operands.append(dict(base=i.reg_name(op.mem.base)or'',index=i.reg_name(op.mem.index)or'',scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(op.access&CS_AC_READ),write=bool(op.access&CS_AC_WRITE)))
   row=dict(bytes=i.bytes.hex(),size=i.size,memory=operands)
   if pc in services:row.update(service_arguments=services[pc][0],ecx_consumed=services[pc][1],result_consumed=services[pc][2])
   rows[str(pc)]=row
  hashes[hex(entry)]=hashlib.sha256(''.join(listing).encode()).hexdigest()
 assert counts=={'0x282870':194,'0x316200':2};return rows,counts,hashes

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--compilers',nargs='+',default=['g++','clang++']);p.add_argument('--case',default='.*');a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 source=ROOT/'src/physics/native_tu15_activation_agents.cpp';header=ROOT/'include/halo2/native_tu15_activation_agents.hpp';observer=ROOT/'tools/observe_native_activation_agents_tu15.cpp';files=[source,header,observer,Path(__file__)];pins={str(f.relative_to(ROOT)):sha(f)for f in files};save(a.output/'report.json',report)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;driver=scratch/'driver.cpp';shutil.copyfile(observer,driver);instructions,counts,hashes=metadata();instruction_file=scratch/'instructions.json';save(instruction_file,instructions)
  base=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458';cases=[];contexts=[]
  for name in('gemini-column0','gemini-column1','elongation-crates'):
   island_ordinals=(0,1,2)if name=='elongation-crates'else(0,1)
   variations=tuple(range(16))+tuple(1+(mode+1)*16 for mode in range(16))
   if not any(re.fullmatch(a.case,f'{name}_{island}_{variation}')for island in island_ordinals for variation in variations):continue
   capture=base/('iteration-04-'+name)/'original/required-failure-inputs';replay=scratch/('original-'+name);command=[sys.executable,str(ROOT/'tools/replay_live_phantom_hardware_tu15.py'),'--native-frame-semantics','--return-breakpoints','--capture',str(capture),'--output',str(replay)];r=subprocess.run(command,capture_output=True,text=True,timeout=180);assert(replay/'original-hardware').exists(),r.stderr[-2000:];contexts.append(dict(name=name,capture=str(capture),capture_sha256=sha(capture/'capture.json'),reference_command=command,ordinary_full_frame_reference_exit=r.returncode))
   for island in island_ordinals:
    for variation in variations if name=='gemini-column0'and island==0 else range(16):
     if re.fullmatch(a.case,f'{name}_{island}_{variation}'):cases.append(dict(name=f'{name}_{island}_{variation}',elf=str(replay/'original-hardware'),capture=str(capture/'capture.json'),island=island,variation=variation))
  project=scratch/'project';project.mkdir();shutil.copyfile(source,project/'native.cpp');variants={};text=source.read_text()
  for name,needle,replacement in(
   ('wrong_factory_slot','(first_class<<5u)+second_class','(first_class<<4u)+second_class'),
   ('wrong_rank','accepted?0u:0xffffffffu','accepted?1u:0xffffffffu'),
   ('wrong_body_policy','if(kind==1u)','if(kind==2u)')):
   assert text.count(needle)==1,name;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
  negative_case=next((c for c in cases if c['variation']==1),None)
  if negative_case is None:variants={}
  (project/'negative.py').write_text('import json,subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nj=json.load(open(sys.argv[-1]));assert r.returncode!=0 and (j.get("error")or j.get("different_bytes",0)>0 or not j.get("ordered_retained_operands_equal",True)or j.get("service_context_differences")),(r.returncode,j)\nprint("Actual original activation rejects mutation")\n')
  cmake=f'''cmake_minimum_required(VERSION 3.20)
project(NativeActivationAgents LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
add_library(driver OBJECT "{driver}")
target_include_directories(driver PRIVATE "{ROOT}/include")
target_compile_features(driver PRIVATE cxx_std_20)
target_compile_options(driver PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(driver PRIVATE nlohmann_json::nlohmann_json)
file(MAKE_DIRECTORY "${{CMAKE_BINARY_DIR}}/evidence")
''';retained=[]
  for variant in('native',*variants):
   cmake+=f'add_executable(activation_{variant} $<TARGET_OBJECTS:driver> {variant}.cpp)\ntarget_include_directories(activation_{variant} PRIVATE "{ROOT}/include")\ntarget_compile_features(activation_{variant} PRIVATE cxx_std_20)\ntarget_compile_options(activation_{variant} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
   for c in cases if variant=='native'else[negative_case]:
    name=variant+'_'+c['name'];output='evidence/'+name+'.json';retained.append(output);command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '+(''if variant=='native'else f'"{sys.executable}" "{project}/negative.py" ')+f'$<TARGET_FILE:activation_{variant}> "{c["elf"]}" "{c["capture"]}" {c["island"]} {c["variation"]} "{instruction_file}" "${{CMAKE_BINARY_DIR}}/{output}"';cmake+=f'add_test(NAME activation_{name} COMMAND {command})\nset_tests_properties(activation_{name} PROPERTIES TIMEOUT 45)\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in a.compilers:
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','activation_native',*sum((['--target','activation_'+name]for name in variants),[]),'--filter','^activation_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1',*sum((['--retain-report',path]for path in retained),[])];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  unchanged=all(sha(ROOT/f)==v for f,v in pins.items());report.update(passed=all(r['exit_code']==0 for r in results)and unchanged,compiler_results=results,contexts=contexts,physical_cases_per_compiler=len(cases),body_sites=counts,listing_sha256=hashes,negative_controls=list(variants),source_hashes=pins,current_sources_unchanged=unchanged,image_sha256=NEW_PIN,supplied_original_outcomes=False,instruction_patches=[],native_parent_receives_guest_register_state=False,production_native_only_closure=False,remaining=['native remaining factory/filter/policy/motion/discrete/destructor service owners','fault/mutable-callback/alias and unmasked platform qualification','actual visible multiplayer activation/wake integration'])
 except BaseException as error:report['error']=str(error);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
