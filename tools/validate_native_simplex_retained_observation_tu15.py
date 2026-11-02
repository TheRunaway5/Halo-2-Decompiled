#!/usr/bin/env python3
"""Bounded same-view availability, bitwise copy and retained-observer controls."""
import argparse, hashlib, json, os, pathlib, subprocess, time
ROOT=pathlib.Path(__file__).resolve().parents[1]
SOURCE=r'''
#include "halo2/native_tu15_convex_simplex_single.hpp"
#include "halo2/native_tu15_moving_shape_process_binding.hpp"
#include "halo2/native_tu15_contact_candidate_private_data.hpp"
#include "halo2/native_tu15_contact_point_create.hpp"
#include "native_tu15_joint_constraint_numeric_internal.hpp"
#include <iostream>
#include <stdexcept>
using namespace halo2;
struct Storage:SourcePhysicsContactMemory{
 std::array<std::byte,0x1000>b{};mutable unsigned reads{};unsigned writes{};bool fault{};
 void read(std::uint32_t p,std::span<std::byte>v)const override{++reads;if(p<0x1000||std::uint64_t(p)+v.size()>0x2000)throw std::runtime_error("outer read");std::copy_n(b.begin()+p-0x1000,v.size(),v.begin());}
 void write(std::uint32_t p,std::span<const std::byte>v)override{if(fault)throw std::runtime_error("write fault");if(p<0x1000||std::uint64_t(p)+v.size()>0x2000)throw std::runtime_error("outer write");++writes;std::copy(v.begin(),v.end(),b.begin()+p-0x1000);}
};
void require_at(bool x,int line){if(!x)throw std::runtime_error("control assertion line="+std::to_string(line));}
#define require(x) require_at((x),__LINE__)
template<class F>void refuses(F&&f){bool caught=false;try{f();}catch(const std::runtime_error&){caught=true;}require(caught);}
static_assert(!std::is_copy_constructible_v<NativeTu15ConvexSimplexMemory>);
static_assert(!std::is_convertible_v<SourcePhysicsContactRecordObserver,NativeTu15RetainedStoreObserver>);
int main(){try{
 Storage storage;std::vector<SourcePhysicsContactRecordStore>events;
 NativeTu15RetainedStoreObserver typed{[&](const auto&e){events.push_back(e);}};
 const std::array<NativeTu15ContactPrivateRange,1>owned{{{0x4000,0x100}}},retained{{{0x1000,0x1000}}};
 NativeTu15ContactCandidatePrivateData arena(storage,owned,retained);
 NativeTu15ConvexSimplexMemory view(arena,typed);view.withhold(0x4000,0x100);
 std::array<std::byte,4>four{};unsigned controls=0;
 refuses([&]{view.read(0x4000,four);});require(storage.reads==0);++controls;
 {joint_constraint_numeric::Memory m(view,{});m.word(0x1234,0x4000,0x12345678);require(events.empty()&&storage.reads==0);require(m.word(0x4000)==0x12345678);}++controls;
 {joint_constraint_numeric::Memory m(view,{});m.word(0x2345,0x1000,0x87654321);require(events.size()==1&&events.back().instruction==0x2345&&events.back().address==0x1000&&events.back().before==std::vector<std::byte>(4)&&events.back().written[0]==std::byte{0x21});}++controls;
 view.withhold(0x4000,4);require(!view.data_is_defined(0x4000,4)&&!arena.data_is_defined(0x4000,4));refuses([&]{view.read(0x4000,four);});++controls;
 {NativeTu15ConvexSimplexMemory nested(static_cast<SourcePhysicsContactMemory&>(view));nested.withhold(0x4000,4);joint_constraint_numeric::Memory m(nested,{});m.word(0x3456,0x4000,0xaabbccdd);require(view.data_is_defined(0x4000,4)&&arena.data_is_defined(0x4000,4));m.word(0x4567,0x1004,0x10203040);require(events.size()==2&&events.back().instruction==0x4567);}++controls;
 const std::array<std::byte,2>pair{std::byte{0x11},std::byte{0x22}};view.write(0x4040,pair);require(view.copy_defined_data(0x5678,0x4042,0x4040,4));require(view.data_is_defined(0x4042,2)&&!view.data_is_defined(0x4044,2));std::array<std::byte,2>out{};view.read(0x4042,out);require(out==pair);refuses([&]{view.read(0x4042,four);});++controls;
 {const auto before=storage.b;refuses([&]{view.copy_defined_data(0x6789,0x1010,0x4042,4);});require(storage.b==before);}++controls;
 {NativeTu15ConvexSimplexMemory nested(static_cast<SourcePhysicsContactMemory&>(view));nested.withhold(0x4050,4);require(!arena.data_is_defined(0x4050,4));require(nested.copy_defined_data(0x789a,0x4054,0x4050,4));require(!nested.data_is_defined(0x4054,4)&&!view.data_is_defined(0x4054,4)&&!arena.data_is_defined(0x4054,4));}++controls;
 {const auto before=storage.b;refuses([&]{view.withhold(0x1010,4);});require(storage.b==before&&view.data_is_defined(0x1010,4));}++controls;
 refuses([&]{view.require_retained_extent(0x4000,4);});++controls;
 {const auto n=events.size();joint_constraint_numeric::Memory m(view,{});refuses([&]{m.word(0x2f37e3,0x4000,0);});require(events.size()==n);}++controls;
 {const auto reads=storage.reads;refuses([&]{view.read(0x40fe,four);});require(storage.reads==reads);}++controls;
 {NativeTu15ConvexSimplexMemory raw(storage);unsigned n=0;SourcePhysicsContactRecordObserver observer=[&](const auto&e){require(e.instruction==0x9876);++n;};joint_constraint_numeric::Memory m(raw,observer);m.word(0x9876,0x1014,0xffffffff);require(n==1);}++controls;
 {unsigned callbacks=0;NativeTu15RetainedStoreObserver throwing{[&](const auto&){++callbacks;throw std::runtime_error("observer");}};NativeTu15ConvexSimplexMemory observed(arena,throwing);NativeTu15PhysicsFrameFp fp{0x27f,0,0x1f80};joint_constraint_numeric::Environment env(fp,0);joint_constraint_numeric::Memory m(observed,{});joint_constraint_numeric::push(0x3f800000);refuses([&]{m.pop_float(0xabcd,0x1020);});std::uint16_t status;asm volatile("fnstsw %0":"=a"(status));require(callbacks==1&&!(status&0x3800)&&m.word(0x1020)==0x3f800000);}++controls;
 {SourcePhysicsContactRecordObserver observer=[](const auto&){};NativeTu15PhysicsFrameFp fp{0x27f,0,0x1f80};NativeTu15ContactPointCreateStats stats;const auto reads=storage.reads;refuses([&]{native_tu15_create_contact_point(view,0x1100,{0x1200},{0x1300},{0x1400},0x1500,{0x4060},fp,0,{},observer,&stats);});require(storage.reads==reads&&stats.refusals==1);}++controls;
 {joint_constraint_numeric::Memory m(view,{});m.word(0x1111,0x4060,0x1234);const auto n=events.size();require(view.copy_defined_data(0x2222,0x1024,0x4060,4));require(events.size()==n+1&&events.back().instruction==0x2222&&events.back().address==0x1024);}++controls;
 {const auto n=events.size();joint_constraint_numeric::Memory m(view,{});m.word(0x3333,0x4060,0x5678);require(view.copy_defined_data(0x4444,0x4064,0x4060,4));require(events.size()==n);}++controls;
 {joint_constraint_numeric::Memory m(view,{});refuses([&]{m.word(0x2e565c,0x4000,0);});}++controls;
 {NativeTu15RetainedStoreObserver one{[count=0](const auto&)mutable{if(++count==2)throw std::runtime_error("second event");}};NativeTu15RetainedStoreObserver two=one;NativeTu15ConvexSimplexMemory first(arena,one),second(arena,two);joint_constraint_numeric::Memory a(first,{}),b(second,{});a.word(0x5555,0x1030,1);refuses([&]{b.word(0x6666,0x1034,2);});require(b.word(0x1034)==2);}++controls;
 {unsigned callbacks=0;std::uint16_t hostcw;asm volatile("fnstcw %0":"=m"(hostcw));NativeTu15RetainedStoreObserver noisy{[&](const auto&){std::uint16_t incoming;asm volatile("fnstcw %0":"=m"(incoming));require(incoming==hostcw);++callbacks;const std::uint16_t changed=0x77f;const std::uint32_t csr=0x5fa0;asm volatile("fldcw %0;fld1;fldz;fdivp;fstp %%st(0)"::"m"(changed):"memory","st","st(1)");asm volatile("ldmxcsr %0"::"m"(csr):"memory");if(callbacks==2)throw std::runtime_error("FP callback");}};NativeTu15ConvexSimplexMemory observed(arena,noisy);NativeTu15PhysicsFrameFp fp{0x27f,0,0x1f80};{joint_constraint_numeric::Environment env(fp,0);joint_constraint_numeric::Memory m(observed,{});joint_constraint_numeric::push(0x3f800000);std::uint16_t before;asm volatile("fnstsw %0":"=a"(before));m.word(0x7777,0x1038,3);std::uint16_t after,cw;std::uint32_t csr,bits;asm volatile("fnstsw %0":"=a"(after));asm volatile("fnstcw %0":"=m"(cw));asm volatile("stmxcsr %0":"=m"(csr));asm volatile("fsts %0":"=m"(bits));require(before==after&&cw==0x27f&&csr==0x1f80&&bits==0x3f800000);refuses([&]{m.pop_float(0x8888,0x103c);});asm volatile("fnstsw %0":"=a"(after));asm volatile("stmxcsr %0":"=m"(csr));require(callbacks==2&&!(after&0x3800)&&csr==0x1f80&&m.word(0x103c)==0x3f800000);}require(fp.control_word==0x27f&&fp.status==0&&fp.mxcsr==0x1f80);}++controls;
 {const std::array<NativeTu15ContactPrivateRange,1> agent{{{0x3f80,0xf4}}};refuses([&]{NativeTu15ContactCandidatePrivateData alias(storage,owned,agent);});}++controls;
 std::cout<<"{\"passed\":true,\"controls\":"<<controls<<",\"retained_events\":"<<events.size()<<"}\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
'''
def main():
 p=argparse.ArgumentParser();p.add_argument('--stage',type=pathlib.Path,required=True);p.add_argument('--report',type=pathlib.Path,required=True);a=p.parse_args();a.stage=a.stage.resolve();scratch=pathlib.Path(os.environ['HALO2_TEST_SCRATCH']);base=ROOT/'analysis/physics/native-moving-shape-process-qualified-actual1479-20261102/source';src=scratch/'controls.cpp';src.write_text(SOURCE);results=[];start=time.time()
 for cc in ('g++','clang++'):
  exe=scratch/(cc+'-controls');cmd=[cc,'-std=c++20','-O2','-ffp-contract=off','-fno-fast-math','-ffunction-sections','-fdata-sections','-I'+str(a.stage/'include'),'-I'+str(a.stage/'src/physics'),'-I'+str(base/'include'),'-I'+str(base/'src/physics'),'-I'+str(ROOT/'include'),str(src),str(a.stage/'src/physics/native_tu15_convex_simplex_single.cpp'),str(a.stage/'src/physics/native_tu15_contact_point_create.cpp'),'-Wl,--gc-sections','-o',str(exe)];cp=subprocess.run(cmd,text=True,capture_output=True);entry={'compiler':cc,'compile_exit':cp.returncode,'diagnostic':(cp.stdout+cp.stderr)[-12000:]};
  if cp.returncode==0:
   run=subprocess.run([str(exe)],text=True,capture_output=True);entry.update(run_exit=run.returncode,stdout=run.stdout,stderr=run.stderr);entry['passed']=run.returncode==0
  else:entry['passed']=False
  results.append(entry)
 pins={str(q.relative_to(a.stage)):hashlib.sha256(q.read_bytes()).hexdigest() for q in a.stage.rglob('*') if q.is_file()};report={'passed':all(x['passed'] for x in results),'results':results,'stage_sha256':pins,'owned_scratch':str(scratch),'elapsed_seconds':time.time()-start,'retained_large_artifacts':[]};a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({'passed':report['passed'],'report':str(a.report)}));return 0 if report['passed'] else 1
if __name__=='__main__':raise SystemExit(main())
