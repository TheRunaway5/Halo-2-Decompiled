#!/usr/bin/env python3
"""Native309060/309150 cache parents with real original shape dependencies."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def save(p,j):p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')
def metadata(native_shapes=False):
 code=_Code(Xbe(NEW_DEFAULT),'.text');rows={};listings={};counts=[]
 for owner in((0x309060,0x309150,0x2e4690,0x2e47b0,0x2e5da0,0x3077c0,0x2e7e10,0x306750,0x2e5db0,0x3077f0,0x1f6610,0x306790,0x2e7d90,0x1cb030)if native_shapes else(0x309060,0x309150,0x2e4690,0x2e47b0)):
  body=complete_body(code,owner);counts.append(len(body.code));listing=[]
  for pc in sorted(body.code):
   i=code.at(pc);listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');ops=[]
   for op in i.operands:
    if op.type!=X86_OP_MEM or i.mnemonic=='lea'or pc in(0x3078a2,0x3078a9):continue
    assert not op.mem.segment
    access=CS_AC_READ if i.mnemonic in('test','cmp')else op.access
    assert access&(CS_AC_READ|CS_AC_WRITE),(hex(pc),i.mnemonic,i.op_str)
    ops.append(dict(base=i.reg_name(op.mem.base)or'',index=i.reg_name(op.mem.index)or'',scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(access&CS_AC_READ),write=bool(access&CS_AC_WRITE)))
   rows[str(pc)]=dict(memory=ops)
  listings[hex(owner)]=hashlib.sha256(''.join(listing).encode()).hexdigest()
 assert counts[:2]==[82,100]
 authority=dict(body_sites=counts,listing_sha256=listings)
 if native_shapes:authority['immutable_support_dispatch_tables_sha256']=hashlib.sha256(Xbe(NEW_DEFAULT).read(0x307988,43)).hexdigest()
 return rows,authority
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);p.add_argument('--context',action='append',choices=['gemini-column0','gemini-column1','elongation-crates']);p.add_argument('--producer',type=int);p.add_argument('--compilers',nargs='+',default=['g++','clang++']);p.add_argument('--native-shapes',action='store_true');p.add_argument('--variants',nargs='+',type=int,default=[0,1,2,3,4]);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;source=ROOT/'src/physics/native_tu15_contact_cache.cpp';header=ROOT/'include/halo2/native_tu15_contact_cache.hpp';geometry=ROOT/'src/physics/native_tu15_geometry.cpp';fragment=ROOT/'tools/observe_native_contact_cache_tu15.cpp';bridge=ROOT/'tools/observe_native_contact_producer_tu15.cpp';files=[source,header,geometry,ROOT/'include/halo2/native_tu15_geometry.hpp',fragment,bridge,Path(__file__)];pins={str(p.relative_to(ROOT)):sha(p)for p in files};meta,authority=metadata();instructions=scratch/'instructions.json';save(instructions,meta);project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');shutil.copyfile(source,project/'native.cpp');shutil.copyfile(geometry,project/'geometry.cpp');(project/'observer.cpp').write_text('#include "halo2/native_tu15_contact_cache.hpp"\n'+bridge.read_text().split('int main(int argc,char**argv)')[0]+fragment.read_text());contexts=[];cases=[];base=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458'
  if a.native_shapes:
   for path in('src/physics/native_tu15_contact_shape.cpp','include/halo2/native_tu15_contact_shape.hpp'):pins[path]=sha(ROOT/path)
   shutil.copyfile(ROOT/'src/physics/native_tu15_contact_shape.cpp',project/'shape.cpp');(project/'observer.cpp').write_text('#include "halo2/native_tu15_contact_cache.hpp"\n#include "halo2/native_tu15_contact_shape.hpp"\n#define HALO2_NATIVE_CACHE_SHAPES 1\n'+bridge.read_text().split('int main(int argc,char**argv)')[0]+fragment.read_text());meta,authority=metadata(True);save(instructions,meta)
  for name in(a.context or('gemini-column0','gemini-column1','elongation-crates')):
   capture=base/('iteration-04-'+name)/'original/required-failure-inputs';original=scratch/('original-'+name);command=[sys.executable,str(ROOT/'tools/replay_live_phantom_hardware_tu15.py'),'--native-frame-semantics','--return-breakpoints','--capture',str(capture),'--output',str(original)];r=subprocess.run(command,capture_output=True,text=True,timeout=180);assert(original/'original-hardware').exists(),r.stderr[-3000:]
   for producer in([a.producer]if a.producer is not None else range(3 if name=='elongation-crates'else 2)):
    for owner in(0x309060,0x309150):
     cases.extend(dict(name=f'{name}_{producer}_{owner:x}_{variant}',producer=producer,owner=owner,variant=variant,elf=original/'original-hardware',capture=capture/'capture.json')for variant in a.variants)
   contexts.append(dict(name=name,capture=str(capture),capture_sha256=sha(capture/'capture.json'),reference_command=command,reference_raw_exit=r.returncode));print(name,'cache source inputs ready',flush=True)
  text=source.read_text();variants={};shape_variants=set()
  for name,needle,replacement in(('wrong_empty_slot','m.word(0x309120u,cache+4u,0xffffu,2)','m.word(0x309120u,cache+4u,0xfffeu,2)'),('wrong_triangle_slot','m.word(0x30916eu,cache+6u,0x20u,2)','m.word(0x30916eu,cache+6u,0x30u,2)'),('wrong_cross_product','subtract(second_product,first_product)','add(second_product,first_product)')):
   assert text.count(needle)==1;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
  if a.native_shapes:
   shape_text=(ROOT/'src/physics/native_tu15_contact_shape.cpp').read_text()
   for name,needle,replacement in(('shape_wrong_SoA_feature','selected|0x3f000000u','(selected^1u)|0x3f000000u'),('shape_wrong_polygon_vertex','shape.identity+(selected+2u)*16u','shape.identity+(selected+3u)*16u'),('shape_wrong_center','m.word(0x3077dau,output+8u,m.word(coordinates+0x20u))','m.word(0x3077dau,output+8u,m.word(coordinates+0x24u))')):
    assert shape_text.count(needle)==1;shape_variants.add(name);variants[name]=text;(project/(name+'.cpp')).write_text(text);(project/(name+'_shape.cpp')).write_text(shape_text.replace(needle,replacement))
  (project/'negative.py').write_text('import json,subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True);j=json.load(open(sys.argv[-1]));assert r.returncode!=0 and not j.get("error") and not j["passed"],(r.returncode,j)\nprint("Unchanged original cache rejects native mutation")\n')
  cmake='''cmake_minimum_required(VERSION 3.20)
project(native_contact_cache LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
add_library(driver OBJECT observer.cpp)
add_library(dependencies OBJECT geometry.cpp)
foreach(target driver dependencies)
 target_include_directories(${target} PRIVATE include)
 target_compile_features(${target} PRIVATE cxx_std_20)
 target_compile_options(${target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
endforeach()
target_link_libraries(driver PRIVATE nlohmann_json::nlohmann_json)
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/evidence")
''';retain=[]
  for mode in('native',*variants):
   shape_source=(mode+'_shape.cpp'if mode in shape_variants else'shape.cpp')if a.native_shapes else''
   cmake+=f'add_executable(cache_{mode} $<TARGET_OBJECTS:driver> $<TARGET_OBJECTS:dependencies> {mode}.cpp {shape_source})\ntarget_include_directories(cache_{mode} PRIVATE include)\ntarget_compile_features(cache_{mode} PRIVATE cxx_std_20)\ntarget_compile_options(cache_{mode} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
   selected=next(c for c in cases if c['owner']==(0x309060 if mode=='wrong_empty_slot'or mode in shape_variants else 0x309150))
   for c in(cases if mode=='native'else[selected]):
    relative='evidence/'+mode+'_'+c['name']+'.json';retain.append(relative);negative=''if mode=='native'else f'"{sys.executable}" "{project}/negative.py" ';cmake+=f'add_test(NAME cache_{mode}_{c["name"]} COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- {negative}$<TARGET_FILE:cache_{mode}> "{c["elf"]}" "{c["capture"]}" {c["producer"]} {hex(c["owner"])} {c["variant"]} "{instructions}" "${{CMAKE_BINARY_DIR}}/{relative}")\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in a.compilers:
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','cache_native',*sum((['--target','cache_'+name]for name in variants),[]),'--filter','^cache_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1',*sum((['--retain-report',p]for p in retain),[])];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  unchanged=all(sha(ROOT/p)==pin for p,pin in pins.items());report.update(passed=all(r['exit_code']==0 for r in results)and unchanged,compiler_results=results,contexts=contexts,source_hashes=pins,sources_unchanged=unchanged,original_sha256=NEW_PIN,authority=authority,original_cases_per_compiler=len(cases),negative_controls=list(variants),declared_variants=a.variants,supplied_original_outcomes=False,captured_after_consumed=False,production_native_only_closure=False,native_shape_closure=a.native_shapes,declared_triangle_inputs='Complete unchanged309150 invoked with authenticated ABI and current first shape/cache/relative transform; second shape receives declared triangle vertices before invocation',remaining=([]if a.native_shapes else['shape center/support/encoding native dependency closure'])+['prefix/alias/fault proof','fullquery and304350 production composition','visible multiplayer integration'])
 except BaseException as error:report['error']=str(error)or repr(error);report['error_type']=type(error).__name__;raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
