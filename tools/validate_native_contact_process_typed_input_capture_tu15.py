#!/usr/bin/env python3
"""Typed2E5320/304350 readonly registry, ephemeral-bank capture and fullWorld context."""
import argparse,os,shutil,subprocess
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',required=True,type=Path);p.add_argument('--source-root',required=True,type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);stage=a.stage/'staged';scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  cpp=r'''#include "helpers/native_contact_process_typed_input_capture_tu15.hpp"
#include <cassert>
using namespace halo2;using Input=NativeTu15ContactProcessTypedInput;using Binding=NativeTu15ContactProcessTypedInputObserverBinding;
void other(const SourcePhysicsContactMemory&,const Input&);
struct Memory:SourcePhysicsContactMemory{
 mutable std::vector<std::byte>loan;std::array<std::byte,4096>bank{};std::array<std::byte,8>noise{};unsigned writes{},extra{};
 void read(std::uint32_t,std::span<std::byte>)const override{assert(false);}void write(std::uint32_t,std::span<const std::byte>)override{++writes;}
 template<class F>void visit_committed(F&&f)const{
  loan.assign(bank.begin(),bank.end());f(0x1000000u,std::span<const std::byte>(loan));std::fill(loan.begin(),loan.end(),std::byte{0xa5});
  for(unsigned n=0;n<1+extra;++n){loan.assign(noise.begin(),noise.end());f(0x52be48u+n*16u,std::span<const std::byte>(loan));std::fill(loan.begin(),loan.end(),std::byte{0xa5});}
 }
};
int main(int argc,char**argv){assert(argc==2);Memory m,other_memory;for(unsigned n=0;n<m.bank.size();++n)m.bank[n]=std::byte(n*19u);const auto before=m.bank;
 Input i{0x2e5320u,0x1000100u,0x1000200u,0x1000300u,0x1000400u,0x1000500u,{0x27f,0x20,0x1fa0},0};unsigned outer{},inner{};
 {Binding a(m,[&](const auto&owner,const auto&v){assert(&owner==&m&&v.owner==i.owner&&v.agent==i.agent&&v.first==i.first&&v.second==i.second&&v.configuration==i.configuration&&v.output==i.output&&v.floating_point.status==0x20&&v.preserved_x87_depth==0);++outer;});
  other(m,i);assert(outer==1);{Binding b(m,[&](const auto&,const auto&){++inner;});other(m,i);assert(inner==1&&outer==1);}other(m,i);assert(outer==2);assert(!native_tu15_observe_contact_process_typed_input(other_memory,i));std::thread t([&]{assert(!native_tu15_observe_contact_process_typed_input(m,i));});t.join();try{Binding b(m,[](const auto&,const auto&){throw std::runtime_error("stop");});other(m,i);assert(false);}catch(const std::runtime_error&){}other(m,i);assert(outer==3);}
 assert(!native_tu15_observe_contact_process_typed_input(m,i));
 const auto directory=std::filesystem::path(argv[1]);test::NativeContactProcessTypedInputCaptureTu15 capture(directory,0x2e5320u);auto unrelated=i;unrelated.owner=0x304350u;unrelated.agent=0;capture.observe(unrelated,m);assert(!std::filesystem::exists(directory));capture.observe(i,m);capture.observe(i,m);assert(m.writes==0&&m.bank==before);
 std::ifstream in(directory/"capture-native-before.json");nlohmann::json j;in>>j;assert(j["schema"]=="native-tu15-contact-process-typed-before-v1"&&j["input"]["owner"]==0x2e5320u&&j["input"]["agent"]==i.agent&&j["input"]["first"]==i.first&&j["input"]["second"]==i.second&&j["input"]["configuration"]==i.configuration&&j["input"]["output"]==i.output&&j["regions"].size()==2&&j["bank_bytes"]==4104&&j["guest_abi_fabricated"]==false&&j["physics_result_supplied"]==false);
 for(const auto&row:j["regions"]){std::ifstream f(directory/row["before_file"].get<std::string>(),std::ios::binary);std::vector<char>b((std::istreambuf_iterator<char>(f)),{});if(row["address"]==0x1000000u){assert(b.size()==m.bank.size());for(unsigned n=0;n<b.size();++n)assert(std::byte(b[n])==m.bank[n]);}else assert(b.size()==m.noise.size());}
 for(unsigned mode=0;mode<4;++mode){auto bad=i;if(mode==0)bad.preserved_x87_depth=1;if(mode==1)bad.first=0;if(mode==2)bad.output=0x99999999u;if(mode==3)m.extra=8192;const auto path=directory.parent_path()/("bad"+std::to_string(mode));test::NativeContactProcessTypedInputCaptureTu15 rejected(path,0x2e5320u);bool failed{};try{rejected.observe(bad,m);}catch(const std::runtime_error&){failed=true;}assert(failed&&!std::filesystem::exists(path)&&m.writes==0);}bool escaped{};try{test::NativeContactProcessTypedInputCaptureTu15 rejected("/tmp/foreign-moving-shape-capture",0x2e5320u);}catch(const std::runtime_error&){escaped=true;}assert(escaped);m.extra=0;auto other_owner=i;other_owner.owner=0x304350u;test::NativeContactProcessTypedInputCaptureTu15 secondary(std::filesystem::path(directory.string()+"-owner304350"),0x304350u);secondary.observe(other_owner,m);std::ifstream second(std::filesystem::path(directory.string()+"-owner304350")/"capture-native-before.json");nlohmann::json sj;second>>sj;assert(sj["input"]["owner"]==0x304350u);bool bad_filter{};try{test::NativeContactProcessTypedInputCaptureTu15 rejected(directory.parent_path()/"ownerBad",0xdead);}catch(const std::runtime_error&){bad_filter=true;}assert(bad_filter&&!std::filesystem::exists(directory.parent_path()/"ownerBad"));return 0;
}
'''
  model=scratch/'model.cpp';model.write_text(cpp);other=scratch/'other.cpp';other.write_text('#include "halo2/native_tu15_contact_process_typed_input_observer.hpp"\nvoid other(const halo2::SourcePhysicsContactMemory&m,const halo2::NativeTu15ContactProcessTypedInput&i){halo2::native_tu15_observe_contact_process_typed_input(m,i);}\n');results=[]
  for cc in('g++','clang++'):
   exe=scratch/cc.replace('+','x');command=[cc,'-std=c++20','-O1','-pthread','-I'+str(stage/'include'),'-I'+str(a.source_root/'include'),'-I'+str(stage/'tests'),str(model),str(other),'-o',str(exe)];r=subprocess.run(command,capture_output=True,text=True);(a.output/(cc+'-interface-build.log')).write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr[-3000:];r=subprocess.run([str(exe),str(scratch/(cc+'-capture'))],capture_output=True,text=True);(a.output/(cc+'-interface-run.log')).write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr
   world=stage/'src/physics/native_tu15_world_integration_residuals.cpp';command=[cc,'-std=c++20','-O0','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(stage/'include'),'-I'+str(a.source_root/'include'),'-I'+str(a.source_root/'src'),'-fsyntax-only',str(world)];r=subprocess.run(command,capture_output=True,text=True);(a.output/(cc+'-World-context.log')).write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr[-3000:];results.append(dict(compiler=cc,interface_passed=True,full_World_TU_context_passed=True,World_command=command))
  report.update(passed=True,results=results,model_only=True,native_admission_added=False,source_physics_mutated=False,models=['crossTU','nestedScopes','exceptionRestore','perMemory','perExecutor','ephemeralSpans','bankRoundtrip','depthRefusal','nonnullRefusal','unmappedFormalRefusal','metadataBoundBeforeDirectory','ownedPathEscape','captureOnce','ownerFilterBeforeDATA','separate304350Owner','unknownFilterRefusal'],source_hashes={str(p.relative_to(stage)):sha(p)for p in stage.rglob('*')if p.is_file()})
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
