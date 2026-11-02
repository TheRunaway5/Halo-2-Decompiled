#!/usr/bin/env python3
"""Typed232D20 read-only observer and ephemeral-span capture interface model."""
import argparse,os,subprocess
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--source-root',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);stage=a.stage.resolve()/'staged';scratch=Path(os.environ['HALO2_TEST_SCRATCH']);r=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  cpp=r'''#include "helpers/native_pair_pool_release_input_capture_tu15.hpp"
#include <cassert>
using namespace halo2;using Input=NativeTu15PairPoolReleaseInput;using Binding=NativeTu15PairPoolReleaseInputObserverBinding;
void other(const SourcePhysicsContactMemory&,const Input&);
struct Memory:SourcePhysicsContactMemory{mutable std::vector<std::byte>loan=std::vector<std::byte>(4096);std::array<std::byte,4096>bank{};std::array<std::byte,8>noise{};unsigned writes{};
 void read(std::uint32_t,std::span<std::byte>)const override{assert(false);}void write(std::uint32_t,std::span<const std::byte>)override{++writes;}
 template<class F>void visit_committed(F&&f)const{loan.assign(bank.begin(),bank.end());f(0x1000000u,std::span<const std::byte>(loan));std::fill(loan.begin(),loan.end(),std::byte{0xa5});loan.assign(noise.begin(),noise.end());f(0x52be48u,std::span<const std::byte>(loan));std::fill(loan.begin(),loan.end(),std::byte{0xa5});}
};
int main(int argc,char**argv){assert(argc==2);Memory m,other_memory;Input i{0x1000300u,0x232d20u,0x1000400u,128u,27u,{0x27f,0x20,0x1fa0},0};for(unsigned n=0;n<m.bank.size();++n)m.bank[n]=std::byte(n*37u);const auto before=m.bank;unsigned outer{},inner{};
 {Binding a(m,[&](const auto&owner,const auto&v){assert(&owner==&m&&v.allocator==i.allocator&&v.owner==i.owner&&v.data==i.data&&v.bytes==i.bytes&&v.tag==i.tag&&v.floating_point.status==0x20&&v.preserved_x87_depth==0);++outer;});other(m,i);assert(outer==1);{Binding b(m,[&](const auto&,const auto&){++inner;});other(m,i);assert(inner==1&&outer==1);}other(m,i);assert(outer==2);assert(!native_tu15_observe_pair_pool_release_input(other_memory,i));std::thread t([&]{assert(!native_tu15_observe_pair_pool_release_input(m,i));});t.join();try{Binding b(m,[](const auto&,const auto&){throw std::runtime_error("observer stop");});other(m,i);assert(false);}catch(const std::runtime_error&){}other(m,i);assert(outer==3);}assert(!native_tu15_observe_pair_pool_release_input(m,i));
 const auto directory=std::filesystem::path(argv[1]);test::NativePairPoolReleaseInputCaptureTu15 capture(directory);capture.observe(i,m);capture.observe(i,m);assert(m.writes==0&&m.bank==before);std::ifstream in(directory/"capture-native-before.json");nlohmann::json j;in>>j;assert(j["schema"]=="native-tu15-pair-pool-release-typed-before-v1"&&j["input"]["owner"]==i.owner&&j["input"]["allocator"]==i.allocator&&j["input"]["data"]==i.data&&j["input"]["bytes"]==i.bytes&&j["input"]["tag"]==i.tag&&j["regions"].size()==2&&j["bank_bytes"]==4104&&j["guest_abi_fabricated"]==false);
 for(const auto&row:j["regions"]){std::ifstream f(directory/row["before_file"].get<std::string>(),std::ios::binary);std::vector<char>b((std::istreambuf_iterator<char>(f)),{});if(row["address"]==0x1000000u){assert(b.size()==m.bank.size());for(unsigned n=0;n<b.size();++n)assert(std::byte(b[n])==m.bank[n]);}else assert(b.size()==m.noise.size());}
 auto bad=i;bad.owner^=1;test::NativePairPoolReleaseInputCaptureTu15 mismatch(directory.parent_path()/"mismatch");bool rejected{};try{mismatch.observe(bad,m);}catch(const std::runtime_error&){rejected=true;}assert(rejected&&!std::filesystem::exists(directory.parent_path()/"mismatch"));bad=i;bad.preserved_x87_depth=1;rejected=false;try{mismatch.observe(bad,m);}catch(const std::runtime_error&){rejected=true;}assert(rejected&&m.writes==0);rejected=false;try{test::NativePairPoolReleaseInputCaptureTu15 escaped("/tmp/halo2-foreign-pair-pool-release");}catch(const std::runtime_error&){rejected=true;}assert(rejected);return 0;}
''';source=scratch/'model.cpp';source.write_text(cpp);other=scratch/'other.cpp';other.write_text('#include "halo2/native_tu15_pair_pool_release_input_observer.hpp"\nvoid other(const halo2::SourcePhysicsContactMemory&m,const halo2::NativeTu15PairPoolReleaseInput&i){halo2::native_tu15_observe_pair_pool_release_input(m,i);}\n');results=[]
  for compiler in('g++','clang++'):
   binary=scratch/compiler.replace('+','x');cmd=[compiler,'-std=c++20','-O1','-pthread','-I'+str(stage/'include'),'-I'+str(a.source_root/'include'),'-I'+str(stage/'tests'),str(source),str(other),'-o',str(binary)];q=subprocess.run(cmd,capture_output=True,text=True);(a.output/(compiler.replace('+','x')+'.log')).write_text(q.stdout+q.stderr);assert q.returncode==0,q.stderr[-2200:];subprocess.run([str(binary),str(scratch/(compiler.replace('+','x')+'-capture'))],check=True);results.append(dict(compiler=compiler,command=cmd,passed=True))
  r.update(passed=True,compiler_results=results,model_only=True,native_admission_added=False,source_physics_mutated=False,tests=['crossTU','nestedScopes','exceptionRestore','perMemory','perExecutor','ephemeralSpans','bankRoundtrip','actualOwnerAdmission','depthRefusal','ownedPathEscape','captureOnce'],source_hashes={str(p.relative_to(stage)):sha(p)for p in stage.rglob('*')if p.is_file()})
 finally:r['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',r)
 return 0
if __name__=='__main__':raise SystemExit(main())
