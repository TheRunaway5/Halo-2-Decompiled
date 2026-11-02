#!/usr/bin/env python3
"""Qualify native 281F10/1D7530 with actual captured authored contact pairs."""
import argparse,hashlib,json,os,re,shutil,subprocess,sys
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
from validate_native_contact_agent_dispatch_tu15 import sha,save

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--compilers',nargs='+',default=['g++','clang++']);p.add_argument('--case',default='.*');p.add_argument('--property',action='store_true');a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 units=['activation_payload_filter','activation_services'];observer=ROOT/('tools/observe_native_activation_body_datum_tu15.cpp'if a.property else'tools/observe_native_activation_payload_filter_tu15.cpp');files=[ROOT/f'src/physics/native_tu15_{name}.cpp'for name in units]+[ROOT/f'include/halo2/native_tu15_{name}.hpp'for name in units]+[ROOT/'tools/observe_native_activation_agents_tu15.cpp',observer,Path(__file__)];pins={str(f.relative_to(ROOT)):sha(f)for f in files};save(a.output/'report.json',report)
 try:
  from capstone import CS_AC_READ,CS_AC_WRITE
  from capstone.x86 import X86_OP_MEM
  assert sha(NEW_DEFAULT)==NEW_PIN;code=_Code(Xbe(NEW_DEFAULT),'.text');metadata={};counts={};hashes={}
  for owner in(0x281f10,0x1d7530,0xadb50,0xad1d0,0x2e8cf0,0x31dd70,0x31edf0,0xad740,0x31d520):
   body=complete_body(code,owner);counts[hex(owner)]=len(body.code);listing=[]
   for pc in sorted(body.code):
    i=code.at(pc);listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');operands=[]
    if i.mnemonic!='lea':
     for op in i.operands:
      if op.type!=X86_OP_MEM or op.mem.segment:continue
      operands.append(dict(base=i.reg_name(op.mem.base)or'',index=i.reg_name(op.mem.index)or'',scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(op.access&CS_AC_READ),write=bool(op.access&CS_AC_WRITE)))
    metadata[str(pc)]=dict(bytes=i.bytes.hex(),size=i.size,memory=operands)
   hashes[hex(owner)]=hashlib.sha256(''.join(listing).encode()).hexdigest()
  instruction_file=scratch/'instructions.json';save(instruction_file,metadata);prefix=(ROOT/'tools/observe_native_activation_agents_tu15.cpp').read_text().split('int main(int argc,char**argv)')[0];driver=scratch/'driver.cpp';driver.write_text('#include "halo2/native_tu15_activation_payload_filter.hpp"\n'+prefix+observer.read_text());base=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458';contexts=[];cases=[]
  for name in('gemini-column0','gemini-column1','elongation-crates'):
   selections=range(3)if name=='elongation-crates'else((0,1,*range(256,269))if a.property else(0,1,268));variations=range(1)if a.property else range(8)
   if not any(re.fullmatch(a.case,f'{name}_{selection}_{variation}')for selection in selections for variation in variations):continue
   capture=base/('iteration-04-'+name)/'original/required-failure-inputs';replay=scratch/('original-'+name);command=[sys.executable,str(ROOT/'tools/replay_live_phantom_hardware_tu15.py'),'--native-frame-semantics','--return-breakpoints','--capture',str(capture),'--output',str(replay)];r=subprocess.run(command,capture_output=True,text=True,timeout=180);assert(replay/'original-hardware').exists(),r.stderr[-2000:];contexts.append(dict(name=name,capture=str(capture),capture_sha256=sha(capture/'capture.json'),reference_command=command,ordinary_full_frame_reference_exit=r.returncode))
   for selection in selections:
    for variation in variations:
     if re.fullmatch(a.case,f'{name}_{selection}_{variation}'):cases.append(dict(name=f'{name}_{selection}_{variation}',elf=str(replay/'original-hardware'),capture=str(capture/'capture.json'),selection=selection,variation=variation))
  assert cases;project=scratch/'project';project.mkdir()
  for name in units:shutil.copyfile(ROOT/f'src/physics/native_tu15_{name}.cpp',project/(name+'.cpp'))
  text=(project/'activation_payload_filter.cpp').read_text();first='    const auto first_body=word(memory,first.identity+0x18u)==1u?word(memory,first.identity+0x20u):0u;\n';second='    const auto second_body=word(memory,second.identity+0x18u)==1u?word(memory,second.identity+0x20u):0u;\n';assert text.count(first+second)==1;variants={'wrong_read_order':text.replace(first+second,second+first),'wrong_accept':text.replace('return 1;','return 0;')}
  if a.property:
   needle='return word(memory,properties+index*8u+4u);';assert text.count(needle)==1
   variants={'wrong_property_key':text.replace('==0x2001u','==0x2002u'),'wrong_property_value':text.replace(needle,'return word(memory,properties+index*8u+4u)+1u;')}
  for name,source in variants.items():(project/(name+'.cpp')).write_text(source)
  (project/'negative.py').write_text('import json,subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nj=json.load(open(sys.argv[-1]));assert r.returncode!=0 and not j.get("error")and any(not c["result_equal"]or not c["ordered_operands_equal"]for c in j.get("pair_cases",j.get("property_cases",[]))),(r.returncode,j)\nprint("Original actual-input owner rejects the native mutation")\n')
  cmake=f'''cmake_minimum_required(VERSION 3.20)
project(NativeActivationPayload LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
add_library(driver OBJECT "{driver}")
target_include_directories(driver PRIVATE "{ROOT}/include")
target_compile_features(driver PRIVATE cxx_std_20)
target_link_libraries(driver PRIVATE nlohmann_json::nlohmann_json)
file(MAKE_DIRECTORY "${{CMAKE_BINARY_DIR}}/evidence")
''';retained=[]
  for variant in('native',*variants):
   filename='activation_payload_filter.cpp'if variant=='native'else variant+'.cpp';cmake+=f'add_executable(payload_{variant} $<TARGET_OBJECTS:driver> {filename} activation_services.cpp)\ntarget_include_directories(payload_{variant} PRIVATE "{ROOT}/include")\ntarget_compile_features(payload_{variant} PRIVATE cxx_std_20)\n'
   for c in cases if variant=='native'else[cases[0]]:
    name=variant+'_'+c['name'];output='evidence/'+name+'.json';retained.append(output);command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '+(''if variant=='native'else f'"{sys.executable}" "{project}/negative.py" ')+f'$<TARGET_FILE:payload_{variant}> "{c["elf"]}" "{c["capture"]}" {c["selection"]} {c["variation"]} "{instruction_file}" "${{CMAKE_BINARY_DIR}}/{output}"';cmake+=f'add_test(NAME payload_{name} COMMAND {command})\nset_tests_properties(payload_{name} PROPERTIES TIMEOUT 60)\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in a.compilers:
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','payload_native',*sum((['--target','payload_'+name]for name in variants),[]),'--filter','^payload_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1',*sum((['--retain-report',path]for path in retained),[])];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  unchanged=all(sha(ROOT/f)==v for f,v in pins.items());report.update(passed=all(r['exit_code']==0 for r in results)and unchanged,property_owner=a.property,compiler_results=results,contexts=contexts,physical_island_cases_per_compiler=len(cases),negative_controls=list(variants),body_sites=counts,listing_sha256=hashes,source_hashes=pins,current_sources_unchanged=unchanged,image_sha256=NEW_PIN,instruction_patches=[],supplied_original_outcomes=False,manufactured_contact_pairs=False,production_live_qualification=False)
 except BaseException as error:report['error']=str(error);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
