#!/usr/bin/env python3
"""Sole production3187C0 native service builder against physical original."""
import argparse,hashlib,os,shutil,subprocess,sys
from pathlib import Path
from build_native_tu15_constraint_before_reference import BeforeInput,build_constraint_reference,sha,save
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
from validate_native_world_island_merge_stage_tu15 import ARCHIVE,PIN
ROOT=Path(__file__).resolve().parents[1]
HOOKS='''static bool record_native_operands=true;
static void*opaque_functions[32]{};static unsigned opaque_function_count{},opaque_function_depth{};
static bool opaque_saved_recording[128]{};
extern "C" __attribute__((no_instrument_function))void __cyg_profile_func_enter(void*function,void*){
 for(unsigned n=0;n<opaque_function_count;++n)if(opaque_functions[n]==function){if(opaque_function_depth>=128)__builtin_trap();opaque_saved_recording[opaque_function_depth++]=record_native_operands;record_native_operands=false;break;}}
extern "C" __attribute__((no_instrument_function))void __cyg_profile_func_exit(void*function,void*){
 for(unsigned n=0;n<opaque_function_count;++n)if(opaque_functions[n]==function){if(!opaque_function_depth)__builtin_trap();record_native_operands=opaque_saved_recording[--opaque_function_depth];break;}}
struct QuietNativeReads{bool previous;QuietNativeReads():previous(record_native_operands){record_native_operands=false;}~QuietNativeReads(){record_native_operands=previous;}};
'''
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--compilers',nargs='+',default=['g++','clang++']);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],native_original_CPU_children=False,genuine_active_positive_partition_claimed=False)
 units=['island_regroup_binding','island_partition_temporary','island_partition_edges','island_partition_graph','island_partition_build','island_partition_metadata','island_partition_allocator_binding','island_construction','island_lifetime','array_allocator','array_growth','array_reserve','raw_allocator','stack_arena','contact_collector_info'];files=[*[ROOT/f'src/physics/native_tu15_{n}.cpp'for n in['island_regroup',*units]],*[ROOT/f'include/halo2/native_tu15_{n}.hpp'for n in['island_regroup',*units]],ROOT/'tools/observe_native_island_regroup_binding_actual_tu15.cpp',ROOT/'tools/observe_native_activation_agents_tu15.cpp',Path(__file__)];pins={str(p.relative_to(ROOT)):sha(p)for p in files}
 try:
  source=BeforeInput(ARCHIVE,PIN,scratch/'before');reference=build_constraint_reference(source,scratch/'reference');reference['entry']='Declared3187C0 ECXworld: actual09 World DATA plus explicit coherent private1/6-body no-contact/no-constraint/no-action split controls';save(a.output/'reference.json',reference)
  from capstone import CS_AC_READ,CS_AC_WRITE
  from capstone.x86 import X86_OP_MEM
  assert sha(NEW_DEFAULT)==NEW_PIN;code=_Code(Xbe(NEW_DEFAULT),'.text');body=complete_body(code,0x3187c0);metadata={};listing=[]
  static_args={0x3177e0:1,0x321eb0:1,0x2eba20:1,0x37c170:2,0x37c2d0:2,0x37c460:1,0x3212c0:1,0x2e3250:2,0x2e31e0:3,0x37c3d0:0,0x31d400:0,0x2eb850:0,0x317870:0}
  virtual_args={0x318821:1,0x31892f:2,0x318a1d:0,0x318ade:1,0x318b9b:3,0x318c22:1,0x318c30:2,0x318c3d:2}
  for pc in sorted(body.code):
   i=code.at(pc);listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');operands=[]
   if i.mnemonic!='lea':
    for op in i.operands:
     if op.type!=X86_OP_MEM or op.mem.segment:continue
     operands.append(dict(base=i.reg_name(op.mem.base)or'',index=i.reg_name(op.mem.index)or'',scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(op.access&CS_AC_READ),write=bool(op.access&CS_AC_WRITE)))
   row=dict(size=i.size,memory=operands)
   if i.mnemonic=='call':
    target=i.operands[0].imm if i.operands[0].type==2 else None;assert target in static_args or pc in virtual_args,(hex(pc),i.op_str)
    row.update(service_arguments=static_args[target]if target is not None else virtual_args[pc],ecx_consumed=target not in(0x2e3250,0x2e31e0,0x31d400),result_consumed=pc in(0x31892f,0x318947,0x318a1d))
   metadata[str(pc)]=row
  save(scratch/'metadata.json',metadata);project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
  for n in units:shutil.copyfile(ROOT/f'src/physics/native_tu15_{n}.cpp',project/(n+'.cpp'))
  prefix=(ROOT/'tools/observe_native_activation_agents_tu15.cpp').read_text().split('int main(int argc,char**argv)')[0];prefix=HOOKS+prefix
  keep='return p<private_first||std::uint64_t(p)+n>private_end;';assert prefix.count(keep)==1;prefix=prefix.replace(keep,'return (p>=0x72001000u&&std::uint64_t(p)+n<=0x72004000u)||p<private_first||std::uint64_t(p)+n>private_end;')
  prefix=prefix.replace('if(retained(p,b.size()))ledger.push_back','if(record_native_operands&&retained(p,b.size()))ledger.push_back');assert prefix.count('if(record_native_operands&&retained(p,b.size()))')==2
  includes=''.join('#include "halo2/native_tu15_'+n+'.hpp"\n'for n in['island_regroup','island_regroup_binding','island_partition_allocator_binding','contact_collector_info','array_allocator','array_reserve']);(project/'driver.cpp').write_text(includes+prefix+(ROOT/'tools/observe_native_island_regroup_binding_actual_tu15.cpp').read_text());text=(ROOT/'src/physics/native_tu15_island_regroup.cpp').read_text();(project/'native.cpp').write_text(text)
  variants={'wrong_partition_clear':text.replace('write(0x318d6eu,island.identity+0x28u,0,1);','write(0x318d6eu,island.identity+0x28u,1,1);'),'wrong_body_index':text.replace('write(0x318a18u,body+0x58u,index_in_group);','write(0x318a18u,body+0x58u,index_in_group+1u);')}
  for n,s in variants.items():assert s!=text;(project/(n+'.cpp')).write_text(s)
  (project/'negative.py').write_text('import json,subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True);j=json.load(open(sys.argv[-1]));assert r.returncode!=0 and not j.get("error") and any(not c["passed"]for c in j["cases"]),(r.returncode,j)\n')
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(actualRegroup LANGUAGES CXX)\ninclude(CTest)\nadd_library(driver OBJECT driver.cpp)\nadd_library(children OBJECT '+' '.join(n+'.cpp'for n in units)+')\n'
  for target in('driver','children'):cmake+=f'target_include_directories({target} PRIVATE "{project}/include")\ntarget_compile_features({target} PRIVATE cxx_std_20)\n'
  cmake+='target_compile_options(children PRIVATE -finstrument-functions -fno-fast-math -ffp-contract=off -frounding-math)\n'
  for name in('native',*variants):
   cmake+=f'add_executable(regroup_{name} $<TARGET_OBJECTS:driver> $<TARGET_OBJECTS:children> {name}.cpp)\ntarget_include_directories(regroup_{name} PRIVATE "{project}/include")\ntarget_compile_features(regroup_{name} PRIVATE cxx_std_20)\ntarget_compile_options(regroup_{name} PRIVATE -finstrument-functions -fno-fast-math -ffp-contract=off -frounding-math)\n';negative=''if name=='native'else f'"{sys.executable}" "{project}/negative.py" ';cmake+=f'add_test(NAME regroup_{name} COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- {negative}$<TARGET_FILE:regroup_{name}> "{reference["elf"]}" "{source.path}/capture-native-before.json" "{scratch}/metadata.json" "${{CMAKE_BINARY_DIR}}/{name}.json")\nset_tests_properties(regroup_{name} PROPERTIES TIMEOUT 180)\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in a.compilers:
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','regroup_native','--target','regroup_wrong_partition_clear','--target','regroup_wrong_body_index','--filter','^regroup_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1',*sum((['--retain-report',n+'.json']for n in('native',*variants)),[])];r=subprocess.run(command,check=False);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  report.update(passed=all(r['exit_code']==0 for r in results)and all(sha(ROOT/p)==h for p,h in pins.items()),source_hashes=pins,compiler_results=results,source_sites=len(body.code),listing_sha256=hashlib.sha256(''.join(listing).encode()).hexdigest(),reference=reference,scope='Whole482-site normal-completion parent with native children: genuine09 World skips plus declared coherent1/6-body no-row split controls; nested child access scopes independently qualified; no actual active-positive/live partition claim',remaining=['Genuine active-positive and live World composition','Positive action/constraint/contact redistribution callbacks','Empty-island and callback fault/unwind/alias domains'])
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
