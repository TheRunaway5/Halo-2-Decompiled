#!/usr/bin/env python3
"""Exact canonical BSP bootstrap method, topology, transport, gate and link."""
from pathlib import Path
import argparse,hashlib,json,os,re,shutil,subprocess,sys
ROOT=Path(__file__).resolve().parents[1]
STAGE=ROOT/'analysis/physics/native-bsp-object-association-host-stage-20261102/staged'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);owner=Path(os.environ['HALO2_TEST_SCRATCH']);r=dict(passed=False,owned_scratch=str(owner),retained_large_artifacts=[],scope=__doc__)
 try:
  live=(STAGE/'src/physics/physics_live_world_tu15.cpp').read_text();host=(STAGE/'tests/physics_multiplayer_acceptance.cpp').read_text();header=(STAGE/'include/halo2/native_tu15_physics_frame_residuals.hpp').read_text()
  begin=live.index('    void initialize_native_bsp_object_associations() {');end=live.index('    NativeTu15PhysicsFrameExecutionStats native_physics_frames;',begin);method=live[begin:end]
  begin=live.index('    const auto&bsp_associations=impl_->native_bsp_associations;');end=live.index('    const auto&look_initialization=',begin);getter=live[begin:end]
  fields=re.findall(r'^  std::uint64_t (native_bsp_association_\w+)\{\};$',header,re.M);assert len(fields)==12
  dumps=[re.findall(r'^        \{"'+n+r'",native_frames\.'+n+r'\},$',host,re.M)for n in fields];assert all(len(x)==2 for x in dumps)
  declarations='\n'.join('std::uint64_t '+n+'{};'for n in fields);json_rows='\n'.join('        {"'+n+'",native_frames.'+n+'},'for n in fields)
  begin=host.index('  if(std::getenv("HALO2_REQUIRE_NATIVE_BSP_OBJECT_ASSOCIATION")) {');end=host.index('  if(std::getenv("HALO2_REQUIRE_NATIVE_LOOK_INITIALIZATION")) {',begin);gate=host[begin:end];negative_gates=9 if 'native_frames.native_bsp_association_iterator_calls>=calls'in gate else 7
  assert live.count('initialize_native_bsp_object_associations();')==1 and 'call_actual(0x2190c0u)'not in live
  code=r'''
#include "halo2/native_tu15_bsp_object_association_binding.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <nlohmann/json.hpp>
#include <xmmintrin.h>
using namespace halo2;using Json=nlohmann::json;using FormatError=std::runtime_error;
void check(bool b,const char*s){if(!b)throw std::runtime_error(s);}
static unsigned untracked{};template<class B>void source_execution_untracked(B&,std::uint32_t owner){check(owner==0x2190c0u,"wrong source owner");++untracked;}
template<class B=SourceRuntimeMemory>struct Impl{B bank;SourcePhysicsContactRecordObserver physics_store_observer;bool physics_store_observer_read_only{};unsigned depth{},legacy_calls{};struct Execution{Impl&owner;explicit Execution(Impl&o):owner(o){++owner.depth;}~Execution(){--owner.depth;}};
'''+method+r'''
};
struct Flat{
'''+declarations+r'''
};
template<class B>Flat transport(Impl<B>&owner){const auto*impl_=&owner;Flat result;
'''+getter+r'''
 return result;}
Json report(const Flat&native_frames){return Json{
'''+json_rows+r'''
};}
bool gate(const Flat&native_frames){bool passed=true;Json report;
'''+gate+r'''
 return passed;}
void put(SourceRuntimeMemory&m,std::uint32_t p,std::uint32_t v,unsigned n=4){std::array<std::byte,4>b{};for(unsigned i=0;i<n;++i)b[i]=std::byte(v>>(8*i));m.write(p,std::span(b).first(n));}
unsigned word(const SourceRuntimeMemory&m,std::uint32_t p,unsigned n=4){auto b=m.view(p,n);unsigned v{};for(unsigned i=0;i<n;++i)v|=std::uint32_t(b[i])<<(8*i);return v;}
void fixture(Impl<>&i,bool objects,bool reserve=false){auto&m=i.bank;std::vector<std::byte>b(0x10000,std::byte{0xa5});m.mount(0x10000000,b,SourceRuntimeMemory::Access::ReadWrite,"declared object/scenario records");
 for(auto[p,v]:std::array<std::pair<unsigned,unsigned>,3>{{{0x4ecbd0,0x10003000},{0x4ecc18,0x10001000},{0x505e7c,0x10006000}}}){std::array<std::byte,4>r{};m.mount(p,r,SourceRuntimeMemory::Access::ReadWrite,"declared source global");put(m,p,v);}
 put(m,0x10001000+0x220,0x10005000);put(m,0x10005000,objects?1:0);put(m,0x10005004,0x10005100);put(m,0x10001000+0x5c,objects?1:0);put(m,0x10001000+0x60,0x10008000);put(m,0x10008018,8,1);
 put(m,0x10003038,objects?1:0);put(m,0x10003044,0x10004000);put(m,0x10004000,0x07000001);put(m,0x10004008,0x10007000);put(m,0x100070a4,0x12345678);put(m,0x100070a8,0x00030002);put(m,0x10005100,0x12345678);put(m,0x10005104,0x00030002);put(m,0x10005108,0x00090007);
 if(reserve)m.reserve(0x70000000u,4096,4096,"uncommitted retained reservation");
}
struct RefusalBank:SourcePhysicsContactMemory{bool huge{};mutable unsigned touches{},metadata{};std::optional<unsigned>find_unowned_range(unsigned,std::uint64_t,std::size_t,unsigned)const{return huge?std::optional<unsigned>{0x70000000}:std::nullopt;}
 template<class F>void visit_region_metadata(F&&f)const{++metadata;f(SourceRuntimeMemory::RegionMetadata{0,SourceRuntimeMemory::Access::ReadWrite,"declared huge topology",0x100000000ull,0,0});}
 void read(unsigned,std::span<std::byte>)const override{++touches;throw std::runtime_error("unqualified DATA read");}void write(unsigned,std::span<const std::byte>)override{++touches;throw std::runtime_error("unqualified DATA write");}};
std::array<unsigned,3>fp(){std::uint16_t cw,sw;asm volatile("fnstcw %0":"=m"(cw));asm volatile("fnstsw %0":"=a"(sw));return{cw,sw,_mm_getcsr()};}
int main(){try{setenv("HALO2_REQUIRE_NATIVE_BSP_OBJECT_ASSOCIATION","1",1);unsigned models{};
 for(unsigned mode=0;mode<3;++mode){Impl<>i;fixture(i,mode!=0,mode==2);const auto before=fp();std::vector<std::pair<unsigned,std::size_t>>topology;i.bank.visit_region_metadata([&](auto&r){topology.emplace_back(r.address,r.bytes);});
  if(mode==2)check(i.bank.find_unowned_range(0x70000000,0x100000000ull,32,16)==0x70001000,"gap query lost reservation hole");
  const auto marks=untracked;i.initialize_native_bsp_object_associations();check(untracked==marks+1&&i.depth==0&&i.legacy_calls==0,"host source scope/lifetime differs");check(fp()==before,"integer bootstrap changed hardware FP");
  std::vector<std::pair<unsigned,std::size_t>>after;i.bank.visit_region_metadata([&](auto&r){after.emplace_back(r.address,r.bytes);});check(topology==after,"private iterator was mounted/retained");
  auto flat=transport(i);check(gate(flat),"positive native bootstrap gate differs");const auto j=report(flat);check(j.size()==12&&j.at("native_bsp_association_kernel_entries")==1&&j.at("native_bsp_association_calls")==1,"getter/JSON counters differ");
  check(i.native_bsp_associations.iterator_calls==(mode?3u:1u)&&i.native_bsp_associations.reset_objects==unsigned(mode!=0)&&i.native_bsp_associations.matched_objects==unsigned(mode!=0)&&i.native_bsp_associations.excluded_objects==unsigned(mode!=0),"kernel subcounts differ");
  check(word(i.bank,0x10006000)==(mode?0xfffffffeu:0xffffffffu),"source mask differs");if(mode)check(word(i.bank,0x100071da,2)==7&&word(i.bank,0x100071dc,2)==9,"authored BSP object fields differ");++models;
 }
 for(unsigned mode=0;mode<2;++mode){Impl<>i;i.physics_store_observer_read_only=mode==1;unsigned delivered{};i.physics_store_observer=[&](auto&){++delivered;};bool refused{};try{i.initialize_native_bsp_object_associations();}catch(const std::runtime_error&e){refused=std::string(e.what())=="Native BSP association requires absent observer or qualified retained capability";}check(refused&&i.native_bsp_association_dispatches==1&&i.native_bsp_association_refusals==1&&i.native_bsp_association_observer_refusals==1&&i.native_bsp_associations.attempts==0&&i.depth==0&&delivered==0,"observer preDATA refusal differs");++models;}
 for(bool huge:{false,true}){Impl<RefusalBank>i;i.bank.huge=huge;bool refused{};try{i.initialize_native_bsp_object_associations();}catch(const std::runtime_error&){refused=true;}check(refused&&i.native_bsp_association_resource_refusals==1&&i.native_bsp_association_refusals==1&&i.native_bsp_associations.attempts==0&&i.bank.touches==0&&i.depth==0,"resource refusal touched DATA");++models;}
 {Impl<>i;fixture(i,true);put(i.bank,0x505e7c,0x70000000);const auto before=std::vector<std::byte>(i.bank.view(0x10006000,64).begin(),i.bank.view(0x10006000,64).end());bool refused{};try{i.initialize_native_bsp_object_associations();}catch(const std::runtime_error&){refused=true;}check(refused&&i.native_bsp_associations.attempts==1&&i.native_bsp_associations.refusals==1&&i.native_bsp_association_refusals==1&&i.native_bsp_associations.completed==0,"retained mask aliased private sidecar");check(std::equal(before.begin(),before.end(),i.bank.view(0x10006000,64).begin()),"alias guard changed retained mask");++models;}
 Flat positive{};positive.native_bsp_association_calls=positive.native_bsp_association_attempts=positive.native_bsp_association_kernel_entries=positive.native_bsp_association_iterator_calls=1;check(gate(positive),"declared positive gate differs");
 for(unsigned mode=0;mode<NEGATIVE_GATE_COUNT;++mode){auto f=positive;if(mode==0)f.native_bsp_association_calls=0;if(mode==1)f.native_bsp_association_attempts=2;if(mode==2)f.native_bsp_association_kernel_entries=2;if(mode==3)f.native_bsp_association_refusals=1;if(mode==4)f.native_bsp_association_kernel_refusals=1;if(mode==5)f.native_bsp_association_resource_refusals=1;if(mode==6)f.native_bsp_association_observer_refusals=1;if(mode==7)f.native_bsp_association_iterator_calls=0;if(mode==8)f.native_bsp_association_calls=f.native_bsp_association_attempts=f.native_bsp_association_kernel_entries=2;check(!gate(f),"negative native gate admitted");++models;}
 Impl<>distinct;auto&k=distinct.native_bsp_associations;k.attempts=1;k.completed=2;k.refusals=3;k.iterator_calls=4;k.reset_objects=5;k.association_records=6;k.matched_objects=7;k.excluded_objects=8;distinct.native_bsp_association_dispatches=9;distinct.native_bsp_association_refusals=10;distinct.native_bsp_association_resource_refusals=11;distinct.native_bsp_association_observer_refusals=12;const auto j=report(transport(distinct));const Json expected{{"native_bsp_association_attempts",9},{"native_bsp_association_calls",2},{"native_bsp_association_refusals",10},{"native_bsp_association_kernel_entries",1},{"native_bsp_association_kernel_refusals",3},{"native_bsp_association_iterator_calls",4},{"native_bsp_association_reset_objects",5},{"native_bsp_association_association_records",6},{"native_bsp_association_matched_objects",7},{"native_bsp_association_excluded_objects",8},{"native_bsp_association_resource_refusals",11},{"native_bsp_association_observer_refusals",12}};check(j==expected,"distinct getter contribution lost");
 std::cout<<models<<" exact BSP bootstrap/resource/counter/gate controls passed\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
'''
  code=code.replace('NEGATIVE_GATE_COUNT',str(negative_gates));project=owner/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');shutil.copytree(STAGE/'include',project/'include',dirs_exist_ok=True)
  files=['src/runtime/source_runtime_memory.cpp','src/runtime/source_runtime_snapshot_bytes.cpp','src/assets/cache_file_snapshot.cpp','src/runtime/source_runtime_write_protection.inc']
  for n in files:
   dst=project/n;dst.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/n,dst)
  dst=project/'src/physics/native_tu15_bsp_object_association.cpp';dst.parent.mkdir(parents=True);shutil.copyfile(STAGE/'src/physics/native_tu15_bsp_object_association.cpp',dst)
  (project/'cmake').mkdir();shutil.copyfile(STAGE/'cmake/NativeTu15BspObjectAssociation.cmake',project/'cmake/NativeTu15BspObjectAssociation.cmake');(project/'driver.cpp').write_text(code);(a.output/'driver.cpp').write_text(code);(project/'systems.cpp').write_text('int bsp_systems_anchor(){return 0;}\n')
  (project/'CMakeLists.txt').write_text('cmake_minimum_required(VERSION 3.24)\nproject(bspBootstrap LANGUAGES CXX)\ninclude(CTest)\nfind_package(Threads REQUIRED)\nadd_library(halo2_systems STATIC systems.cpp)\ninclude(cmake/NativeTu15BspObjectAssociation.cmake)\nadd_executable(model driver.cpp src/runtime/source_runtime_memory.cpp src/runtime/source_runtime_snapshot_bytes.cpp src/assets/cache_file_snapshot.cpp)\ntarget_include_directories(model PRIVATE include src)\ntarget_compile_features(model PRIVATE cxx_std_20)\ntarget_link_libraries(model PRIVATE halo2_systems Threads::Threads)\nadd_test(NAME bsp_bootstrap_host COMMAND model)\n')
  pins={str(p.relative_to(STAGE)):sha(p)for p in STAGE.rglob('*')if p.is_file()};compiled={str(p.relative_to(project)):sha(p)for p in project.rglob('*')if p.is_file()};results=[]
  for cc in('g++','clang++'):
   cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(owner/'builds'),'--output',str(a.output/cc),'--target','model','--filter','^bsp_bootstrap_host$','--cmake-arg=-DCMAKE_CXX_COMPILER='+cc,'--jobs','1','--test-jobs','1'];run=subprocess.run(cmd);results.append(dict(compiler=cc,exit_code=run.returncode));assert run.returncode==0,'Exact bootstrap host/resource model differs'
  assert all(sha(STAGE/n)==h for n,h in pins.items())and all(sha(project/n)==h for n,h in compiled.items());r.update(passed=True,results=results,stage_source_sha256=pins,compiled_source_sha256=compiled,exact_source_methods=True,positive_models=3,observer_refusal_models=2,resource_models=3,negative_gate_models=negative_gates,scalar_fields=12,two_json_fields=24,source_and_active_cohort_unchanged=True,actual_bootstrap_live_claim=False)
 except BaseException as e:r['error']=str(e)or repr(e);raise
 finally:r['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in owner.rglob('*')if p.is_file());(a.output/'report.json').write_text(json.dumps(r,indent=2)+'\n')
if __name__=='__main__':main()
