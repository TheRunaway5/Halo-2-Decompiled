#!/usr/bin/env python3
"""Readonly281710 owner/view separation, provided-event DATA and World context."""
import argparse,hashlib,json,os,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',required=True,type=Path);p.add_argument('--source-root',required=True,type=Path);p.add_argument('--base-manifest',required=True,type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.stage=a.stage.resolve();a.source_root=a.source_root.resolve();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);stage=a.stage/'staged';scratch=Path(os.environ['HALO2_TEST_SCRATCH']);pins={str(p.relative_to(stage)):sha(p)for p in stage.rglob('*')if p.is_file()};tool=sha(__file__)
 report=dict(passed=False,owned_scratch=str(scratch),source_sha256=pins,validator_sha256=tool,retained_large_artifacts=[],native_admission_added=False)
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
 try:
  manifest_sha=sha(a.base_manifest);base_pins=json.loads(a.base_manifest.read_text())['source_files_sha256']
  assert all(sha(a.source_root/n)==h for n,h in base_pins.items()),'Frozen base differs before context'
  report.update(base_manifest=str(a.base_manifest.resolve()),base_manifest_sha256=manifest_sha,base_sources_verified_before=len(base_pins))
  for n in pins:
   q=a.output/'source'/n;q.parent.mkdir(parents=True,exist_ok=True);q.write_bytes((stage/n).read_bytes())
  q=a.output/'source/tools'/Path(__file__).name;q.parent.mkdir(parents=True,exist_ok=True);q.write_bytes(Path(__file__).read_bytes())
  model=r'''#include "helpers/native_created_body_contact_input_capture_tu15.hpp"
#include <immintrin.h>
#include <cassert>
using namespace halo2;using Input=NativeTu15CreatedBodyContactInput;using Binding=NativeTu15CreatedBodyContactInputObserverBinding;
void other(const SourcePhysicsContactMemory&,const SourcePhysicsContactMemory&,const Input&);
constexpr std::uint32_t Event=0x600e1000u;
unsigned short sw(){unsigned short x;asm volatile("fnstsw %0":"=a"(x));return x;}
unsigned short cw(){unsigned short x;asm volatile("fnstcw %0":"=m"(x));return x;}
struct View:SourcePhysicsContactMemory{
 std::array<std::byte,2>id{std::byte{0x34},std::byte{0x12}};std::array<std::byte,32>fields{};mutable unsigned reads{},bad{};unsigned writes{};
 void put(unsigned i,std::uint32_t v){for(unsigned n=0;n<4;++n)fields[i*4+n]=std::byte(v>>(n*8u));}
 void read(std::uint32_t p,std::span<std::byte>out)const override{
  ++reads;if(p==Event&&out.size()==2)std::copy(id.begin(),id.end(),out.begin());
  else if(p==Event+4&&out.size()==32)std::copy(fields.begin(),fields.end(),out.begin());
  else if(p==Event+0x20&&out.size()==4)std::copy(fields.end()-4,fields.end(),out.begin());
  else{++bad;throw std::runtime_error("Unprovided padding/stale event read");}
 }
 void write(std::uint32_t,std::span<const std::byte>)override{++writes;assert(false);}
};
struct Banks:SourcePhysicsContactMemory{
 std::array<std::byte,4096>retained{};std::array<std::byte,65536>stale{};mutable std::vector<std::byte>loan;mutable unsigned visits{};unsigned extra{},writes{};mutable std::function<void()>reenter;
 void read(std::uint32_t,std::span<std::byte>)const override{throw std::runtime_error("Stale backing used for current event");}
 void write(std::uint32_t,std::span<const std::byte>)override{++writes;assert(false);}
 template<class F>void visit_committed(F&&f)const{
  ++visits;if(reenter){auto call=std::move(reenter);reenter={};call();}
  loan.assign(retained.begin(),retained.end());f(0x10000000u,std::span<const std::byte>(loan));std::fill(loan.begin(),loan.end(),std::byte{0xa5});
  loan.assign(stale.begin(),stale.end());f(0x600e0000u,std::span<const std::byte>(loan));std::fill(loan.begin(),loan.end(),std::byte{0xa5});
  for(unsigned n=0;n<extra;++n)f(0x5000000u+n*4u,std::span<const std::byte>(retained).first(1));
 }
};
struct NoisyCopy{
 bool*enabled;bool*throw_copy;unsigned*copies;
 NoisyCopy(bool&n,bool&t,unsigned&c):enabled(&n),throw_copy(&t),copies(&c){}
 NoisyCopy(const NoisyCopy&o):enabled(o.enabled),throw_copy(o.throw_copy),copies(o.copies){if(*enabled){++*copies;_mm_setcsr(_mm_getcsr()|0x3fu);if(*throw_copy)throw std::runtime_error("copy");}}
 ~NoisyCopy(){if(*enabled)_mm_setcsr(_mm_getcsr()|0x3fu);}
 void operator()(const SourcePhysicsContactMemory&,const SourcePhysicsContactMemory&,const Input&)const{}
};
int main(int argc,char**argv){assert(argc==2);Banks banks,foreign;View view;for(unsigned n=0;n<8;++n)view.put(n,0x12340000u+n);view.put(7,7u);std::fill(banks.retained.begin(),banks.retained.end(),std::byte{0x41});std::fill(banks.stale.begin(),banks.stale.end(),std::byte{0x3c});const auto saved_fields=view.fields;
 Input i{0x281710u,0x10000010u,Event,{0x27f,0x20,0x1fa0},0};unsigned outer{},inner{};
 {Binding bind(banks,[&](const auto&owner,const auto&event_view,const auto&v){assert(&owner==&banks&&&event_view==&view&&v.listener==i.listener&&v.event==i.event&&v.owner==i.owner&&v.floating_point.status==0x20&&!v.preserved_x87_depth);++outer;});
  other(banks,view,i);assert(outer==1);{Binding nested(banks,[&](const auto&,const auto&,const auto&){++inner;});other(banks,view,i);assert(inner==1&&outer==1);}other(banks,view,i);assert(outer==2);
  assert(!native_tu15_observe_created_body_contact_input(foreign,view,i));std::thread t([&]{assert(!native_tu15_observe_created_body_contact_input(banks,view,i));});t.join();
  for(bool throws:{false,true}){const auto before_sw=sw(),before_cw=cw();const auto before_mx=_mm_getcsr();bool caught{};
   try{Binding fp(banks,[&](const auto&,const auto&,const auto&){_mm_setcsr(_mm_getcsr()|0x3fu);auto control=std::uint16_t(cw()|0x3fu);asm volatile("fldcw %0;fldz;fldz;fdivp;fstp %%st(0)"::"m"(control):"memory","st","st(1)");if(throws)throw std::runtime_error("stop");});other(banks,view,i);}catch(const std::runtime_error&){caught=true;}
   assert(caught==throws&&sw()==before_sw&&cw()==before_cw&&_mm_getcsr()==before_mx);
  }other(banks,view,i);assert(outer==3);
 }assert(!native_tu15_observe_created_body_contact_input(banks,view,i));
 {bool enabled{},throw_copy{};unsigned copies{};NoisyCopy callable(enabled,throw_copy,copies);Binding noisy(banks,callable);const auto before=_mm_getcsr();enabled=true;other(banks,view,i);assert(copies>0&&_mm_getcsr()==before);throw_copy=true;bool caught{};try{other(banks,view,i);}catch(const std::runtime_error&){caught=true;}assert(caught&&_mm_getcsr()==before);enabled=false;}
 const auto directory=std::filesystem::path(argv[1]);test::NativeCreatedBodyContactInputCaptureTu15 capture(directory);
 auto unrelated=i;unrelated.owner=0x316510u;capture.observe(unrelated,banks,view);assert(view.reads==0&&banks.visits==0&&!std::filesystem::exists(directory));
 view.put(7,1u);capture.observe(i,banks,view);assert(view.reads==1&&banks.visits==0&&!std::filesystem::exists(directory));view.put(7,7u);
 unsigned nested{};banks.reenter=[&]{++nested;capture.observe(i,banks,view);};capture.observe(i,banks,view);assert(nested==1);const auto reads=view.reads;capture.observe(i,banks,view);assert(view.reads==reads);
 assert(view.bad==0&&view.writes==0&&banks.writes==0&&view.fields==saved_fields);
 std::ifstream in(directory/"capture-native-before.json");nlohmann::json j;in>>j;assert(j["event_fields"]["flag_word"]==7&&j["event_fields"]["id"]==0x1234&&j["event_padding_provided"]==false&&j["event_backing_bank_is_current_event"]==false&&j["event_data_fragments"].size()==2&&j["bank_bytes"]==69632&&j["regions"].size()==2&&!j["guest_abi_fabricated"].get<bool>()&&!j["physics_result_supplied"].get<bool>());
 for(const auto&r:j["regions"]){std::ifstream f(directory/r["before_file"].get<std::string>(),std::ios::binary);std::vector<char>b((std::istreambuf_iterator<char>(f)),{});const auto expected=r["address"]==0x10000000u?std::byte{0x41}:std::byte{0x3c};for(auto x:b)assert(std::byte(x)==expected);}
 for(const auto&r:j["event_data_fragments"]){std::ifstream f(directory/r["before_file"].get<std::string>(),std::ios::binary);std::vector<char>b((std::istreambuf_iterator<char>(f)),{});assert(b.size()==r["size"]);const auto source=r["size"]==2?std::span<const std::byte>(view.id):std::span<const std::byte>(view.fields);for(unsigned n=0;n<b.size();++n)assert(std::byte(b[n])==source[n]);}
 for(unsigned mode=0;mode<5;++mode){auto bad=i;const auto path=directory.parent_path()/("bad"+std::to_string(mode));test::NativeCreatedBodyContactInputCaptureTu15 rejected(path);if(mode==0)bad.preserved_x87_depth=1;if(mode==1)bad.event=0xfffffff0u;if(mode==2)bad.listener=0;if(mode==3)bad.listener=0x70000000u;if(mode==4)banks.extra=8192;bool caught{};try{rejected.observe(bad,banks,view);}catch(const std::runtime_error&){caught=true;}assert(caught&&!std::filesystem::exists(path));banks.extra=0;}
 bool escaped{};try{test::NativeCreatedBodyContactInputCaptureTu15 bad("/tmp/foreign-created-body-input");}catch(const std::runtime_error&){escaped=true;}assert(escaped);return 0;
}
'''
  (scratch/'model.cpp').write_text(model);(scratch/'other.cpp').write_text('#include "halo2/native_tu15_created_body_contact_input_observer.hpp"\nvoid other(const halo2::SourcePhysicsContactMemory&o,const halo2::SourcePhysicsContactMemory&v,const halo2::NativeTu15CreatedBodyContactInput&i){halo2::native_tu15_observe_created_body_contact_input(o,v,i);}\n')
  results=[]
  for compiler in ('g++','clang++'):
   exe=scratch/compiler.replace('+','x');cmd=[compiler,'-std=c++20','-O1','-pthread','-I'+str(stage/'include'),'-I'+str(a.source_root/'include'),'-I'+str(stage/'tests'),str(scratch/'model.cpp'),str(scratch/'other.cpp'),'-o',str(exe)];r=subprocess.run(cmd,capture_output=True,text=True);(a.output/(compiler+'-interface-build.log')).write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr[-4000:]
   r=subprocess.run([str(exe),str(scratch/(compiler+'-capture'))],capture_output=True,text=True);(a.output/(compiler+'-interface-run.log')).write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr[-4000:]
   cmd=[compiler,'-std=c++20','-O0','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(stage/'include'),'-I'+str(a.source_root/'include'),'-I'+str(a.source_root/'src'),'-I'+str(a.source_root/'src/physics'),'-fsyntax-only',str(a.source_root/'src/physics/native_tu15_world_integration_residuals.cpp')];r=subprocess.run(cmd,capture_output=True,text=True);(a.output/(compiler+'-World-context.log')).write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr[-4000:];results.append(dict(compiler=compiler,interface=True,full_World_context=True,World_command=cmd))
  assert all(sha(stage/n)==h for n,h in pins.items())and sha(__file__)==tool
  assert sha(a.base_manifest)==manifest_sha and all(sha(a.source_root/n)==h for n,h in base_pins.items()),'Frozen base differs after context'
  report.update(passed=True,source_inputs_unchanged=True,base_sources_verified_after=len(base_pins),results=results,models=['crossTU','nestedScope','perOwner','perExecutor','exceptionRestore','hardwareFPnormal','hardwareFPthrow','callableCopyDestructorFP','callableCopyThrowFP','ownerFilterBeforeDATA','flag1SkipBeforeIO','non1FlagPreserved7','currentGuardedEventVsStaleBacking','noPaddingRead','twoSemanticFragments','ephemeralBankRoundtrip','sameView','reentrantSkip','captureOnce','depth','pointerWrap','nonnull','listenerBacking','boundedBanksBeforeDirectory','ownedPathEscape'],generated_model_sha256=hashlib.sha256(model.encode()).hexdigest(),no_physics_or_admission_change=True)
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
if __name__=='__main__':main()
