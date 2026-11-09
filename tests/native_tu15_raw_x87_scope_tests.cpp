#include "physics/runtime/native_tu15_raw_x87_scope.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "platform/native_tu15_tick_conversion.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <vector>
using namespace halo2;
namespace raw=halo2::native_raw_x87_detail;
using State=physics_world_runtime_detail::WorldState;
using J=nlohmann::json;
namespace {
void check(bool ok,const std::string&why){if(!ok)throw std::runtime_error(why);}
std::vector<std::byte> unhex(const std::string&s){std::vector<std::byte> out;for(unsigned n=0;n<s.size();n+=2)out.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return out;}
std::string hex(const std::byte*p,unsigned n){constexpr char digit[]="0123456789abcdef";std::string out;for(unsigned i=0;i<n;++i){const auto b=std::to_integer<unsigned>(p[i]);out+=digit[b>>4u];out+=digit[b&15u];}return out;}
raw::Image image(const J&r){raw::Image out;const auto v=unhex(r.at("x87"));check(v.size()==108,"Invalid oracle image");std::memcpy(out.x87.data(),v.data(),108);out.mxcsr=r.at("mxcsr");return out;}
void same(const raw::Image&a,const raw::Image&b,const std::string&why){
 if(!raw::equivalent(a,b))throw std::runtime_error(why+" actual CW/SW/TW/MX "+std::to_string(a.word(0))+"/"+std::to_string(a.word(4))+"/"+std::to_string(a.word(8))+"/"+std::to_string(a.mxcsr)+" expected "+std::to_string(b.word(0))+"/"+std::to_string(b.word(4))+"/"+std::to_string(b.word(8))+"/"+std::to_string(b.mxcsr));
}
struct Failure{};
struct Memory:SourcePhysicsContactMemory {
 bool fail_read{},fail_write{};
 std::array<std::byte,8192> bank{};
 void read(std::uint32_t at,std::span<std::byte>out)const override{if(fail_read)throw Failure{};check(at>=0x62000000u&&std::uint64_t(at-0x62000000u)+out.size()<=bank.size(),"Child read escaped bank");std::memcpy(out.data(),bank.data()+at-0x62000000u,out.size());}
 void write(std::uint32_t at,std::span<const std::byte>in)override{if(fail_write)throw Failure{};check(at>=0x62000000u&&std::uint64_t(at-0x62000000u)+in.size()<=bank.size(),"Child write escaped bank");std::memcpy(bank.data()+at-0x62000000u,in.data(),in.size());}
};
SourcePhysicsWorldRuntimeAbi abi(const raw::Image&input,unsigned count){
 SourcePhysicsWorldRuntimeAbi a{};a.x87_status=input.word(4);a.x87_depth=count;a.floating_point=SourcePhysicsFactoryFloatingPoint{input.word(0),input.mxcsr};
 for(unsigned n=0;n<count;++n)std::memcpy(&a.x87[n],input.x87.data()+28+n*10,10);
 for(unsigned n=0;n<8;++n){a.gp[n]=0x43210000u+n;for(unsigned k=0;k<4;++k)a.xmm[n][k]=0x51230000u+n*4+k;}
 a.gp[4]=0x62001000u;a.eflags=0x647;return a;
}
void state_matches(State&s,const raw::Image&expected,const SourcePhysicsWorldRuntimeAbi&before,const std::string&why){
 const auto a=s.save();check(a.floating_point->x87_control_word==expected.word(0)&&a.floating_point->mxcsr==expected.mxcsr&&a.x87_status==expected.word(4),why+" saved controls/status");
 unsigned count{};check(raw::dense(expected,count)&&a.x87_depth==count,why+" saved depth");
 for(unsigned n=0;n<count;++n)check(!std::memcmp(&a.x87[n],expected.x87.data()+28+n*10,10),why+" saved logical80");
 check(a.gp==before.gp&&a.eflags==before.eflags&&a.xmm==before.xmm,why+" changed integer/SIMD ABI");
}
raw::Image host_image(const raw::Image&source,unsigned count){
 auto result=source;result.word(4,std::uint16_t((source.word(4)&~0x3800u)|(((source.top()+3)&7u)<<11u)));std::uint16_t tags=0xffff;
 static constexpr std::array<std::array<unsigned char,10>,4> values{{{{0,0,0,0,0,0,0,0x80,0xff,0x3f}},{{0,0,0,0,0,0,0,0,0,0x80}},{{1,0,0,0,0,0,0,0xc0,0xff,0x7f}},{{1,0,0,0,0,0,0,0,0,0}}}};
 for(unsigned n=0;n<count;++n){std::memcpy(result.x87.data()+28+n*10,values[n%4].data(),10);const auto shift=((result.top()+n)&7)*2;tags=std::uint16_t((tags&~(3u<<shift))|(raw::value_tag(result.x87.data()+28+n*10)<<shift));}
 result.word(8,tags);return result;
}
// Deliberately corrupt every hardware ownership field without touching State.
void hostile(){asm volatile("fninit; fld1; fldz; fdivp; fld1":::"memory","st","st(1)");const std::uint16_t cw=0x0740;asm volatile("fldcw %0"::"m"(cw));_mm_setcsr(0x8065);}
void execute(unsigned op,std::uint32_t operand,std::uint32_t&bits){
 switch(op){
 case 0:break;
 case 1:asm volatile("flds %0"::"m"(operand):"memory","st");break;
 case 2:asm volatile("fadds %0"::"m"(operand):"memory","st");break;
 case 3:asm volatile("fdivs %0"::"m"(operand):"memory","st");break;
 case 4:asm volatile("fchs":::"memory","st");break;
 case 5:asm volatile("fsts %0":"=m"(bits)::"memory");break;
 case 6:asm volatile("fstps %0":"=m"(bits)::"memory","st");break;
 case 7:asm volatile("fsqrt":::"memory","st");break;
 case 8:asm volatile("faddp":::"memory","st","st(1)");break;
 case 9:asm volatile("movd %0,%%xmm0; divss %1,%%xmm0"::"r"(operand),"m"(operand):"memory","xmm0");break;
 case 10:asm volatile("fldcw %0"::"m"(operand):"memory");break;
 case 11:_mm_setcsr(operand);break;
 default:throw std::runtime_error("Unknown numerical oracle instruction");
 }
}
void packet(State&s,const raw::Image&expected,const SourcePhysicsWorldRuntimeAbi&before){
 state_matches(s,expected,before,"I/O packet");const auto hardware=raw::capture();check(hardware.word(8)==0xffffu&&hardware.top()==0,"Callback inherited a live hardware stack");
 check(hardware.word(0)==expected.word(0)&&hardware.word(4)==(expected.word(4)&~0x3800u)&&hardware.mxcsr==expected.mxcsr,"Callback lost source FP environment");
}
unsigned check_cases(const J&rows){
 unsigned comparisons{};
 for(const auto&r:rows){const auto before=image(r.at("before")),after=image(r.at("after"));const auto op=r.at("op").get<unsigned>(),operand=r.at("operand").get<std::uint32_t>();
  const auto name=r.at("name").get<std::string>();
  for(unsigned host_count:{0u,1u,4u,8u})for(unsigned mode=0;mode<4;++mode){
   Memory memory;SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observer;State s{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};const auto incoming=abi(before,r.at("count"));s.load(incoming);
   const auto host=host_image(before,host_count);raw::restore(host);std::uint32_t bits{};
   {
    raw::Scope scope(s);same(raw::capture(),before,name+" import");
    if(mode){scope.io([&]{packet(s,before,incoming);hostile();});same(raw::capture(),before,name+" provider restore");
     try{scope.io([&]{packet(s,before,incoming);hostile();throw Failure{};});check(false,"Throwing provider returned");}catch(const Failure&){}
     same(raw::capture(),before,name+" provider failure");
    }
    if((op==5||op==6)&&mode){
     const auto token=scope.stage_single(op==6);bits=token.bits;same(raw::capture(),before,name+" stage committed FP early");
     check(bits==r.at("bits"),name+" staged rounded bits");
     std::array<std::byte,4> written{};std::memcpy(written.data(),&bits,4);
     memory.fail_read=true;
     try{scope.io([&]{packet(s,before,incoming);hostile();std::array<std::byte,4> preimage{};memory.read(0x62000000u,preimage);});check(false,"Preimage provider returned");}catch(const Failure&){}memory.fail_read=false;
     memory.fail_write=true;
     try{scope.io([&]{packet(s,before,incoming);hostile();memory.write(0x62000000u,written);});check(false,"Store provider returned");}catch(const Failure&){}memory.fail_write=false;
     same(raw::capture(),before,name+" store provider fault changed FP");check(memory.bank[0]==std::byte{},"Failed store committed backing");
     scope.io([&]{packet(s,before,incoming);hostile();memory.write(0x62000000u,written);});scope.commit(token);same(raw::capture(),after,name+" commit");
     check(!std::memcmp(memory.bank.data(),written.data(),4),"Completed spill lost backing");
     bool refused=false;try{scope.commit(token);}catch(const std::runtime_error&){refused=true;}check(refused,"Spill token replay admitted");
     if(mode==2){try{scope.io([&]{packet(s,after,incoming);hostile();throw Failure{};});check(false,"Observer returned");}catch(const Failure&){} }
    }else execute(op,operand,bits);
    scope.publish();same(raw::capture(),after,name+" numerical poststate");
    if(mode)scope.io([&]{packet(s,after,incoming);hostile();});
    check(bits==r.at("bits"),name+" spill bits");
   }
   auto expected_host=host;expected_host.word(0,after.word(0));expected_host.word(4,std::uint16_t((after.word(4)&~0x3800u)|(host.word(4)&0x3800u)));expected_host.mxcsr=after.mxcsr;
   same(raw::capture(),expected_host,name+" caller physical register preservation");
   state_matches(s,after,incoming,name+" final State.save");same(raw::capture(),expected_host,name+" State.save changed caller stack");++comparisons;
   // End this fixture independently; the enclosing process guard owns the real caller.
   asm volatile("fninit":::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
  }
 }
 return comparisons;
}

void expected_child(State&s,const J&expected,const std::string&name){
 const auto a=s.save();check(a.gp==expected.at("gp").get<decltype(a.gp)>()&&a.eflags==expected.at("eflags")&&a.xmm==expected.at("xmm").get<decltype(a.xmm)>(),name+" child GP/flags/SIMD");
 check(a.x87_status==expected.at("fp")[1]&&a.floating_point->x87_control_word==expected.at("fp")[0]&&a.floating_point->mxcsr==expected.at("fp")[2],name+" child FP controls/status");
 check(a.x87_depth==expected.at("live_x87").size(),name+" child returned depth");
 for(unsigned n=0;n<a.x87_depth;++n)check(hex(reinterpret_cast<const std::byte*>(&a.x87[n]),10)==expected.at("live_x87")[n].get<std::string>(),name+" child logical80");
}
unsigned children(const J&rows){
 unsigned comparisons{};
 for(const auto&r:rows)for(unsigned mode=0;mode<4;++mode){
  const auto name=r.at("name").get<std::string>();Memory memory;const auto bank=unhex(r.at("bank"));check(bank.size()==memory.bank.size(),"Invalid genuine child bank");std::memcpy(memory.bank.data(),bank.data(),bank.size());
  SourcePhysicsWorldRuntimeAbi a{};a.gp=r.at("gp");a.eflags=r.at("eflags");a.xmm=r.at("xmm");a.x87_status=r.at("fp")[1];a.floating_point=SourcePhysicsFactoryFloatingPoint{r.at("fp")[0].get<std::uint16_t>(),r.at("fp")[2].get<std::uint32_t>()};a.x87_depth=r.at("live_x87").size();
  for(unsigned n=0;n<a.x87_depth;++n){const auto bytes=unhex(r.at("live_x87")[n]);std::memcpy(&a.x87[n],bytes.data(),10);}
  J stores=J::array();SourcePhysicsContactRecordObserver observer=[&](const auto&e){stores.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before.data(),e.before.size())},{"written",hex(e.written.data(),e.written.size())}});};
  SourcePhysicsWorldRuntimeServices services;unsigned calls{},phases{};bool partial=mode==3&&r.at("entry")==0x22a5c0u;
  services.required_owner=[&](auto&memory,const auto&call,auto&transport,const auto&observe){
   check(call.target==r.at("entry"),"Changed actual child target");++calls;
   SourcePhysicsWorldRuntimeServices child_services;State child{memory,observe,child_services,SourcePhysicsContactImage::TitleUpdate15};child.load(transport);
   const auto run_child=[&]{
    if(call.target==0x22a5c0u)check(native_tick_conversion_detail::convert(child,[&](auto&state,auto pc){++phases;if(partial&&pc==0x22a615u)throw Failure{};}),"Actual Tick child refused");
    else native_tick_conversion_detail::multiply(child);
   };
   try {
    if(mode==1){raw::Scope nested(child);nested.child(run_child);}else run_child();
    transport=child.save();if(mode==2)throw Failure{};
   }catch(...){transport=child.save();throw;}
  };
  State state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};state.load(a);
  const auto old_host=raw::capture();const auto host=host_image(old_host,mode==0?0:mode==1?1:mode==2?4:8);raw::restore(host);
  {
   raw::Scope scope(state);bool caught=false;
   try{scope.child([&]{state.required(0x30b05fu,r.at("entry"),false);});}catch(const Failure&){caught=true;}
   check(caught==(mode==2||partial),"Actual child exception transport changed");
   const auto&expected=r.at(partial?"partial":"after");scope.io([&]{expected_child(state,expected,name);hostile();});
   if(mode==2||partial)check(caught,"Throwing genuine child was not observed");
  }
  const auto&expected=r.at(partial?"partial":"after");expected_child(state,expected,name+" final");
  check(hex(memory.bank.data(),memory.bank.size())==expected.at("bank").get<std::string>(),name+" child banks");check(stores==expected.at("stores"),name+" child ordered stores");
  const auto actual=raw::capture();check(actual.word(8)==host.word(8)&&actual.top()==host.top(),name+" child caller topology");
  for(unsigned n=0;n<8;++n)if(host.tag(n)!=3)check(!std::memcmp(actual.x87.data()+28+n*10,host.x87.data()+28+n*10,10),name+" child caller payload");
  check(calls==1&&(r.at("entry")==0x22a5c0u?phases>0:phases==0),"Genuine child route was bypassed");++comparisons;
  asm volatile("fninit":::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
 }
 return comparisons;
}


unsigned outer_scope(){
 unsigned comparisons{};
 for(unsigned host_count:{0u,1u,4u,8u}){
  auto host=host_image(raw::capture(),host_count);host.word(0,0x037f);host.word(4,std::uint16_t((host.word(4)&0x3800u)|0x4500u));host.mxcsr=0x1f80;raw::restore(host);
  {
   physics_step_detail::FpScope outer({0x0a7f,0xdfa1});
   Memory memory;SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observer;State state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};
   auto input=host_image(raw::capture(),1);input.word(0,0x0a7f);input.word(4,std::uint16_t((input.word(4)&0x3800u)|0x0100));input.mxcsr=0xdfa1;const auto incoming=abi(input,1);state.load(incoming);raw::Image after;
   {raw::Scope scope(state);std::uint32_t ignored{};execute(3,0x40400000,ignored);execute(9,0,ignored);after=raw::capture();scope.io([&]{packet(state,after,incoming);hostile();});}
   // This is the actual runtime ordering: native scope destruction, State.save,
   // then the outer runtime FP scope restores its caller's environment.
   state_matches(state,after,incoming,"Outer runtime save");check(after.word(0)==0x0a7f&&after.mxcsr==0xdfa1&&(after.word(4)&0x20u),"Outer save fixture did not raise precision");
  }
  const auto returned=raw::capture();
  // fenv_t restores standard FP controls/exceptions; the existing runtime
  // scope does not promise restoration of caller-saved x87 condition bits.
  auto expected=host;expected.word(4,std::uint16_t((host.word(4)&~0x4700u)|(returned.word(4)&0x4700u)));
  same(returned,expected,"Outer runtime failed to restore its caller stack/controls/exceptions");++comparisons;
 }
 return comparisons;
}

unsigned refusals(){
 Memory memory;SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observer;State s{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};
 SourcePhysicsWorldRuntimeAbi a{};a.floating_point=SourcePhysicsFactoryFloatingPoint{0x37f,0x1f80};s.load(a);unsigned count{};
 const auto refuse=[&]{const auto old=raw::capture();check(!raw::supported(s),"Unsupported FP admitted");bool threw=false;try{raw::Scope scope(s);}catch(const std::runtime_error&){threw=true;}check(threw,"Unsupported FP constructed");same(raw::capture(),old,"Refusal changed hardware");++count;};
 s.fp.count=9;refuse();s.fp.count=0;s.fp.top=8;refuse();s.fp.top=0;s.fp.status=0x80;refuse();s.fp.status=0x8000;refuse();s.fp.status=0;s.active_mmx=SourcePhysicsMmxAlias{};refuse();s.active_mmx.reset();
 auto old=raw::capture(),bad=old;bad.word(0,std::uint16_t(old.word(0)&~1u));raw::restore(bad);refuse();raw::restore(old);
 bad=old;bad.mxcsr&=~0x80u;raw::restore(bad);refuse();raw::restore(old);
 {raw::Scope scope(s);bool threw=false;try{scope.stage_single(true);}catch(const std::runtime_error&){threw=true;}check(threw,"Empty spill admitted");++count;scope.require_capacity(8);try{scope.require_capacity(9);check(false,"Overflow capacity admitted");}catch(const std::runtime_error&){}++count;}
 {
  const auto host=host_image(raw::capture(),8);raw::restore(host);bool caught=false;
  {raw::Scope scope(s);try{scope.child([&]{s.active_mmx=SourcePhysicsMmxAlias{};throw Failure{};});}catch(const Failure&){caught=true;}}
  check(caught&&bool(s.active_mmx),"Unsupported throwing child lost original exception or retained outcome");same(raw::capture(),host,"Unsupported throwing child lost host stack");s.active_mmx.reset();++count;
 }
 {
  const auto host=host_image(raw::capture(),4);raw::restore(host);bool caught=false;
  {raw::Scope scope(s);try{scope.child([&]{s.fp.count=9;});}catch(const std::runtime_error&){caught=true;}}
  check(caught&&s.fp.count==9,"Unsupported returned child was normalized");same(raw::capture(),host,"Unsupported returned child lost host stack");s.fp.count=0;++count;
 }
 {
  auto hole=host_image(raw::capture(),4);const auto shift=((hole.top()+1u)&7u)*2u;hole.word(8,std::uint16_t(hole.word(8)|(3u<<shift)));unsigned depth{};
  check(!raw::dense(hole,depth),"Sparse physical tags admitted");++count;
 }
 {
  const auto input=host_image(raw::capture(),1);const auto incoming=abi(input,1);s.load(incoming);raw::Scope scope(s);const auto old=scope.stage_single(true);
  {raw::Scope nested(s);bool caught=false;try{nested.commit(old);}catch(const std::runtime_error&){caught=true;}check(caught,"Foreign spill token admitted");++count;}
  scope.child([]{});bool caught=false;try{scope.commit(old);}catch(const std::runtime_error&){caught=true;}check(caught,"Pre-child spill token admitted");++count;
  const auto stale=scope.stage_single(true);asm volatile("fchs":::"memory","st");caught=false;try{scope.commit(stale);}catch(const std::runtime_error&){caught=true;}check(caught,"Numerically stale spill token admitted");++count;
 }
 return count;
}
}
int main(int argc,char**argv)try{
 const auto caller=raw::capture();check(argc==2,"Supply physical x87 instruction fixture");std::ifstream in(argv[1]);const auto fixture=J::parse(in);const auto comparisons=check_cases(fixture.at("cases"));const auto child_comparisons=children(fixture.at("children"));const auto outer_comparisons=outer_scope();const auto rejected=refusals();raw::restore(caller);
 std::cout<<J{{"comparisons",comparisons},{"genuine_child_comparisons",child_comparisons},{"outer_runtime_comparisons",outer_comparisons},{"refusals",rejected},{"passed",true}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
