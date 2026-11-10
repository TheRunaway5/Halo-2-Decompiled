#!/usr/bin/env python3
"""Native30D3F0 triangle orientation at genuine simplex helper entries."""
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
 code=_Code(Xbe(NEW_DEFAULT),'.text');rows={};listings={};counts=[]
 for owner in(0x30d3f0,):
  body=complete_body(code,owner);counts.append(len(body.code));listing=[]
  for pc in sorted(body.code):
   i=code.at(pc);listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');ops=[]
   for op in i.operands:
    if op.type!=X86_OP_MEM or i.mnemonic=='lea':continue
    assert not op.mem.segment
    access=CS_AC_READ if i.mnemonic in('test','cmp')else CS_AC_WRITE if i.mnemonic in('fst','fstp')else op.access
    assert access&(CS_AC_READ|CS_AC_WRITE),(hex(pc),i.mnemonic,i.op_str)
    ops.append(dict(base=i.reg_name(op.mem.base)or'',index=i.reg_name(op.mem.index)or'',scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(access&CS_AC_READ),write=bool(access&CS_AC_WRITE)))
   rows[str(pc)]=dict(memory=ops)
  listings[hex(owner)]=hashlib.sha256(''.join(listing).encode()).hexdigest()
 assert counts==[137]
 return rows,dict(body_sites=counts,listing_sha256=listings)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);p.add_argument('--native-source',type=Path,help='Private candidate DATA owner; default production source is unchanged');p.add_argument('--context',action='append',choices=['gemini-column0','gemini-column1','elongation-crates']);p.add_argument('--producer',type=int);p.add_argument('--compilers',nargs='+',default=['g++','clang++']);p.add_argument('--variants',nargs='+',type=int,default=list(range(8)));a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;source=a.native_source.resolve()if a.native_source is not None else ROOT/'src/physics/native_tu15_simplex_triangle.cpp';header=ROOT/'include/halo2/native_tu15_simplex_triangle.hpp';fragment=ROOT/'tools/observe_native_simplex_triangle_tu15.cpp';bridge=ROOT/'tools/observe_native_contact_producer_tu15.cpp';files=[source,header,fragment,bridge,Path(__file__)];pins={str(p.relative_to(ROOT)):sha(p)for p in files};shared='native_simplex_triangle_value_detail::View'in source.read_text();kernel=ROOT/'include/halo2/native_tu15_simplex_triangle_kernel.hpp';value=ROOT/'include/halo2/native_tu15_simplex_triangle_value.hpp';
  if shared:pins.update({str(h.relative_to(ROOT)):sha(h)for h in(kernel,value,ROOT/'include/halo2/native_tu15_simplex_direction_kernel.hpp',ROOT/'include/halo2/native_tu15_simplex_initialize_kernel.hpp')})
  meta,authority=metadata();instructions=scratch/'instructions.json';save(instructions,meta);project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');shutil.copyfile(source,project/'native.cpp');(project/'observer.cpp').write_text('#include "halo2/native_tu15_simplex_triangle.hpp"\n'+bridge.read_text().split('int main(int argc,char**argv)')[0]+fragment.read_text());contexts=[];cases=[];base=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458'
  inventory_path=ROOT/'analysis/physics/native-simplex-dependency-inventory-tu15-20261101.json';inventory=json.loads(inventory_path.read_text());assert inventory['0']['passed'];admitted=set()
  for row in inventory['0']['cases']:
   if row['passed'] and row['entered'].get('0x30d3f0'):
    fields=Path(row['case']).stem.split('_');admitted.add((fields[1],int(fields[2])))
  report['actual_entry_inventory_sha256']=sha(inventory_path)
  for name in(a.context or('gemini-column0','gemini-column1','elongation-crates')):
   capture=base/('iteration-04-'+name)/'original/required-failure-inputs';original=scratch/('original-'+name);command=[sys.executable,str(ROOT/'tools/replay_live_phantom_hardware_tu15.py'),'--native-frame-semantics','--return-breakpoints','--capture',str(capture),'--output',str(original)];r=subprocess.run(command,capture_output=True,text=True,timeout=180);assert(original/'original-hardware').exists(),r.stderr[-3000:]
   for producer in([a.producer]if a.producer is not None else range(3 if name=='elongation-crates'else 2)):
    if(name,producer)in admitted:cases.extend(dict(name=f'{name}_{producer}_{variant}',producer=producer,variant=variant,elf=original/'original-hardware',capture=capture/'capture.json')for variant in a.variants)
   contexts.append(dict(name=name,capture=str(capture),capture_sha256=sha(capture/'capture.json'),reference_command=command,reference_raw_exit=r.returncode));print(name,'triangle source inputs ready',flush=True)
  text=source.read_text();variants={}
  assert cases,'No genuine triangle-helper entries in requested source cohort'
  changes=(('wrong_weight_sign','d.weights=add(add(d.x_term,d.y_term),d.z_term);','d.weights=N::subtract(add(d.x_term,d.y_term),d.z_term);'),('skip_orientation','if(!(_mm_movemask_ps(d.dot)&1u))','if(true)'),('wrong_mask','std::uint32_t(_mm_movemask_ps(d.weights))&7u','std::uint32_t(_mm_movemask_ps(d.weights))&3u'))if shared else(('wrong_weight_sign','return add(add(xterm,yterm),zterm);','return subtract(add(xterm,yterm),zterm);'),('skip_orientation','if(!(_mm_movemask_ps(dot)&1u))return mask;','return mask;'),('wrong_mask','std::uint32_t(_mm_movemask_ps(weights))&7u','std::uint32_t(_mm_movemask_ps(weights))&3u'))
  mutation_source=kernel.read_text()if shared else text
  for name,needle,replacement in changes:
   assert mutation_source.count(needle)==1;variants[name]=mutation_source.replace(needle,replacement)
   (project/(name+'.cpp')).write_text(text if shared else variants[name])
   if shared:
    override=project/'mutants'/name/'halo2';override.mkdir(parents=True);(override/kernel.name).write_text(variants[name])
  (project/'negative.py').write_text('import json,subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True);j=json.load(open(sys.argv[-1]));assert r.returncode!=0 and not j.get("error") and not j["passed"],(r.returncode,j)\nprint("Unchanged original triangle rejects native mutation")\n')
  cmake='''cmake_minimum_required(VERSION 3.20)
project(native_simplex_triangle LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
add_library(driver OBJECT observer.cpp)
target_include_directories(driver PRIVATE include)
target_compile_features(driver PRIVATE cxx_std_20)
target_compile_options(driver PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(driver PRIVATE nlohmann_json::nlohmann_json)
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/evidence")
''';retain=[]
  for mode in('native',*variants):
   cmake+=f'add_executable(triangle_{mode} $<TARGET_OBJECTS:driver> {mode}.cpp)\ntarget_include_directories(triangle_{mode} PRIVATE include)\ntarget_compile_features(triangle_{mode} PRIVATE cxx_std_20)\ntarget_compile_options(triangle_{mode} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
   if shared and mode!='native':cmake+=f'target_include_directories(triangle_{mode} BEFORE PRIVATE mutants/{mode})\n'
   selected=next((c for c in cases if c['variant']==1),cases[0])
   for c in(cases if mode=='native'else[selected]):
    relative='evidence/'+mode+'_'+c['name']+'.json';retain.append(relative);negative=''if mode=='native'else f'"{sys.executable}" "{project}/negative.py" ';cmake+=f'add_test(NAME triangle_{mode}_{c["name"]} COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- {negative}$<TARGET_FILE:triangle_{mode}> "{c["elf"]}" "{c["capture"]}" {c["producer"]} {c["variant"]} "{instructions}" "${{CMAKE_BINARY_DIR}}/{relative}")\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in a.compilers:
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','triangle_native',*sum((['--target','triangle_'+name]for name in variants),[]),'--filter','^triangle_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1',*sum((['--retain-report',p]for p in retain),[])];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  unchanged=all(sha(ROOT/p)==pin for p,pin in pins.items());report.update(passed=all(r['exit_code']==0 for r in results)and unchanged,compiler_results=results,contexts=contexts,source_hashes=pins,sources_unchanged=unchanged,original_sha256=NEW_PIN,authority=authority,original_cases_per_compiler=len(cases),negative_controls=list(variants),declared_variants=a.variants,supplied_original_outcomes=False,captured_after_consumed=False,production_native_only_closure=False,shared_typed_core=shared,private_candidate_source=str(source)if a.native_source is not None else None,remaining=['source aliases and fault/observer prefixes','30E930 semantic parent and remaining helpers','wholequery/304350 and visible multiplayer integration'])
 except BaseException as error:report['error']=str(error)or repr(error);report['error_type']=type(error).__name__;raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
