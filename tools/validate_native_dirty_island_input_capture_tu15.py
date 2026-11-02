#!/usr/bin/env python3
"""Interface/ownership/thread/read-only models for actual282C10 capture."""
import argparse,os,subprocess
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--source-root',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);stage=a.stage.resolve()/'staged';scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  cpp=r'''#include "helpers/native_dirty_island_objects_input_capture_tu15.hpp"
#include <cassert>
#include <thread>
using namespace halo2;using Input=NativeTu15DirtyIslandObjectsInput;using Binding=NativeTu15DirtyIslandObjectsInputObserverBinding;
void other(const SourcePhysicsContactMemory&,const Input&);
struct Memory:SourcePhysicsContactMemory{
 std::map<std::uint32_t,std::vector<std::byte>>banks;unsigned writes{};mutable std::vector<std::byte>loan=std::vector<std::byte>(4096);bool ephemeral{true};
 void put(std::uint32_t p,std::uint32_t v,unsigned width=4){auto i=banks.upper_bound(p);assert(i!=banks.begin());--i;assert(p-i->first+width<=i->second.size());for(unsigned n=0;n<width;++n)i->second[p-i->first+n]=std::byte(v>>(8u*n));}
 void read(std::uint32_t,std::span<std::byte>)const override{assert(false&&"Capture must inspect const committed spans only");}
 void write(std::uint32_t,std::span<const std::byte>)override{++writes;}
 template<class F>void visit_committed(F&&f)const{for(const auto&[p,b]:banks){if(ephemeral){loan.assign(b.begin(),b.end());f(p,std::span<const std::byte>(loan));std::fill(loan.begin(),loan.end(),std::byte{0xa5});}else f(p,std::span<const std::byte>(b));}}
};
Memory make(unsigned cohort){Memory m;for(auto p:{0x56dfccu,0x48b870u,0x52be48u})m.banks[p].resize(8);m.banks[0x1000000u].resize(0x1000);m.put(0x56dfcc,1,1);m.put(0x52be48,0x1000100);m.put(0x1000100+(cohort?0x14:8),0x1000200);m.put(0x1000100+(cohort?0x18:12),1);m.put(0x1000200,0x1000300);m.put(0x10003ae,1,1);m.put(0x1000340,4);return m;}
int main(int argc,char**argv){assert(argc==2);const std::filesystem::path root=argv[1];Input input{{0x27f,0x20,0x1fa0},0};auto m=make(0),m2=make(1);const auto original=m.banks;unsigned outer{},inner{},other_memory{};
 {Binding a(m,[&](const auto&owner,const auto&i){assert(&owner==&m&&i.floating_point.status==0x20&&i.preserved_x87_depth==0);++outer;});
 other(m,input);assert(outer==1);
 {Binding b(m,[&](const auto&,const auto&){++inner;});other(m,input);assert(inner==1&&outer==1);}
 other(m,input);assert(outer==2);
 {Binding c(m2,[&](const auto&,const auto&){++other_memory;});other(m2,input);assert(other_memory==1);other(m,input);assert(outer==3);}
 std::thread t([&]{assert(!native_tu15_observe_dirty_island_objects_input(m,input));});t.join();
 try{Binding b(m,[](const auto&,const auto&){throw std::runtime_error("observer stop");});other(m,input);assert(false);}catch(const std::runtime_error&){}
 other(m,input);assert(outer==4);
 }assert(!native_tu15_observe_dirty_island_objects_input(m,input));
 for(unsigned cohort=0;cohort<2;++cohort){auto memory=make(cohort);auto before=memory.banks;test::NativeDirtyIslandObjectsInputCaptureTu15 capture(root/("capture"+std::to_string(cohort)));auto depth=input;depth.preserved_x87_depth=1;capture.observe(depth,memory);assert(capture.activity().unsupported_depths==1&&capture.activity().captures==0);capture.observe(input,memory);capture.observe(input,memory);assert(capture.activity().captures==1&&capture.activity().attempts==2&&memory.banks==before&&memory.writes==0);
 std::ifstream in(root/("capture"+std::to_string(cohort))/"capture-native-before.json");nlohmann::json j;in>>j;assert(j["schema"]=="native-tu15-dirty-island-objects-typed-before-v1"&&j["input"].size()==3&&j["input"]["owner"]==0x282c10u&&j["input"]["preserved_x87_depth"]==0&&j["input"]["floating_point"]["mxcsr"]==0x1fa0&&j["readonly_selection"]["cohort"]==cohort&&j["readonly_selection"]["island"]==0x1000300u&&j["regions"].size()==4&&j["bank_bytes"]==4120u&&j["source_memory_mutated"]==false&&j["guest_abi_fabricated"]==false);
 for(const auto&r:j["regions"]){std::ifstream bank(root/("capture"+std::to_string(cohort))/r["before_file"].get<std::string>(),std::ios::binary);std::vector<char>b((std::istreambuf_iterator<char>(bank)),{});const auto&expected=before.at(r["address"]);assert(b.size()==expected.size());for(unsigned n=0;n<b.size();++n)assert(std::byte(b[n])==expected[n]);}}
 test::NativeDirtyIslandObjectsInputCaptureTu15 skip(root/"skip");m.put(0x56dfcc,0,1);skip.observe(input,m);assert(skip.activity().pending_skips==1);m.put(0x56dfcc,1,1);m.put(0x48b873,1,1);skip.observe(input,m);assert(skip.activity().reentrant_skips==1);m.put(0x48b873,0,1);m.put(0x10003ae,0,1);skip.observe(input,m);assert(skip.activity().no_dirty_island==1);assert(!std::filesystem::exists(root/"skip"));m.put(0x10003ae,1,1);
 bool rejected{};try{test::NativeDirtyIslandObjectsInputCaptureTu15 bad("/tmp/halo2-foreign-dirty-island");}catch(const std::runtime_error&){rejected=true;}assert(rejected);
 std::filesystem::create_directories(root/"existing");test::NativeDirtyIslandObjectsInputCaptureTu15 existing(root/"existing");rejected=false;try{existing.observe(input,m);}catch(const std::runtime_error&){rejected=true;}assert(rejected&&m.writes==0);
 m.banks.erase(0x52be48);skip.observe(input,m);assert(skip.activity().unavailable_banks==1&&m.writes==0);
 return 0;}
'''
  source=scratch/'model.cpp';source.write_text(cpp);other=scratch/'other.cpp';other.write_text('#include "halo2/native_tu15_dirty_island_objects_input_observer.hpp"\nvoid other(const halo2::SourcePhysicsContactMemory&m,const halo2::NativeTu15DirtyIslandObjectsInput&i){halo2::native_tu15_observe_dirty_island_objects_input(m,i);}\n');results=[]
  for compiler in ('g++','clang++'):
   binary=scratch/compiler.replace('+','x');cmd=[compiler,'-std=c++20','-O1','-pthread','-I'+str(stage/'include'),'-I'+str(a.source_root/'include'),'-I'+str(stage/'tests'),str(source),str(other),'-o',str(binary)];r=subprocess.run(cmd,capture_output=True,text=True);(a.output/(compiler.replace('+','x')+'.log')).write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr[-2500:];subprocess.run([str(binary),str(scratch/(compiler.replace('+','x')+'-captures'))],check=True);results.append(dict(compiler=compiler,command=cmd,passed=True))
  report.update(passed=True,compiler_results=results,model_only=True,physics_source_mutated=False,native_admission_added=False,tests=['crossTU','nestedScopes','exceptionRestore','perMemory','perExecutor','bothWorldCohorts','depthRefusal','readonlyFilters','actualConstBanks','bankRoundtrip','ephemeralBorrowedSpans','ownedPathEscape','existingDirectoryRefusal','unmappedSkip'],source_hashes={str(p.relative_to(stage)):sha(p)for p in stage.rglob('*')if p.is_file()})
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0
if __name__=='__main__':raise SystemExit(main())
