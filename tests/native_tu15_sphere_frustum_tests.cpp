#include "rendering/native_tu15_sphere_frustum_abi.hpp"
#include "halo2/native_tu15_sphere_frustum_value.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_sse_mmx_scope.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <map>
using namespace halo2;
using J=nlohmann::json;
using State=physics_world_runtime_detail::WorldState;
struct InstrumentedState:State {
 using Base=physics_world_runtime_detail::WorldState;using Base::Base;unsigned literal_calls{};
 void instruction_literal(std::uint32_t pc){++literal_calls;Base::instruction_literal(pc);}
};
namespace copies {
using namespace physics_world_runtime_detail;
using WorldState=InstrumentedState;
#include "sphere_frustum_bodies.inc"
}
namespace world_reference {
#include "native_tu15_sphere_frustum_value_reference.inc"
}
namespace {
struct Fault{};
void check(bool ok,const std::string&why){if(!ok)throw std::runtime_error(why);}
std::vector<std::byte>unhex(const std::string&s){std::vector<std::byte>v;for(unsigned n=0;n<s.size();n+=2)v.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return v;}
std::string hex(std::span<const std::byte>v){constexpr char d[]="0123456789abcdef";std::string s;for(const auto b:v){const auto n=std::to_integer<unsigned>(b);s+=d[n>>4];s+=d[n&15];}return s;}
void hostile(){const std::uint16_t unmasked=0x037e;asm volatile("fninit;fldz;fldz;fdivp;pxor %%mm0,%%mm0;pxor %%mm1,%%mm1;pxor %%mm2,%%mm2;pxor %%mm3,%%mm3;pxor %%mm4,%%mm4;pxor %%mm5,%%mm5;pxor %%mm6,%%mm6;pxor %%mm7,%%mm7;fldcw %0"::"m"(unmasked):"memory","st","st(1)","mm0","mm1","mm2","mm3","mm4","mm5","mm6","mm7");_mm_setcsr(0x8065);}
J packet(State&s,std::uint32_t pc){const auto a=s.save();J live=J::array();for(unsigned n=0;n<a.x87_depth;++n)live.push_back(hex(std::span(reinterpret_cast<const std::byte*>(&a.x87[n]),10)));J result{{"entry",pc},{"gp",a.gp},{"eflags",a.eflags},{"xmm",a.xmm},{"fp",J::array({a.floating_point->x87_control_word,a.x87_status,a.floating_point->mxcsr})},{"live_x87",live}};if(a.active_mmx){J physical=J::array();unsigned tag{};for(unsigned n=0;n<8;++n){const auto&raw=a.active_mmx->physical[n];physical.push_back(hex(raw));tag|=native_raw_x87_detail::value_tag(raw.data())<<(n*2u);}result["active_mmx"]=J{{"tag_word",tag},{"physical",physical},{"known_mask",a.active_mmx->known_mask}};}return result;}
native_raw_x87_detail::Image host_image(const J&row,unsigned mode){
 auto image=native_raw_x87_detail::capture();const auto top=std::array{5u,3u,7u,2u,0u,6u,1u}[mode];
 image.word(4,std::uint16_t((row.at("before_fp")[1].get<unsigned>()&0x3fu)|0x4500u|(top<<11u)));image.word(8,0xffffu);
 const auto count=std::array{0u,1u,4u,8u,8u,0u,1u}[mode];std::uint16_t tags=0xffffu;
 for(unsigned n=0;n<8;++n){
  const auto raw=unhex(row.at("active_mmx").at("physical")[n]);std::memcpy(image.x87.data()+28+n*10,raw.data(),10);
  if(n<count)tags=std::uint16_t(tags&~(3u<<(((top+n)&7u)*2u)));
 }
 if(mode==5)tags=0xcf3fu;image.word(8,tags);return image;
}
struct Memory:SourcePhysicsContactMemory {
 std::map<std::uint32_t,std::vector<std::byte>>banks;std::uint32_t pc{};mutable J reads=J::array();std::optional<unsigned>fault;bool noisy{};std::function<void()>before_read;
 void read(std::uint32_t at,std::span<std::byte>out)const override{
  if(before_read)before_read();
  if(fault&&reads.size()==*fault){if(noisy)hostile();throw Fault{};}
  reads.push_back(J{{"instruction",pc},{"address",at},{"bytes",out.size()}});auto it=banks.upper_bound(at);check(it!=banks.begin(),"Unmapped sphere operand");--it;check(std::uint64_t(at-it->first)+out.size()<=it->second.size(),"Sphere operand escaped backing");std::memcpy(out.data(),it->second.data()+at-it->first,out.size());if(noisy)hostile();
 }
 void write(std::uint32_t,std::span<const std::byte>)override{throw std::runtime_error("Sphere classifier attempted a store");}
};

struct GuardState:State {using Base=physics_world_runtime_detail::WorldState;using Base::Base;bool selected_observation_pcs{true};};
unsigned refusals(bool reference_only){
 unsigned count{};constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u},changed{0x38846eu,0x184690u,0x33b595u};
 for(unsigned kind=0;kind<(reference_only?1u:24u);++kind){
  Memory memory;unsigned instructions{};bool first{},second{};SourcePhysicsWorldRuntimeServices services;
  services.instruction=[&](auto){++instructions;};auto&cursor=services.instruction_cursor;
  cursor={&memory.pc,&first,&second,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
  SourcePhysicsContactRecordObserver observe;if(kind==14||kind==15)observe=[](const auto&){};
  if(!reference_only&&kind==14)cursor={};
  if(!reference_only&&kind==21)cursor.literal_service_sites=false;
  GuardState state{memory,observe,services,!reference_only&&kind==0?SourcePhysicsContactImage::MapPack11:SourcePhysicsContactImage::TitleUpdate15};SourcePhysicsWorldRuntimeAbi input{};input.floating_point=SourcePhysicsFactoryFloatingPoint{0x37f,0x1f80};state.load(input);
  if(!reference_only){
   if(kind==1){state.active_mmx=SourcePhysicsMmxAlias{};state.fp.count=1;}
   if(kind==2)state.fp.count=9;if(kind==3)state.fp.top=8;
   if(kind==4){const std::uint16_t cw=0x37e;asm volatile("fldcw %0"::"m"(cw));}
   if(kind==5)_mm_setcsr(0x1f00);
   if(kind==6)cursor={};if(kind==7)cursor.position=nullptr;
   if(kind==8)cursor.executor={};if(kind==9)cursor.callback_type=&typeid(int);
   if(kind==10)first=true;if(kind==11)second=true;
   if(kind==12)cursor.service_sites=changed;if(kind==13)cursor.literal_service_sites=false;
   if(kind==15)cursor.callback_type=&typeid(int);
   if(kind==16){const auto original=services.instruction;services.instruction=[original](auto pc){original(pc);};}
   if(kind==17)state.selected_observation_pcs=false;
   if(kind==18)state.fp.status=0x80;if(kind==19)state.fp.status=0x8000;
   if(kind==20)state.instruction_cursor=nullptr;
   
   if(kind==22)cursor.service_sites={};
   if(kind==23)services.instruction={};
  }
  const auto hardware=native_raw_x87_detail::capture();const auto gp=state.gp;const auto flags=state.flags.bits();const auto depth=state.fp.count,top=state.fp.top;const auto status=state.fp.status;std::array<std::byte,128>xmm{};std::memcpy(xmm.data(),state.xmm,128);memory.pc=0xdeadbeefu;
  check(!native_sphere_frustum_abi_detail::bridge(state),"Unqualified sphere view admitted");check(gp==state.gp&&flags==state.flags.bits()&&depth==state.fp.count&&top==state.fp.top&&status==state.fp.status&&!std::memcmp(xmm.data(),state.xmm,128),"Sphere refusal changed retained state");check(memory.pc==0xdeadbeefu&&memory.reads.empty()&&!instructions&&native_raw_x87_detail::equivalent(hardware,native_raw_x87_detail::capture()),"Sphere refusal changed hardware/protocol");++count;
  asm volatile("fninit":::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(0x1f80);
 }
 return count;
}
unsigned stale_clear_regressions(const J&fixture){
 unsigned count{};
 for(const auto&row:fixture.at("cases")){
  if(row.contains("fault")||(row.at("name")!="family-18-variant-0"&&row.at("name")!="mmx-family-18-variant-0"))continue;
  J expected,reads;
  for(unsigned route=0;route<3;++route){const bool bound=route!=0;
   Memory memory;for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),unhex(bank.at("before")));
   unsigned callbacks{};bool first{},second{};constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
   SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto){++callbacks;};services.instruction_cursor={&memory.pc,&first,&second,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
   SourcePhysicsContactRecordObserver observer;InstrumentedState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};SourcePhysicsWorldRuntimeAbi input{};
   input.gp=row.at("gp");input.eflags=row.at("eflags");input.xmm=row.at("xmm");input.x87_status=row.at("before_fp")[1];input.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};input.x87_depth=row.at("live_x87").size();
   for(unsigned n=0;n<input.x87_depth;++n){const auto value=unhex(row.at("live_x87")[n]);std::memcpy(&input.x87[n],value.data(),10);}
   if(row.contains("active_mmx")){SourcePhysicsMmxAlias alias{};alias.known_mask=row.at("active_mmx").at("known_mask");for(unsigned n=0;n<8;++n){const auto value=unhex(row.at("active_mmx").at("physical")[n]);std::memcpy(alias.physical[n].data(),value.data(),10);}input.active_mmx=alias;}
   state.load(input);check(state.instruction_cursor&&state.literal_service_sites,"No authentic cached cursor witness");services.instruction={};memory.pc=0xdeadbeefu;
   if(route==2)copies::specialized::run_184690(state);else if(bound)copies::bound::run_184690(state);else copies::retained::run_184690(state);
   check(!callbacks&&memory.pc==0xdeadbeefu,"Cached-cleared sphere published a cursor unlike the exact retained body");check(route==2?state.literal_calls>0:state.literal_calls==0,"Cached-clear retained route evidence differs");
   const auto result=packet(state,memory.pc);if(bound)check(result==expected&&memory.reads==reads,"Cached-cleared bound fallback changed ABI/read order");else{expected=result;reads=memory.reads;}
   for(const auto&bank:row.at("banks"))check(hex(memory.banks.at(bank.at("address")))==bank.at("before").get<std::string>(),"Cached-clear changed read-only DATA");
  }
  ++count;
 }
 check(count>0,"Missing cached-clear source input");return count;
}
unsigned mmx_refusals(){
 unsigned count{};
 for(unsigned kind=0;kind<7;++kind){
  Memory memory;SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observe;
  State state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};SourcePhysicsWorldRuntimeAbi input{};input.floating_point=SourcePhysicsFactoryFloatingPoint{0x37f,0x1f80};input.active_mmx=SourcePhysicsMmxAlias{};state.load(input);
  if(kind==0)state.fp.count=1;if(kind==1)state.fp.top=1;if(kind==2)state.active_mmx.reset();
  if(kind==3)state.fp.status=0x80;if(kind==4)state.fp.status=0x8000;
  if(kind==5){const std::uint16_t cw=0x37e;asm volatile("fldcw %0"::"m"(cw));}
  if(kind==6)_mm_setcsr(0x1f00);
  const auto before=native_raw_x87_detail::capture();check(!native_sse_mmx_detail::supported(state),"Unqualified opaque MMX owner admitted");bool rejected{};
  try{native_sse_mmx_detail::Scope scope{state};}catch(const std::runtime_error&){rejected=true;}
  check(rejected&&native_raw_x87_detail::equivalent(before,native_raw_x87_detail::capture())&&memory.reads.empty(),"Opaque MMX refusal changed host FP/protocol");++count;
  asm volatile("fninit":::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(0x1f80);
 }
 return count;
}

}
int main(int argc,char**argv)try{
 if(argc==3&&std::string_view(argv[1])=="--cached-clear"){std::ifstream in(argv[2]);const auto fixture=J::parse(in);std::cout<<J{{"cached_clear_source_regressions",stale_clear_regressions(fixture)}}.dump()<<'\n';return 0;}
 if(argc==2&&std::string_view(argv[1])=="--refusals"){std::cout<<J{{"refusals",refusals(false)},{"opaque_mmx_owner_refusals",mmx_refusals()}}.dump()<<'\n';return 0;}
 if(argc==2&&std::string_view(argv[1])=="--reference-guard"){std::cout<<J{{"reference_refusals",refusals(true)}}.dump()<<'\n';return 0;}
 check(argc==2,"Supply original sphere classification fixture");std::ifstream in(argv[1]);const auto fixture=J::parse(in);unsigned comparisons{},packets{},value_regressions{},default_calls{},reference_calls{},reference_callbacks{},reference_literals{};
 const bool forced=std::getenv("HALO2_SPHERE_FRUSTUM_REFERENCE_TEST");const bool candidate=std::getenv("HALO2_SPHERE_FRUSTUM_MMX_CANDIDATE");unsigned mmx_comparisons{},host_opaque_checks{};
 const auto stale_clear_cases=forced?0u:stale_clear_regressions(fixture);
 for(const auto&row:fixture.at("cases"))for(unsigned mode=forced?5u:0u;mode<7;++mode){
  const bool mmx=row.contains("active_mmx");if(candidate&&mmx&&!forced&&mode>=5)continue;
  if(forced&&row.at("name")!="family-18-variant-0"&&row.at("name")!="mmx-family-18-variant-0")continue;
  Memory memory;for(const auto&bank:row.at("banks"))memory.banks.emplace(bank.at("address"),unhex(bank.at("before")));if(row.contains("fault"))memory.fault=row.at("fault").at("ordinal");memory.noisy=mode==1;
  SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observer;unsigned instruction_calls{};bool trace{};constexpr std::array service_sites{0x38846eu,0x21f590u,0x33b595u};
  if(mode==3||mode>=5){services.instruction=[&](auto pc){memory.pc=pc;++instruction_calls;};services.instruction_cursor={&memory.pc,&trace,nullptr,service_sites,std::this_thread::get_id(),&services.instruction.target_type(),true,false};}
  if(mode==3||mode==4)observer=[](const auto&){throw std::runtime_error("Store-free sphere invoked its observer");};
  InstrumentedState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};SourcePhysicsWorldRuntimeAbi a{};a.gp=row.at("gp");a.eflags=row.at("eflags");a.xmm=row.at("xmm");a.x87_status=row.at("before_fp")[1];a.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};a.x87_depth=row.at("live_x87").size();for(unsigned n=0;n<a.x87_depth;++n){const auto raw=unhex(row.at("live_x87")[n]);std::memcpy(&a.x87[n],raw.data(),10);}if(mmx){SourcePhysicsMmxAlias alias{};alias.known_mask=row.at("active_mmx").at("known_mask");for(unsigned n=0;n<8;++n){const auto raw=unhex(row.at("active_mmx").at("physical")[n]);std::memcpy(alias.physical[n].data(),raw.data(),10);}a.active_mmx=alias;}state.load(a);
  auto caller=native_raw_x87_detail::capture();if(mmx&&!forced){caller=host_image(row,mode);native_raw_x87_detail::restore(caller);caller=native_raw_x87_detail::capture();}
  J observed=J::array();bool faulted{};
  const auto provider_check=[&]{if(mmx&&!forced){const auto current=native_raw_x87_detail::capture();check(current.word(8)==0xffffu&&!current.top(),"MMX provider did not borrow an empty host x87 file");check(current.word(0)==row.at("before_fp")[0]&&current.word(4)==row.at("before_fp")[1],"MMX provider source x87 control/status differs");}};
  memory.before_read=[&]{provider_check();if(mode>=5)observed.push_back(packet(state,memory.pc));};
  const auto before=[&](auto&s,auto pc){memory.pc=pc;provider_check();observed.push_back(packet(s,pc));if(mode==2)hostile();};
  try{
   if(mode==5)copies::bound::run_184690(state);
   else if(mode==6)copies::specialized::run_184690(state);
   else if(mmx&&candidate){check(native_sphere_frustum_abi_detail::policy_admitted(state)&&native_sse_mmx_detail::supported(state),"Opaque MMX candidate refused");native_sse_mmx_detail::Scope scope{state};native_sphere_frustum_abi_detail::Frame frame{state,before,scope};native_sphere_frustum_detail::classify(frame);}
   else check(native_sphere_frustum_abi_detail::bridge(state,before),"Private native sphere refused");
  }catch(const Fault&){faulted=true;}
  if(forced){check(instruction_calls+state.literal_calls>0,"Reference control skipped retained body");if(mode==6)check(state.literal_calls>0,"Specialized reference control skipped literal body");reference_callbacks+=instruction_calls;reference_literals+=state.literal_calls;++reference_calls;}
  else {check(!instruction_calls&&!state.literal_calls,"Native sphere evaluated instruction callbacks/literals");if(mode>=5)++default_calls;}
  check(faulted==row.contains("fault"),"Sphere read-fault completion changed");const auto name=row.at("name").get<std::string>()+" mode="+std::to_string(mode);
  check(observed==row.at("calls"),name+": sphere pre-read packets differ actual="+observed.dump()+" expected="+row.at("calls").dump());
  auto expected=J{{"entry",row.at("expected_pc")},{"gp",row.at("boundary_gp")},{"eflags",row.at("boundary_eflags")},{"xmm",row.at("boundary_xmm")},{"fp",row.at("after_fp")},{"live_x87",row.at("live_x87")}};if(mmx)expected["active_mmx"]=row.at("boundary_active_mmx");
  check(packet(state,memory.pc)==expected,name+": sphere final/prefix ABI differs actual="+packet(state,memory.pc).dump()+" expected="+expected.dump());check(memory.reads==row.at("reads"),name+": sphere ordered reads differ");
  for(const auto&bank:row.at("banks"))check(hex(memory.banks.at(bank.at("address")))==bank.at("after").get<std::string>(),"Sphere read-only backing changed");++comparisons;packets+=observed.size();
  if(mmx){++mmx_comparisons;if(!forced){auto wanted=caller;wanted.word(0,row.at("after_fp")[0]);wanted.word(4,std::uint16_t((row.at("after_fp")[1].get<unsigned>()&~0x3800u)|(caller.word(4)&0x3800u)));wanted.mxcsr=row.at("after_fp")[2];const auto actual=native_raw_x87_detail::capture();check(actual.word(0)==wanted.word(0)&&actual.word(4)==wanted.word(4)&&actual.word(8)==wanted.word(8)&&actual.mxcsr==wanted.mxcsr&&!std::memcmp(actual.x87.data()+28,wanted.x87.data()+28,80),name+": full opaque caller FP file/source handoff changed");++host_opaque_checks;}asm volatile("fninit":::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(0x1f80);}
  if(!mmx&&!mode&&!row.contains("fault")){
   const auto incoming_gp=row.at("gp").get<std::array<std::uint32_t,8>>();const auto get=[&](std::uint32_t address){auto it=memory.banks.upper_bound(address);check(it!=memory.banks.begin(),"World value escaped fixture");--it;std::uint32_t bits{};std::memcpy(&bits,it->second.data()+address-it->first,4);return std::bit_cast<float>(bits);};
   SourceSphereFrustum frustum;for(unsigned n=0;n<6;++n)frustum.bounds[n]=get(incoming_gp[0]+0x60u+n*4u);for(unsigned n=0;n<6;++n)for(unsigned k=0;k<4;++k)frustum.planes[n][k]=get(incoming_gp[0]+0x78u+n*16u+k*4u);
   const Vec3 center{get(incoming_gp[1]),get(incoming_gp[1]+4u),get(incoming_gp[1]+8u)};const auto radius=std::bit_cast<float>(row.at("xmm")[1][0].get<std::uint32_t>());
   auto seed=native_raw_x87_detail::capture();seed.mxcsr=row.at("before_fp")[2];seed.word(0,row.at("before_fp")[0]);seed.word(4,row.at("before_fp")[1]);native_raw_x87_detail::restore(seed);
   const auto reference=world_reference::source_sphere_frustum_visibility_reference(frustum,center,radius);const auto reference_fp=native_raw_x87_detail::capture();native_raw_x87_detail::restore(seed);
   const auto shared=native_tu15_sphere_frustum_value(frustum,center,radius);const auto shared_fp=native_raw_x87_detail::capture();
   check(reference==shared&&std::uint32_t(shared)==row.at("boundary_gp")[0],name+": world native value classification changed");check(native_raw_x87_detail::equivalent(reference_fp,shared_fp),name+": world native value FP effects changed");++value_regressions;
  }
 }
 std::cout<<J{{"passed",true},{"comparisons",comparisons},{"complete_pre_io_packets",packets},{"default_calls",default_calls},{"reference_calls",reference_calls},{"reference_callbacks",reference_callbacks},{"reference_literal_evaluations",reference_literals},{"world_native_native_regressions",value_regressions},{"mmx_comparisons",mmx_comparisons},{"opaque_host_checks",host_opaque_checks},{"cached_clear_source_regressions",stale_clear_cases}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
