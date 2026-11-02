#!/usr/bin/env python3
"""Qualify native graph sequence against actual09 allocator owners, typed children."""
import argparse,hashlib,os,shutil,subprocess,sys
from pathlib import Path
from build_native_tu15_constraint_before_reference import BeforeInput,build_constraint_reference,sha,save
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
from validate_native_world_island_merge_stage_tu15 import ARCHIVE,PIN
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--compilers',nargs='+',default=['g++','clang++']);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],remaining_allocator_children_execute_original_CPU=True,declared_graphs=True,captured_parent_ABI=False)
 names=['island_partition_build','island_partition_graph'];files=[*[ROOT/f'src/physics/native_tu15_{n}.cpp'for n in names],*[ROOT/f'include/halo2/native_tu15_{n}.hpp'for n in names],ROOT/'tools/observe_native_island_partition_build_actual_tu15.cpp',ROOT/'tools/observe_native_activation_agents_tu15.cpp',Path(__file__)];pins={str(p.relative_to(ROOT)):sha(p)for p in files}
 try:
  source=BeforeInput(ARCHIVE,PIN,scratch/'before');reference=build_constraint_reference(source,scratch/'reference');reference['entry']='Declared graph build/extract/destruction formals, original actual09 allocator BEFORE backing; no live partition topology or caller captured';save(a.output/'reference.json',reference)
  from capstone import CS_AC_READ,CS_AC_WRITE
  from capstone.x86 import X86_OP_MEM
  assert sha(NEW_DEFAULT)==NEW_PIN;code=_Code(Xbe(NEW_DEFAULT),'.text');metadata={};counts={};hashes={}
  for owner in(0x37c2d0,0x37c460,0x37c3d0,0x37c190,0x37c250):
   body=complete_body(code,owner);listing=[];counts[hex(owner)]=len(body.code)
   for pc in sorted(body.code):
    i=code.at(pc);listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');operands=[]
    if i.mnemonic!='lea':
     for op in i.operands:
      if op.type!=X86_OP_MEM or op.mem.segment:continue
      operands.append(dict(base=i.reg_name(op.mem.base)or'',index=i.reg_name(op.mem.index)or'',scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(op.access&CS_AC_READ),write=bool(op.access&CS_AC_WRITE)))
    row=dict(size=i.size,memory=operands)
    if i.mnemonic=='call':
     target=i.operands[0].imm if i.operands[0].type==2 else None
     if target==0x2e31e0:row.update(service_arguments=3,ecx_consumed=False,result_consumed=False)
     elif target is None:row.update(service_arguments=2 if owner==0x37c460 else 3 if'0x14'in i.op_str else 1,ecx_consumed=True,result_consumed=owner==0x37c460)
    metadata[str(pc)]=row
   hashes[hex(owner)]=hashlib.sha256(''.join(listing).encode()).hexdigest()
  save(scratch/'metadata.json',metadata);project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');prefix=(ROOT/'tools/observe_native_activation_agents_tu15.cpp').read_text().split('int main(int argc,char**argv)')[0];(project/'driver.cpp').write_text('#include "halo2/native_tu15_island_partition_build.hpp"\n'+prefix+(ROOT/'tools/observe_native_island_partition_build_actual_tu15.cpp').read_text());text=(ROOT/'src/physics/native_tu15_island_partition_build.cpp').read_text();(project/'native.cpp').write_text(text);shutil.copyfile(ROOT/'src/physics/native_tu15_island_partition_graph.cpp',project/'graph.cpp')
  variants={'wrong_build_flag':text.replace('write(0x37c3b7u,graph.identity+8u,0);','write(0x37c3b7u,graph.identity+8u,1);'),'wrong_group_size':text.replace('write(0x37c4deu,record+4u,new_size);','write(0x37c4deu,record+4u,new_size+1u);')}
  for name,s in variants.items():assert s!=text;(project/(name+'.cpp')).write_text(s)
  (project/'negative.py').write_text('import json,subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True);j=json.load(open(sys.argv[-1]));assert r.returncode!=0 and not j.get("error") and any(not c["passed"]for c in j["cases"]),(r.returncode,j)\n')
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(actualPartition LANGUAGES CXX)\ninclude(CTest)\nadd_library(driver OBJECT driver.cpp)\nadd_library(graph OBJECT graph.cpp)\n'
  for target in('driver','graph'):cmake+=f'target_include_directories({target} PRIVATE "{project}/include")\ntarget_compile_features({target} PRIVATE cxx_std_20)\n'
  for name in('native',*variants):
   cmake+=f'add_executable(partition_{name} $<TARGET_OBJECTS:driver> $<TARGET_OBJECTS:graph> {name}.cpp)\ntarget_include_directories(partition_{name} PRIVATE "{project}/include")\ntarget_compile_features(partition_{name} PRIVATE cxx_std_20)\n';negative=''if name=='native'else f'"{sys.executable}" "{project}/negative.py" ';cmake+=f'add_test(NAME partition_{name} COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- {negative}$<TARGET_FILE:partition_{name}> "{reference["elf"]}" "{source.path}/capture-native-before.json" "{scratch}/metadata.json" "${{CMAKE_BINARY_DIR}}/{name}.json")\nset_tests_properties(partition_{name} PROPERTIES TIMEOUT 180)\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in a.compilers:
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','partition_native','--target','partition_wrong_build_flag','--target','partition_wrong_group_size','--filter','^partition_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1',*sum((['--retain-report',name+'.json']for name in('native',*variants)),[])];r=subprocess.run(command,check=False);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  report.update(passed=all(r['exit_code']==0 for r in results)and all(sha(ROOT/p)==h for p,h in pins.items()),compiler_results=results,source_hashes=pins,source_sites=counts,listing_sha256=hashes,reference=reference,scope='Complete259-site graph build/group extraction/destruction plus134-site native union/compression; declared topology, actual09 allocator backing and genuine typed original allocator children only',remaining=['Native raw149BF0/149C40 routes and actual allocation/free children','Native actual321EB0 edge collection and temp arena lifetime','Complete482-site3187C0 parent and genuine live active positive repartition'])
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
