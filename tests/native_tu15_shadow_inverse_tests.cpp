#include "halo2/native_tu15_shadow_inverse_binding.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <xmmintrin.h>
namespace {
using namespace halo2;using J=nlohmann::json;
void check(bool v,const std::string &s){if(!v)throw std::runtime_error(s);}
std::vector<std::byte> raw(const std::string &s){std::vector<std::byte>b;for(std::size_t n=0;n<s.size();n+=2)b.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return b;}
std::string hex(std::span<const std::byte>b){constexpr char d[]="0123456789abcdef";std::string s;for(auto v:b){const auto n=std::to_integer<unsigned>(v);s+=d[n>>4];s+=d[n&15];}return s;}
struct Cut{};
struct Memory final:SourcePhysicsContactMemory {
 std::map<std::uint32_t,std::vector<std::byte>> banks;bool recording{};std::string phase;unsigned ordinal{};mutable unsigned attempted_reads{};unsigned attempted_writes{};
 mutable J reads=J::array(),accesses=J::array();std::vector<SourcePhysicsContactRecordStore> stores;mutable std::optional<std::pair<std::uint32_t,std::size_t>> failure;
 auto find(std::uint32_t p,std::size_t n)const {
  auto i=banks.upper_bound(p);if(i!=banks.begin()){--i;if(std::uint64_t(p-i->first)+n<=i->second.size())return i;}
  failure={{p,n}};throw std::runtime_error("Missing native shadow inverse storage");
 }
 void read(std::uint32_t p,std::span<std::byte>out)const override {
  if(phase=="before_read"&&attempted_reads++==ordinal)throw Cut{};const auto i=find(p,out.size());
  if(recording){reads.push_back(J::array({p,out.size()}));accesses.push_back(J::array({"read",p,out.size()}));}
  std::copy_n(i->second.begin()+p-i->first,out.size(),out.begin());
 }
 void write(std::uint32_t p,std::span<const std::byte>in)override {
  if(phase=="before_write"&&attempted_writes++==ordinal)throw Cut{};const auto i=find(p,in.size());check(p!=0x46a0d0u,"Shadow inverse wrote original readonly scalar");
  if(recording)accesses.push_back(J::array({"write",p,in.size()}));
  stores.push_back({0,p,{i->second.begin()+p-i->first,i->second.begin()+p-i->first+in.size()},{in.begin(),in.end()}});
  std::copy(in.begin(),in.end(),banks.at(i->first).begin()+p-i->first);
 }
};
SourcePhysicsWorldRuntimeAbi abi(const J &row) {
 SourcePhysicsWorldRuntimeAbi a{};a.gp=row.at("gp").get<decltype(a.gp)>();a.xmm=row.at("xmm").get<decltype(a.xmm)>();a.eflags=row.at("eflags");a.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};a.x87_status=row.at("before_fp")[1];
 const auto mask=row.at("before_x87").at("occupied").get<unsigned>();check((mask&(mask+1u))==0,"Caller did not supply an ordinary dense x87 frame");while(a.x87_depth<8&&(mask&(1u<<a.x87_depth))){const auto b=raw(row.at("before_x87").at("values")[a.x87_depth]);std::memcpy(&a.x87[a.x87_depth],b.data(),10);++a.x87_depth;}return a;
}
void install(Memory &m,const J &row,bool return_word=true) {
 for(const auto &b:row.at("banks"))m.banks.emplace(b.at("address"),raw(b.at("before")));
 const bool unmapped_return=row.value("actual_read_fault",false)&&row.at("boundary").at("address")==row.at("gp")[4];
 if(return_word&&!unmapped_return)m.banks.emplace(row.at("gp")[4],raw(row.at("real_return")));
}
struct HostX87 {
 alignas(16) std::array<std::byte,108> old{},expected{};
 HostX87(){asm volatile("fnsave %0\n\tfninit\n\tfld1\n\tfldz\n\tfnsave %1\n\tfrstor %1":"=m"(old),"=m"(expected)::"memory","st","st(1)");}
 ~HostX87(){asm volatile("frstor %0"::"m"(old):"memory","st","st(1)");}
 void verify()const {
  alignas(16)std::array<std::byte,108>now{};asm volatile("fnsave %0\n\tfrstor %0":"=m"(now)::"memory","st","st(1)");
  const auto word=[](const auto &image,unsigned n){return std::to_integer<unsigned>(image[n])|(std::to_integer<unsigned>(image[n+1])<<8u);};
  check(word(now,0)==word(expected,0)&&word(now,4)==word(expected,4)&&word(now,8)==word(expected,8),"Shadow inverse leaked host x87 policy/status/tags");
  const auto top=(word(expected,4)>>11u)&7u,tags=word(expected,8);
  for(unsigned n=0;n<8;++n)if(((tags>>(((top+n)&7u)*2u))&3u)!=3u)
   check(std::equal(now.begin()+28u+n*10u,now.begin()+38u+n*10u,expected.begin()+28u+n*10u),"Shadow inverse leaked occupied host x87 value");
  // Runtime exception unwinding may clear last-instruction metadata. Neither
  // the retained ABI nor the authenticated caller observes FIP/FDP or dead
  // unoccupied register bytes; they are not arithmetic/policy outputs.
 }
};
}
int main(int argc,char **argv)try {
 check(argc==2,"Supply genuine original shadow inverse fixture");std::ifstream input(argv[1]);const auto f=J::parse(input);
 check(f.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&f.at("backend")=="physical-unchanged-complete-C3DC0-caller"&&f.at("instruction_patches").empty()&&!f.at("supplied_original_outcomes").get<bool>(),"Foreign shadow caller authority");
 const SourceShadowProfileDirect profile{SourcePhysicsContactImage::TitleUpdate15,{0xc4879u,0x144090u,false}};
 bool normal{},early_width{},early_height{},loop{};for(const auto &p:f.at("parent_runs")){if(p.at("name")=="normal")normal=p.at("normal").get<unsigned>()>0;if(p.at("name")=="early_width")early_width=p.at("normal")==0&&p.at("early")==1;if(p.at("name")=="early_height")early_height=p.at("normal")==0&&p.at("early")==1;if(p.at("name")=="next_loop")loop=p.at("frames")==6&&p.at("next_loop")==5;}
 check(normal&&early_width&&early_height&&loop,"Missing actual original caller branches");unsigned controls{};for(const auto &c:f.at("liveness_controls")){check(c.at("expected").get<bool>()&&c.at("semantic_match").get<bool>()==(c.at("control")!="owed_data"),"Original continuation control was not discriminating");++controls;}check(controls==18,"Missing caller liveness controls");
 unsigned count{},prefixes{},reads{},writes{},actual_faults{};bool fresh_fp{};
 for(const auto &row:f.at("cases"))for(bool recording:{false,true}) {
  check(row.at("physical_verified").get<bool>()&&row.at("real_return")=="7e480c00","Not an actual original CALL frame");Memory m;install(m,row);auto a=abi(row);const auto before=a;const auto fault=row.value("boundary",J{});m.phase=fault.is_null()?"":fault.value("phase",std::string{});m.recording=recording;
  if(!m.phase.empty()) {
   const auto &events=row.at(m.phase=="before_read"?"reads":"writes");m.ordinal=events.size();
   // Before-read/write boundaries exclude the rejected access; after-write
   // boundaries include their already committed store.
   if(m.phase=="after_write")--m.ordinal;
  }
  if(recording&&m.phase=="after_write")continue;
  SourcePhysicsContactRecordObserver observer;unsigned published{};
  if(m.phase=="after_write")observer=[&](const auto&){if(published++==m.ordinal)throw Cut{};};
  const auto host_mxcsr=_mm_getcsr();HostX87 host;bool stopped{};
  try{native_tu15_shadow_inverse_run(m,profile,a,observer);}catch(const Cut&){stopped=true;}
  catch(const std::runtime_error&){check((row.value("actual_read_fault",false)||row.value("actual_write_fault",false))&&m.failure&&m.failure->first==row.at("boundary").at("address")&&m.failure->second==row.at("boundary").at("bytes"),row.at("name").get<std::string>()+": actual shadow inverse fault differs");stopped=true;++actual_faults;}
  host.verify();check(_mm_getcsr()==host_mxcsr,"Shadow inverse leaked host MXCSR");const auto name=row.at("name").get<std::string>();check(stopped==!row.at("completed").get<bool>(),name+": shadow inverse completion differs");
  for(unsigned n:{3u,5u,6u,7u})check(a.gp[n]==(stopped?before.gp[n]:row.at("after_gp")[n].get<std::uint32_t>()),name+": shadow inverse callee-saved state differs");
  check(a.gp[4]==before.gp[4]+(stopped?0u:4u),name+": shadow inverse real RET differs");
  check(a.xmm[6]==row.at("after_xmm")[6].get<std::array<std::uint32_t,4>>()&&a.xmm[7]==row.at("after_xmm")[7].get<std::array<std::uint32_t,4>>(),name+": source-preserved SIMD state differs");
  check(a.floating_point->x87_control_word==row.at("after_fp")[0]&&a.x87_status==row.at("after_fp")[1]&&a.floating_point->mxcsr==row.at("after_fp")[2],name+": shadow inverse FP differs");fresh_fp|=a.floating_point->mxcsr!=before.floating_point->mxcsr;
  check(((1u<<a.x87_depth)-1u)==row.at("after_x87").at("occupied"),name+": source x87 occupancy changed");for(unsigned n=0;n<a.x87_depth;++n)check(hex(std::span(reinterpret_cast<const std::byte*>(&a.x87[n]),10))==row.at("after_x87").at("values")[n].get<std::string>(),name+": source live x87 changed");
  for(const auto &b:row.at("banks"))check(m.banks.at(b.at("address"))==raw(b.at("after")),name+": shadow inverse retained bytes differ");
  check(m.stores.size()==row.at("writes").size(),name+": shadow inverse store count differs");for(unsigned n=0;n<m.stores.size();++n){const auto &actual=m.stores[n];const auto &expected=row.at("writes")[n];check(actual.address==expected.at("address")&&hex(actual.before)==expected.at("before").get<std::string>()&&hex(actual.written)==expected.at("written").get<std::string>(),name+": shadow inverse store differs");}
  if(recording) {
   J expected=J::array();for(const auto &e:row.at("reads"))expected.push_back(J::array({e.at("address"),e.at("bytes")}));check(m.reads==expected,name+": shadow inverse reads differ");check(m.accesses==row.at("accesses"),name+": shadow inverse access order differs");reads+=m.reads.size();
  }
  ++count;prefixes+=stopped;writes+=m.stores.size();
 }
 check(count&&prefixes&&fresh_fp&&actual_faults==6,"Actual caller matrix incomplete");
 const auto &complete=f.at("cases")[0];Memory no_return;install(no_return,complete,false);auto a=abi(complete);const auto before=a;bool failed{};try{native_tu15_shadow_inverse_run(no_return,profile,a);}catch(const std::runtime_error&){failed=no_return.failure==std::make_pair(a.gp[4],std::size_t(4));}
 check(failed&&a.gp[4]==before.gp[4],"Missing real RET consumed or fabricated");for(const auto &b:complete.at("banks"))check(no_return.banks.at(b.at("address"))==raw(b.at("after")),"RET failure lost completed inverse data");check(a.floating_point->mxcsr==complete.at("after_fp")[2],"RET failure lost completed FP status");
 const auto fresh=std::ranges::find_if(f.at("cases"),[](const auto &row){return row.at("name")=="new_inverse_precision_leaf0";});check(fresh!=f.at("cases").end(),"Missing actual FP/return witness");
 for(const auto target:{0u,0xc4879u,0xc487fu,0x142d00u}) {
  Memory m;install(m,*fresh);auto bad=abi(*fresh);const auto initial=bad;auto &ret=m.banks.at(bad.gp[4]);for(unsigned n=0;n<4;++n)ret[n]=std::byte(target>>(8u*n));bool required{};
  try{native_tu15_shadow_inverse_run(m,profile,bad);}catch(const NativeTu15ShadowInverseReturnRequired &e){required=e.actual_return()==target;}
  check(required&&bad.gp[4]==initial.gp[4],"shadow inverse accepted owed return corruption");
  for(const auto &b:fresh->at("banks"))check(m.banks.at(b.at("address"))==raw(b.at("after")),"shadow inverse wrong RET lost completed data");
  check(bad.floating_point->mxcsr==fresh->at("after_fp")[2],"shadow inverse wrong RET lost completed FP");
 }
 for(unsigned mode=0;mode<13;++mode) {
  Memory m;auto p=profile;auto bad=abi(complete);switch(mode){case 0:p.image=SourcePhysicsContactImage::MapPack11;break;case 1:p.call.instruction=0x2ffe9u;break;case 2:p.call.target=0;break;case 3:p.call.tail=true;break;case 4:++bad.gp[1];break;case 5:bad.eflags|=0x400u;break;case 6:bad.floating_point.reset();break;case 7:bad.x87_depth=9;break;case 8:bad.active_mmx=SourcePhysicsMmxAlias{};break;case 9:bad.floating_point->mxcsr&=~0x80u;break;case 10:bad.gp[0]=bad.gp[4]-0x20u;bad.gp[1]=bad.gp[0]+0x34u;break;case 11:bad.gp[0]=0xffffffa0u;bad.gp[1]=0xffffffd4u;break;case 12:bad.gp[4]=2;break;}
  bool denied{};try{native_tu15_shadow_inverse_run(m,p,bad);}catch(const std::invalid_argument&){denied=true;}check(denied&&!m.failure&&m.stores.empty(),"Unqualified shadow profile touched memory");
 }
 check(native_tu15_shadow_inverse_owns_call(profile)&&!native_tu15_shadow_inverse_owns_call({SourcePhysicsContactImage::MapPack11,profile.call}),"Shadow inverse image ownership differs");
 std::cout<<f.at("cases").size()<<" genuine original shadow caller frames/prefixes; "<<reads<<" ordered reads, "<<writes<<" stores, "<<prefixes<<" stopped transports, "<<controls<<" physical continuation controls passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
