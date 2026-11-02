#!/usr/bin/env python3
"""Native1F7C10 surface-key composition with genuine MCC operand histories."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def save(p,j):p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')
def metadata():
 image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');rows={};listings={};counts=[]
 argument_stores={0x1f7c4e,0x1f7c4f,0x1f7c55,0x1f7c57,0x1f7c58,0x1f7d32,0x1f67c4,0x1f67c9,0x1f67d0,0x1f67d1,0x1f67d3}
 for owner in(0x1f7c10,0x1f76a0,0x186e70,0x1f67c0,0x1f5fe0):
  body=complete_body(code,owner);counts.append(len(body.code));listing=[]
  for pc in sorted(body.code):
   i=code.at(pc);listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');ops=[]
   for op in i.operands:
    #1F76BA is the authenticated immutable code dispatch table. Native
    # code compiles its five fixed targets into a switch, not a data read.
    if op.type!=X86_OP_MEM or i.mnemonic=='lea'or pc==0x1f76ba:continue
    assert not op.mem.segment
    access=CS_AC_READ if i.mnemonic in('test','cmp')else op.access
    assert access&(CS_AC_READ|CS_AC_WRITE),(hex(pc),i.mnemonic,i.op_str)
    ops.append(dict(base=i.reg_name(op.mem.base)or'',index=i.reg_name(op.mem.index)or'',scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(access&CS_AC_READ),write=bool(access&CS_AC_WRITE)))
   rows[str(pc)]=dict(memory=ops,argument_store=pc in argument_stores)
  listings[hex(owner)]=hashlib.sha256(''.join(listing).encode()).hexdigest()
 assert counts==[129,82,27,14,123]
 return rows,dict(body_sites=counts,listing_sha256=listings,immutable_dispatch_table_sha256=hashlib.sha256(image.read(0x1f77ac,20)).hexdigest(),data_packet_stores=sorted(argument_stores),private_saved_register_or_return_stores_observed_as_data=False)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);p.add_argument('--context',action='append',choices=['gemini-column0','gemini-column1','elongation-crates']);p.add_argument('--producer',type=int);p.add_argument('--compilers',nargs='+',default=['g++','clang++']);p.add_argument('--variants',nargs='+',type=int,default=[0,1,2]);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],declared_context_selection=a.context,declared_producer_selection=a.producer)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;source=ROOT/'src/physics/native_tu15_collision_component.cpp';header=ROOT/'include/halo2/native_tu15_collision_component.hpp';surface=ROOT/'src/physics/native_tu15_collision_surface_shape.cpp';fragment=ROOT/'tools/observe_native_collision_component_tu15.cpp';bridge=ROOT/'tools/observe_native_contact_producer_tu15.cpp';files=[source,header,surface,ROOT/'include/halo2/native_tu15_collision_surface_shape.hpp',fragment,bridge,Path(__file__)];pins={str(p.relative_to(ROOT)):sha(p)for p in files};meta,authority=metadata();instructions=scratch/'instructions.json';save(instructions,meta);project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');shutil.copyfile(source,project/'native.cpp');shutil.copyfile(surface,project/'surface.cpp');driver=project/'observer.cpp';driver.write_text('#include "halo2/native_tu15_collision_component.hpp"\n'+bridge.read_text().split('int main(int argc,char**argv)')[0]+fragment.read_text());probe=scratch/'enumerator';compile=['g++','-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(project/'include'),str(driver),str(project/'native.cpp'),str(project/'surface.cpp'),'-o',str(probe)];r=subprocess.run(compile,capture_output=True,text=True);assert r.returncode==0,r.stderr[-6000:];contexts=[];cases=[];base=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458'
  for name in(a.context or('gemini-column0','gemini-column1','elongation-crates')):
   capture=base/('iteration-04-'+name)/'original/required-failure-inputs';original=scratch/('original-'+name);command=[sys.executable,str(ROOT/'tools/replay_live_phantom_hardware_tu15.py'),'--native-frame-semantics','--return-breakpoints','--capture',str(capture),'--output',str(original)];r=subprocess.run(command,capture_output=True,text=True,timeout=180);assert(original/'original-hardware').exists(),r.stderr[-3000:];cohort=[]
   for producer in([a.producer]if a.producer is not None else range(3 if name=='elongation-crates'else 2)):
    output=a.output/(name+'-producer-'+str(producer)+'-components.json');cmd=[str(probe),str(original/'original-hardware'),str(capture/'capture.json'),str(producer),'list','0',str(instructions),str(output)];e=subprocess.run(cmd,capture_output=True,text=True,timeout=180);assert e.returncode==0,e.stderr[-3000:];entries=json.loads(output.read_text())['components'];assert all(x['kind']in(1,5)for x in entries);cohort.append(dict(producer_ordinal=producer,components=entries,enumeration_report_sha256=sha(output)));cases.extend(dict(name=f'{name}_{producer}_{entry["ordinal"]}_{variant}',producer=producer,component=entry['ordinal'],variant=variant,kind=entry['kind'],elf=original/'original-hardware',capture=capture/'capture.json')for entry in entries for variant in a.variants)
   contexts.append(dict(name=name,capture=str(capture),capture_sha256=sha(capture/'capture.json'),reference_command=command,reference_raw_exit=r.returncode,enumerated_producers=cohort));print(name,sum(len(p['components'])for p in cohort),'genuine surface-key components',flush=True)
  assert cases;variants={};text=source.read_text()
  for name,needle,replacement in(('wrong_category','data+4u,1);m.word(0x1f7c58u','data+4u,0);m.word(0x1f7c58u'),('wrong_resource','data+8u,0xffffffffu','data+8u,0xfffffffeu'),('wrong_placed_resource','scratch.placed_resource,part','scratch.placed_resource,part^1u')):
   assert text.count(needle)==1;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
  selected1=next(c for c in cases if c['kind']==1);selected5=next((c for c in cases if c['kind']==5),None)
  if selected5 is None:variants.pop('wrong_placed_resource')
  (project/'negative.py').write_text('import json,subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True);j=json.load(open(sys.argv[-1]));assert r.returncode!=0 and not j.get("error") and not j["passed"] and not (j["diagnostic_persistent_output_equal"] and j["diagnostic_all_read_and_written_values_equal"]),(r.returncode,j)\nprint("Unchanged original resolver rejects native semantic mutation beyond baseline prewrite RED")\n')
  cmake='''cmake_minimum_required(VERSION 3.20)
project(native_collision_component LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
add_library(driver OBJECT observer.cpp)
add_library(dependencies OBJECT surface.cpp)
foreach(target driver dependencies)
 target_include_directories(${target} PRIVATE include)
 target_compile_features(${target} PRIVATE cxx_std_20)
 target_compile_options(${target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
endforeach()
target_link_libraries(driver PRIVATE nlohmann_json::nlohmann_json)
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/evidence")
''';retain=[]
  for mode in('native',*variants):
   cmake+=f'add_executable(component_{mode} $<TARGET_OBJECTS:driver> $<TARGET_OBJECTS:dependencies> {mode}.cpp)\ntarget_include_directories(component_{mode} PRIVATE include)\ntarget_compile_features(component_{mode} PRIVATE cxx_std_20)\ntarget_compile_options(component_{mode} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
   for c in(cases if mode=='native'else[selected5 if mode=='wrong_placed_resource'else selected1]):
    relative='evidence/'+mode+'_'+c['name']+'.json';retain.append(relative);negative=''if mode=='native'else f'"{sys.executable}" "{project}/negative.py" ';cmake+=f'add_test(NAME component_{mode}_{c["name"]} COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- {negative}$<TARGET_FILE:component_{mode}> "{c["elf"]}" "{c["capture"]}" {c["producer"]} {c["component"]} {c["variant"]} "{instructions}" "${{CMAKE_BINARY_DIR}}/{relative}")\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in a.compilers:
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','component_native',*sum((['--target','component_'+name]for name in variants),[]),'--filter','^component_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1',*sum((['--retain-report',p]for p in retain),[])];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  unchanged=all(sha(ROOT/p)==pin for p,pin in pins.items());report.update(passed=all(r['exit_code']==0 for r in results)and unchanged,compiler_results=results,contexts=contexts,source_hashes=pins,sources_unchanged=unchanged,original_sha256=NEW_PIN,authority=authority,original_cases_per_compiler=len(cases),placed_input_cases=sum(c['kind']==5 for c in cases),negative_controls=list(variants),declared_variants=a.variants,supplied_original_outcomes=False,captured_after_consumed=False,control_provenance_emitted=False,production_native_only_closure=False,remaining=['Shared/object resolver kinds2/3/4','private control-to-data constructor lifetime','prefix/alias/fault proof','full304350 and visible multiplayer integration'])
 except BaseException as error:report['error']=str(error)or repr(error);report['error_type']=type(error).__name__;raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
