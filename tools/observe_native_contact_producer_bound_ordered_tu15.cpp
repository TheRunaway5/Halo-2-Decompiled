// Uses the independent offline oracle/retained DATA helpers. No original
// execution, original outcomes or private/control bytes enter the binding.
#include "halo2/native_tu15_contact_producer_binding.hpp"
#include <xmmintrin.h>
namespace {
void setup_parent(Child&c,const J&input,unsigned mode){
 c.put(sp,stop);const std::array<std::uint32_t,4>a{input.at("first"),input.at("second"),input.at("configuration"),input.at("output")};
 for(unsigned k=0;k<4;++k)c.put(sp+4u+k*4u,a[k]);
 auto r=c.regs();r.rax=r.rbx=r.rdx=r.rsi=r.rdi=r.rbp=0;r.rcx=input.at("agent");r.rsp=sp;r.rip=0x304350u;r.eflags&=~0x400u;request(PTRACE_SETREGS,c.pid,nullptr,&r);
 auto f=c.fp();f.cwd=input.at("floating_point").at("x87_control_word");f.swd=input.at("floating_point").at("x87_status");f.mxcsr=input.at("floating_point").at("mxcsr");f.cwd=(f.cwd&~0xc00u)|(mode<<10u);f.mxcsr=(f.mxcsr&~0x6000u)|(mode<<13u);f.ftw=0;std::memset(f.st_space,0,sizeof(f.st_space));request(PTRACE_SETFPREGS,c.pid,nullptr,&f);
}
J difference(const J&a,const J&b){std::size_t k{};while(k<a.size()&&k<b.size()&&a[k]==b[k])++k;return J{{"index",k},{"expected_count",a.size()},{"native_count",b.size()},{"expected",k<a.size()?a[k]:J{}},{"native",k<b.size()?b[k]:J{}}};}
void guards(J&report){
 struct Model:SourcePhysicsContactMemory {NativeTu15PairDeletionNativeScope&scope;mutable unsigned touches{};bool reject{};explicit Model(NativeTu15PairDeletionNativeScope&s):scope(s){}void read(std::uint32_t,std::span<std::byte>)const override{if(reject)scope.reject_unconverted(0x304350u);++touches;throw std::runtime_error("Unexpected model DATA read");}void write(std::uint32_t,std::span<const std::byte>)override{++touches;throw std::runtime_error("Unexpected model DATA write");}};
 J controls=J::array();
 for(unsigned mode=0;mode<11;++mode){
  NativeTu15PairDeletionNativeScope scope;Model memory(scope);NativeTu15ContactProducerBindingActivity a;
  NativeTu15ContactProducerBindingScratch s{0x74000000u,0x10000u};NativeTu15PhysicsFrameFp f{0x27f,0x20,0x1fa0};unsigned depth{};auto agent=1u,first=2u,second=3u,context=4u,output=5u;
  if(mode==0)s.identity=0;if(mode==1)++s.identity;if(mode==2)--s.bytes;if(mode==3)s.identity=0xfffffff0u;if(mode==4)depth=1;if(mode==5)--f.control_word;if(mode==6)f.mxcsr|=0x10000u;
  if(mode==7)agent=s.identity-0x20u;if(mode==8)context=s.identity+0xffe0u;if(mode==9)output=s.identity;if(mode==10)memory.reject=true;
  bool refused{};try{native_tu15_produce_bound_contacts(memory,{agent},{first},{second},{context},{output},s,f,depth,{},scope,&a);}catch(const std::runtime_error&){refused=true;}
  check(refused&&a.attempts==1&&a.refusals==1&&!a.completed&&!scope.active()&&!memory.touches,"Native producer admission/native-only model differs");
  if(mode<4)check(a.resource_refusals==1,"Producer resource counter differs");if(mode==4)check(a.depth_refusals==1,"Producer depth counter differs");if(mode==5||mode==6)check(a.fp_refusals==1,"Producer FP counter differs");if(mode==10)check(a.providers.native_only.refused==1,"Producer native-only counter differs");controls.push_back(mode);
 }
 report["production_guard_models"]=controls;
}
}
int main(int argc,char**argv){J report{{"passed",false},{"profiles",J::array()},{"native_original_executor_calls",0},{"captured_gp_used",false},{"source_after_used_as_native_input",false},{"retained_large_artifacts",J::array()}};try{
 check(argc==4,"Supply original ELF, authenticated metadata and report");std::ifstream file(argv[2]);const auto meta=J::parse(file);const auto&input=meta.at("input");const Regions regions(meta.at("regions"));guards(report);
 for(unsigned mode:meta.at("modes")){
  Child source;source.start(argv[1]);setup_parent(source,input,mode);const auto before=source.fp();const auto expected=original(source,meta,regions);const auto after=source.fp();
  J row{{"passed",false},{"mode",mode},{"source_instructions",expected.steps},{"ordered_source_reads",expected.reads.size()},{"ordered_source_stores",expected.stores.size()},{"passes",J::array()}};report["profiles"].push_back(row);auto&result=report["profiles"].back();
  for(bool read_pass:{false,true}){
   Child child;child.start(argv[1]);Memory storage(child,regions,read_pass);NativeTu15PhysicsFrameFp fp{before.cwd,before.swd,before.mxcsr};J events=J::array();NativeTu15RetainedStoreObserver observed;
   if(!read_pass)observed=NativeTu15RetainedStoreObserver{[&](const auto&e){events.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},{"written",hex(e.written)}});
    __m128 x=_mm_set_ss(1.0f),y=_mm_set_ss(3.0f);asm volatile("divss %1,%0":"+x"(x):"x"(y):"memory");std::uint16_t cw;asm volatile("fnstcw %0":"=m"(cw));cw^=0xc00u;asm volatile("fldcw %0"::"m"(cw):"memory");_mm_setcsr(_mm_getcsr()^0x6000u);
    const double three=3.;asm volatile("fld1;fdivl %0;fstp %%st(0)"::"m"(three):"memory","st");
   }};
   NativeTu15PairDeletionNativeScope scope;NativeTu15ContactProducerBindingActivity activity;
   native_tu15_produce_bound_contacts(storage,{input.at("agent")},{input.at("first")},{input.at("second")},{input.at("configuration")},{input.at("output")},{0x74000000u,0x10000u},fp,0,{},scope,&activity,observed);
   const auto&actual=read_pass?storage.reads:events;const auto&wanted=read_pass?expected.reads:expected.stores;
   result[read_pass?"read_difference":"store_difference"]=difference(wanted,actual);
   check(actual==wanted,read_pass?"Native full producer retained read order differs":"Native full producer retained observer SourcePC/preimage/value/order differs");
   std::uint64_t compared{},different{};for(const auto&bank:meta.at("regions")){const auto p=bank.at("address").get<std::uint32_t>(),n=bank.at("size").get<std::uint32_t>();const auto original=source.read(p,n),native=child.read(p,n);compared+=n;for(unsigned k=0;k<n;++k)different+=original[k]!=native[k];}
   check(!different&&fp_words(after)==J::array({fp.control_word,fp.status,fp.mxcsr}),"Observed full producer banks/FP differ");
   check(activity.attempts==1&&activity.completed==1&&!activity.refusals&&!activity.unprovided_reads&&!scope.active()&&!activity.providers.native_only.refused,"Observed full producer activities differ");
   result["passes"].push_back(J{{"passed",true},{"read_pass",read_pass},{"compared_bytes",compared},{"different_bytes",different},{"observer_events",events.size()},{"observer_performs_host_fp_math",!read_pass},{"fp",J::array({fp.control_word,fp.status,fp.mxcsr})}});
  }
  result["passed"]=true;
 }
 report["passed"]=true;
 }catch(const std::exception&e){report["passed"]=false;report["error"]=e.what();std::cerr<<e.what()<<'\n';}
 if(argc==4){std::ofstream out(argv[3]);out<<report.dump(2)<<'\n';}return report.at("passed").get<bool>()?0:1;
}
