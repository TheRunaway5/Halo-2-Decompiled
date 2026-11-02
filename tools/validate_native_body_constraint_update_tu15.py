#!/usr/bin/env python3
"""Compare native22D460 with actual original read/FP/state boundaries.

The retained inputs have zero component constraint rows. This qualifies their
real native gating/model-read paths, not the unexercised positive creation loop.
"""
import argparse,hashlib,json,os,re,shutil,subprocess,sys,tarfile
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
from validate_native_contact_agent_dispatch_tu15 import sha,save
from build_native_tu15_before_reference import build_before_reference
ARCHIVE=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-contact-input-original-v1461/iteration-00-contact-time-before-vehicle-camera/required-contact-time-before/actual-before-input.tar.zst'
ARCHIVE_SHA='3f87636519456e84f8d8bf6cc3754bda002f7dfb42b361bd65fd803c7c4b7185'

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--compilers',nargs='+',default=['g++','clang++']);p.add_argument('--case',default='.*');p.add_argument('--leaf',choices=['parent','frame','distance','lookup','capacity','slots','motion','cache','release'],default='parent');a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 source=ROOT/'src/physics/native_tu15_body_constraint_update.cpp';header=ROOT/'include/halo2/native_tu15_body_constraint_update.hpp';allocator=ROOT/'src/core/native_tu15_data_array_allocate.cpp';observer=ROOT/('tools/observe_native_body_constraint_update_tu15.cpp'if a.leaf=='parent'else'tools/observe_native_body_constraint_leaves_tu15.cpp');geometry=ROOT/'src/physics/native_tu15_geometry.cpp';lookup=ROOT/'src/physics/native_tu15_model_constraint_lookup.cpp';capacity=ROOT/'src/physics/native_tu15_model_constraint_capacity.cpp';slots=ROOT/'src/physics/native_tu15_constraint_body_slots.cpp';motion=ROOT/'src/physics/native_tu15_constraint_motion_compare.cpp';cache=ROOT/'src/physics/native_tu15_constraint_scalar_cache.cpp';release=ROOT/'src/physics/native_tu15_constraint_reference_release.cpp';files=[source,header,allocator,geometry,lookup,capacity,slots,motion,cache,release,ROOT/'include/halo2/native_tu15_constraint_reference_release.hpp',ROOT/'include/halo2/native_tu15_constraint_scalar_cache.hpp',ROOT/'include/halo2/native_tu15_constraint_motion_compare.hpp',ROOT/'include/halo2/native_tu15_constraint_body_slots.hpp',ROOT/'include/halo2/native_tu15_model_constraint_capacity.hpp',ROOT/'include/halo2/native_tu15_model_constraint_lookup.hpp',ROOT/'include/halo2/native_tu15_geometry.hpp',ROOT/'include/halo2/native_tu15_data_array_allocate.hpp',ROOT/'include/halo2/native_tu15_physics_frame.hpp',ROOT/'tools/observe_native_activation_agents_tu15.cpp',observer,ROOT/'tools/build_native_tu15_before_reference.py',Path(__file__)];pins={str(f.relative_to(ROOT)):sha(f)for f in files};save(a.output/'report.json',report)
 try:
  from capstone import CS_AC_READ,CS_AC_WRITE
  from capstone.x86 import X86_OP_MEM
  assert sha(NEW_DEFAULT)==NEW_PIN;code=_Code(Xbe(NEW_DEFAULT),'.text');metadata={};counts={};hashes={}
  for owner in((0x22d460,0x1d9ec0,0x315c90)if a.leaf=='parent'else(0x1d8b90,0x2e4650,0x2e44d0)if a.leaf=='frame'else(0x230f60,)if a.leaf=='distance'else(0x1dd710,0x22def0,0x22e040,0x22e000,0x22e0d0)if a.leaf=='lookup'else(0x1dd7b0,0x22dd90)if a.leaf=='capacity'else(0x22e390,0xde8c0)if a.leaf=='slots'else(0x22ede0,)if a.leaf=='motion'else(0x22fe30,)if a.leaf=='cache'else(0x22e1b0,0x22e390,0xde8c0,0x22fe30)):
   body=complete_body(code,owner);counts[hex(owner)]=len(body.code);listing=[]
   for pc in sorted(body.code):
    i=code.at(pc);listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');operands=[]
    if i.mnemonic!='lea':
     for op in i.operands:
      if op.type!=X86_OP_MEM or op.mem.segment:continue
      operands.append(dict(base=i.reg_name(op.mem.base)or'',index=i.reg_name(op.mem.index)or'',scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(op.access&CS_AC_READ)or i.mnemonic=='test',write=bool(op.access&CS_AC_WRITE)))
    metadata[str(pc)]=dict(bytes=i.bytes.hex(),size=i.size,memory=operands)
   hashes[hex(owner)]=hashlib.sha256(''.join(listing).encode()).hexdigest()
  if a.leaf=='release':metadata[str(0x22e1fc)]['stop_before']=True
  instruction_file=scratch/'instructions.json';save(instruction_file,metadata);driver=scratch/'driver.cpp';prefix=(ROOT/'tools/observe_native_activation_agents_tu15.cpp').read_text().split('int main(int argc,char**argv)')[0];prefix=prefix.replace('const auto&i=metadata.at(key);','const auto&i=metadata.at(key);if(i.value("stop_before",false))break;');driver.write_text('#define CONSTRAINT_LEAF '+str({'parent':0,'frame':1,'distance':2,'lookup':3,'capacity':4,'slots':5,'motion':6,'cache':7,'release':8}[a.leaf])+'\n#include "halo2/native_tu15_constraint_reference_release.hpp"\n#include "halo2/native_tu15_constraint_scalar_cache.hpp"\n#include "halo2/native_tu15_constraint_motion_compare.hpp"\n#include "halo2/native_tu15_constraint_body_slots.hpp"\n#include "halo2/native_tu15_model_constraint_capacity.hpp"\n#include "halo2/native_tu15_model_constraint_lookup.hpp"\n#include "halo2/native_tu15_body_constraint_update.hpp"\n'+prefix+observer.read_text());contexts=[];cases=[];base=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458'
  for name in('gemini-column0','gemini-column1','elongation-crates','actual-warthog'):
   selections=range(5)if name=='actual-warthog'else range(3)if name=='elongation-crates'else(0,1,*range(256,269))
   if not any(re.fullmatch(a.case,f'{name}_{selection}_{mode}')for selection in selections for mode in range(4)):continue
   archive=None
   if name=='actual-warthog':
    assert sha(ARCHIVE)==ARCHIVE_SHA;capture=scratch/'actual-warthog-inputs';capture.mkdir()
    with tarfile.open(ARCHIVE,'r:*')as stream:
     for member in stream.getmembers():
      target=(capture/member.name).resolve();assert target==capture or target.is_relative_to(capture);assert member.isfile()or member.isdir()
     stream.extractall(capture,filter='data')
    archive=dict(path=str(ARCHIVE),sha256=ARCHIVE_SHA)
   else:capture=base/('iteration-04-'+name)/'original/required-failure-inputs'
   loader=build_before_reference(capture,scratch/('reference-'+name));contexts.append(dict(name=name,capture_metadata_sha256=sha(capture/'capture.json'),archive=archive,loader=loader))
   for selection in selections:
    for mode in range(4):
     if re.fullmatch(a.case,f'{name}_{selection}_{mode}'):cases.append(dict(name=f'{name}_{selection}_{mode}',elf=loader['elf'],capture=str(capture/'capture.json'),selection=selection,mode=mode))
  assert cases;project=scratch/'project';project.mkdir();shutil.copyfile(source,project/'native.cpp');shutil.copyfile(allocator,project/'allocator.cpp');shutil.copyfile(geometry,project/'geometry.cpp');shutil.copyfile(lookup,project/'lookup.cpp');shutil.copyfile(capacity,project/'capacity.cpp');shutil.copyfile(slots,project/'slots.cpp');shutil.copyfile(motion,project/'motion.cpp');shutil.copyfile(cache,project/'cache.cpp');shutil.copyfile(release,project/'release.cpp');text=source.read_text();needle='word(component+4u)&0x800u';assert text.count(needle)==1;variants=({'wrong_flag':text.replace(needle,'word(component+4u)&0x400u'),'wrong_activity_count':text.replace('component.identity+0x74u','component.identity+0x80u')}if a.leaf=='parent'else{'wrong_motion':text.replace('columns[0]=vector(memory,motion+0x80u)','columns[0]=vector(memory,motion+0x90u)')}if a.leaf=='frame'else{'wrong_bitmap':text.replace('read(memory,pool+0x48u)','read(memory,pool+0x44u)')}if a.leaf=='distance'else{'wrong_lookup_read_owner':lookup.read_text().replace('const auto datum=word(component.identity+0x20u)','(void)word(component.identity+4u);const auto datum=word(component.identity+0x20u)')}if a.leaf=='lookup'else{'wrong_capacity_read_owner':capacity.read_text().replace('const auto list=word(component.identity+0x20u)','(void)word(component.identity+4u);const auto list=word(component.identity+0x20u)')}if a.leaf=='capacity'else{'wrong_slots_read_owner':slots.read_text().replace('const auto first=word(m,instance+0x24u)','(void)word(m,instance+0x24u);const auto first=word(m,instance+0x24u)')}if a.leaf=='slots'else{'wrong_motion_read_owner':motion.read_text().replace('if(stats)++stats->calls;if(second==0xffffffffu)','if(stats)++stats->calls;(void)word(m,0x52be5cu);if(second==0xffffffffu)')}if a.leaf=='motion'else{'wrong_cache_read_owner':cache.read_text().replace('if(stats)++stats->calls;Environment environment(fp,depth)','if(stats)++stats->calls;(void)read(memory,instance+0x11u,1);Environment environment(fp,depth)')}if a.leaf=='cache'else{'wrong_reference_decrement':release.read_text().replace('word(memory,instance+8u,2)-1u','word(memory,instance+8u,2)-2u')})
  for name,value in variants.items():(project/(name+'.cpp')).write_text(value)
  (project/'negative.py').write_text('import json,subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nj=json.load(open(sys.argv[-1]));assert r.returncode!=0 and not j.get("error")and any(not c["ordered_operands_equal"]for c in j["cases"]),(r.returncode,j)\nprint("Actual original22D460 rejects incorrect native gate/read ownership")\n')
  negative=next((c for c in cases if c['selection']==0 and c['mode']==0),None)
  if negative is None:variants={}
  cmake=f'''cmake_minimum_required(VERSION 3.20)
project(NativeBodyConstraintUpdate LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
add_library(driver OBJECT "{driver}")
target_include_directories(driver PRIVATE "{ROOT}/include")
target_compile_features(driver PRIVATE cxx_std_20)
target_link_libraries(driver PRIVATE nlohmann_json::nlohmann_json)
file(MAKE_DIRECTORY "${{CMAKE_BINARY_DIR}}/evidence")
''';retained=[]
  for variant in('native',*variants):
   filename=('native.cpp lookup.cpp capacity.cpp slots.cpp motion.cpp cache.cpp '+('release.cpp'if variant=='native'else variant+'.cpp'))if a.leaf=='release'else(('native.cpp lookup.cpp capacity.cpp slots.cpp motion.cpp '+('cache.cpp'if variant=='native'else variant+'.cpp'))if a.leaf=='cache'else(('native.cpp lookup.cpp capacity.cpp slots.cpp '+('motion.cpp'if variant=='native'else variant+'.cpp'))if a.leaf=='motion'else('native.cpp '+('lookup.cpp'if variant=='native'else variant+'.cpp')+' capacity.cpp slots.cpp motion.cpp')if a.leaf=='lookup'else('native.cpp lookup.cpp '+('capacity.cpp'if variant=='native'else variant+'.cpp')+' slots.cpp motion.cpp')if a.leaf=='capacity'else('native.cpp lookup.cpp capacity.cpp '+('slots.cpp'if variant=='native'else variant+'.cpp')+' motion.cpp')if a.leaf=='slots'else('native.cpp'if variant=='native'else variant+'.cpp')+' lookup.cpp capacity.cpp slots.cpp motion.cpp')+ ' cache.cpp')+ ' release.cpp';cmake+=f'add_executable(constraint_{variant} $<TARGET_OBJECTS:driver> {filename} allocator.cpp geometry.cpp)\ntarget_include_directories(constraint_{variant} PRIVATE "{ROOT}/include")\ntarget_compile_features(constraint_{variant} PRIVATE cxx_std_20)\ntarget_compile_options(constraint_{variant} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
   for case in cases if variant=='native'else[negative]:
    name=variant+'_'+case['name'];output='evidence/'+name+'.json';retained.append(output);command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '+(''if variant=='native'else f'"{sys.executable}" "{project}/negative.py" ')+f'$<TARGET_FILE:constraint_{variant}> "{case["elf"]}" "{case["capture"]}" {case["selection"]} {case["mode"]} "{instruction_file}" "${{CMAKE_BINARY_DIR}}/{output}"';cmake+=f'add_test(NAME constraint_{name} COMMAND {command})\nset_tests_properties(constraint_{name} PROPERTIES TIMEOUT 60)\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in a.compilers:
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','constraint_native',*sum((['--target','constraint_'+name]for name in variants),[]),'--filter','^constraint_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1',*sum((['--retain-report',path]for path in retained),[])];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  unchanged=all(sha(ROOT/f)==v for f,v in pins.items());report.update(passed=all(r['exit_code']==0 for r in results)and unchanged,contexts=contexts,compiler_results=results,physical_island_cases_per_compiler=len(cases),negative_controls=list(variants),source_hashes=pins,current_sources_unchanged=unchanged,body_sites=counts,listing_sha256=hashes,original_sha256=NEW_PIN,captured_after_consumed=False,instruction_patches=[],native_parent_falls_back=False,positive_loop_qualified=False,leaf=a.leaf,remaining=['Real positive constraint-row input and complete pose/find/measure/admission/create native closure','Fault/alias/mutable-callback domains','Production selection and visible multiplayer'])
 except BaseException as error:report['error']=str(error);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
