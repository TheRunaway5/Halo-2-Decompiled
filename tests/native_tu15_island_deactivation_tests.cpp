#include "halo2/native_tu15_island_deactivation.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <xmmintrin.h>
namespace {
using namespace halo2;
using J=nlohmann::json;
void check(bool b,const std::string&s){if(!b)throw std::runtime_error(s);}
std::vector<std::byte> bytes(const std::string&s){check(s.size()%2==0,"Invalid fixture bytes");std::vector<std::byte>b;for(std::size_t n=0;n<s.size();n+=2)b.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return b;}
std::string hex(std::span<const std::byte>b){const char*d="0123456789abcdef";std::string s;for(auto b:b){auto n=std::to_integer<unsigned>(b);s+=d[n>>4];s+=d[n&15];}return s;}
struct Fault{};
struct Memory:SourcePhysicsContactMemory{
  std::map<std::uint32_t,std::vector<std::byte>> banks;
  bool log_reads{},log_writes{};mutable J reads=J::array();J fault;mutable unsigned fault_count{};
  std::vector<SourcePhysicsContactRecordStore> raw_stores;
  std::function<void(std::uint32_t,std::size_t)> on_write;
  auto at(std::uint32_t p,std::size_t n)const{auto i=banks.upper_bound(p);check(i!=banks.begin(),"Missing motion bank");--i;check(std::uint64_t(p-i->first)+n<=i->second.size(),"Missing motion extent");return i;}
  void read(std::uint32_t p,std::span<std::byte>b)const override{if(!fault.is_null()&&p==fault.at("address")&&b.size()==fault.at("bytes")&&++fault_count==fault.at("occurrence"))throw Fault{};if(log_reads)reads.push_back(J::array({p,b.size()}));auto i=at(p,b.size());std::copy_n(i->second.begin()+p-i->first,b.size(),b.begin());}
  void write(std::uint32_t p,std::span<const std::byte>b)override{auto i=at(p,b.size());if(log_writes)raw_stores.push_back({0,p,std::vector<std::byte>(i->second.begin()+p-i->first,i->second.begin()+p-i->first+b.size()),std::vector<std::byte>(b.begin(),b.end())});std::copy(b.begin(),b.end(),banks.at(i->first).begin()+p-i->first);if(on_write)on_write(p,b.size());}
  void put(std::uint32_t p,std::uint32_t n,unsigned size=4){std::array<std::byte,4>b{};for(unsigned i=0;i<size;++i)b[i]=std::byte(n>>(i*8u));write(p,std::span(b).first(size));}
  std::uint32_t get(std::uint32_t p,unsigned size=4)const{std::array<std::byte,4>b{};read(p,std::span(b).first(size));std::uint32_t n{};for(unsigned i=0;i<size;++i)n|=std::to_integer<unsigned>(b[i])<<(i*8u);return n;}
};
std::uint16_t host_status(){std::uint16_t n;asm volatile("fnstsw %0":"=a"(n));return n;}
std::uint16_t host_control(){std::uint16_t n;asm volatile("fnstcw %0":"=m"(n));return n;}
constexpr std::uint32_t model_base=0x1000000,model_island=model_base+0x100,model_step=model_base+0x200,model_world=model_base+0x300,model_output=model_base+0x900;
Memory model(bool long_period=false,std::uint32_t count=3){
  Memory m;m.banks[model_base].resize(0x1000);m.banks[0x41e000].resize(0x2000);m.put(model_island+0x20,model_world);m.put(model_island+0x34,long_period?0:0x3fc00000);m.put(model_island+0x38,long_period?0x40200000:0);m.put(model_island+0x2c,4,1);m.put(model_step,0x3e000000);m.put(model_world+0x30,0x3f800000);m.put(model_world+0x34,0x40000000);m.put(model_island+0x40,count);m.put(model_island+0x3c,model_base+0x500);
  for(unsigned n=0;n<3;++n){m.put(model_base+0x500+n*4,model_base+0x600+n*0x80);m.put(model_base+0x600+n*0x80+0x54,model_base+0x780);}
  m.put(model_base+0x780,0x41eb6c);m.put(0x41eb80,0x31e5c0);m.put(0x41eb84,0x31e690);m.put(0x41f1c4,0x3233c0);m.put(0x41f1c8,0x3233c0);return m;
}
void models(){
  { // A false policy still visits subsequent bodies; callback changes list/count.
    auto m=model();NativeTu15IslandDeactivationMemory s(m,{model_island},{model_step},model_output);NativeTu15PhysicsFrameFp fp;std::vector<std::uint32_t>seen;
    const bool result=native_tu15_check_island_deactivation(s,fp,0,[&](auto policy,auto owner,auto body,auto period,auto&context){check(policy==model_base+0x780&&owner.identity==0x31e5c0&&period==NativeTu15DeactivationPeriod::short_period,"Short query ABI");seen.push_back(body.identity);
      if(seen.size()==1){m.put(model_island+0x40,2);m.put(model_island+0x3c,model_base+0x540);m.put(model_base+0x544,model_base+0x700);m.put(model_island+0x20,0);return false;}
      context.status=0x4500;context.mxcsr|=0x20;return true;
    });check(!result&&seen==std::vector<std::uint32_t>{model_base+0x600,model_base+0x700},"Policy failure/count/list mutation sequencing");check(m.get(model_island+0x2c,1)==4&&m.get(model_output,1)==0&&fp.status==0x4500&&fp.mxcsr==0x1fa0,"Policy actual FP/result prefix preserved");
  }
  { // Callback growth is observed and the signed counter is re-read afterward.
    auto m=model(false,1);m.put(model_island+0x2c,5,1);NativeTu15IslandDeactivationMemory s(m,{model_island},{model_step},model_output);NativeTu15PhysicsFrameFp fp;unsigned calls{};
    const bool result=native_tu15_check_island_deactivation(s,fp,0,[&](auto,auto,auto body,auto,auto&){++calls;if(calls==1){check(body.identity==model_base+0x600,"Initial body identity");m.put(model_island+0x40,2);m.put(model_island+0x2c,0x80,1);}else check(body.identity==model_base+0x680,"Expanded body identity");return true;});
    check(!result&&calls==2&&m.get(model_island+0x2c,1)==0x81,"Post-callback signed counter/growing cohort");
  }
  { // Null policy marks failure but does not hide a later long-period query.
    auto m=model(true,3);m.put(model_base+0x654,0);NativeTu15IslandDeactivationMemory s(m,{model_island},{model_step},model_output);NativeTu15PhysicsFrameFp fp;unsigned calls{};
    const bool result=native_tu15_check_island_deactivation(s,fp,0,[&](auto,auto owner,auto body,auto period,auto&){check(owner.identity==0x31e690&&body.identity==model_base+0x680&&period==NativeTu15DeactivationPeriod::long_period,"Long query ABI");++calls;m.put(model_island+0x40,0);return true;});
    check(!result&&calls==1&&m.get(model_island+0x38)==0x3f200000&&m.get(model_island+0x2c,1)==4,"Long null policy failure/wrap");
  }
  { // Typed write side effects verify count is fetched BEFORE wrapping store.
    auto m=model(false,1);unsigned elapsed_writes{},queries{};m.on_write=[&](auto p,auto){if(p==model_island+0x34&&++elapsed_writes==2)m.put(model_island+0x40,0);};
    NativeTu15IslandDeactivationMemory s(m,{model_island},{model_step},model_output);NativeTu15PhysicsFrameFp fp;
    native_tu15_check_island_deactivation(s,fp,0,[&](auto,auto,auto,auto,auto&){++queries;return true;});check(queries==1&&m.get(model_island+0x40)==0,"Initial body count must survive wrapping store");
  }
  { // Authenticated default3-site leaf is complete and never calls a residual.
    auto m=model(false,3);m.put(model_base+0x780,0x41f1b0);NativeTu15IslandDeactivationMemory s(m,{model_island},{model_step},model_output);NativeTu15PhysicsFrameFp fp;
    check(!native_tu15_check_island_deactivation(s,fp,0,[](auto,auto,auto,auto,auto&)->bool{throw std::runtime_error("Default policy used external fallback");})&&!native_tu15_default_deactivation_policy(),"Native default policy closure");
  }
  { // Output aliases counter: final result byte follows the reset store.
    auto m=model(false,0);m.put(model_island+0x2c,5,1);NativeTu15IslandDeactivationMemory s(m,{model_island},{model_step},model_island+0x2c);NativeTu15PhysicsFrameFp fp;
    check(native_tu15_check_island_deactivation(s,fp,0,{})&&m.get(model_island+0x2c,1)==1,"Aliased counter/output order");
  }
  { // Re-read aliased timestep after first elapsed publication.
    auto m=model(false,0);m.put(model_island+0x34,0x3f000000);NativeTu15IslandDeactivationMemory s(m,{model_island},{model_island+0x34},model_output);NativeTu15PhysicsFrameFp fp;
    check(!native_tu15_check_island_deactivation(s,fp,0,{})&&m.get(model_island+0x34)==0x3f800000&&m.get(model_island+0x38)==0x3f800000,"Aliased timestep must be read at both phases");
  }
  std::cout<<"Deactivation model callbacks, mutable cohorts/lists, cached count, signed counter, aliases, actual FP transfer and native default policy passed; custom body policies remain explicit\n";
}
void oracle(const std::filesystem::path&path){
  check(std::filesystem::exists(path),"Required TU1.5 deactivation fixture absent");std::ifstream in(path);const auto f=J::parse(in);
  check(f.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","TU1.5 deactivation authority changed");
  check(f.at("required_backend")=="unchanged-tu15-deactivation-physical-operands-stores-full-fp-v2","Required physical original deactivation backend missing");
  check(f.at("deactivation_sites")==119&&f.at("default_policy_sites")==3,"Authenticated deactivation/default-policy bodies changed");
  std::size_t count=0,store_count=0,reads_count=0,faults=0;
  for(const auto&c:f.at("cases")){
    const auto name=c.at("name").get<std::string>();check(c.at("physical_verified").get<bool>(),name+" missing physical original verification");
    Memory original;for(const auto&b:c.at("banks"))check(original.banks.emplace(b.at("address"),bytes(b.at("before"))).second,"Duplicate controller bank");original.fault=c.value("fault",J{});
    for(bool read_pass:{false,true}){
      auto m=original;m.log_reads=read_pass;const bool fault_case=!m.fault.is_null();m.log_writes=fault_case&&!read_pass;std::vector<SourcePhysicsContactRecordStore>stores;
      SourcePhysicsContactRecordObserver observe;if(!read_pass&&!fault_case)observe=[&](const auto&s){stores.push_back(s);};
      NativeTu15IslandDeactivationMemory storage(m,{c.at("island")},{c.at("step")},c.at("output"),observe);NativeTu15PhysicsFrameFp fp{c.at("before_fp")[0],c.at("before_fp")[1],c.at("before_fp")[2]};
      const auto cw=host_control(),sw=host_status();const auto mxcsr=_mm_getcsr();bool boundary=false,failed=false;
      try{native_tu15_check_island_deactivation(storage,fp,0,[&](auto policy,auto owner,auto body,auto period,auto&)->bool{
          const auto expected=c.at("boundary");check(expected!=nullptr&&policy==expected.at("policy")&&owner.identity==expected.at("owner")&&body.identity==expected.at("body")&&int(period)==expected.at("period"),name+" original deactivation policy boundary differs");boundary=true;throw std::runtime_error("original-policy-boundary");
      });}catch(const Fault&){failed=true;}catch(const std::runtime_error&e){if(std::string(e.what())!="original-policy-boundary")throw;}
      check(failed==fault_case&&boundary==!c.at("boundary").is_null(),name+" policy/read-fault prefix completion differs");check(host_control()==cw&&host_status()==sw&&_mm_getcsr()==mxcsr,name+" leaked native platform FP state");
      if(read_pass){J expected=J::array();for(const auto&r:c.at("reads"))expected.push_back(J::array({r.at("address"),r.at("bytes")}));if(m.reads!=expected){std::size_t n=0;while(n<m.reads.size()&&n<expected.size()&&m.reads[n]==expected[n])++n;throw std::runtime_error(name+" original controller read order differs at "+std::to_string(n)+" native "+(n<m.reads.size()?m.reads[n].dump():"end")+" original "+(n<expected.size()?expected[n].dump():"end"));}reads_count+=m.reads.size();}
      else{if(fault_case)stores=m.raw_stores;const auto&w=c.at("writes");check(stores.size()==w.size(),name+" ordered deactivation store count differs");for(std::size_t n=0;n<stores.size();++n){const auto&a=stores[n];const auto&b=w[n];check((fault_case||a.instruction==b.at("instruction"))&&a.address==b.at("address")&&hex(a.before)==b.at("before").get<std::string>()&&hex(a.written)==b.at("written").get<std::string>(),name+" ordered deactivation store differs at "+std::to_string(n));}store_count+=stores.size();}
      for(const auto&b:c.at("banks"))check(hex(m.banks.at(b.at("address")))==b.at("after").get<std::string>(),name+" retained deactivation bytes differ");const auto&end=c.at("after_fp");check(fp.control_word==end[0]&&fp.status==end[1]&&fp.mxcsr==end[2],name+" complete native FP differs "+J::array({fp.control_word,fp.status,fp.mxcsr}).dump()+"/"+end.dump());
    }
    ++count;faults+=!c.value("fault",J{}).is_null();
  }
  check(count>0,"Empty original deactivation fixture");std::cout<<count<<" original TU1.5 deactivation cases, "<<store_count<<" ordered stores, "<<reads_count<<" ordered reads, "<<faults<<" read-fault prefixes and complete physical FP passed; custom policy/live closure remains open\n";
}
}
int main(int argc,char**argv){try{if(argc==3&&std::string(argv[1])=="--oracle-only")oracle(argv[2]);else{models();if(argc==2)oracle(argv[1]);else check(argc==1,"Supply optional required original TU1.5 deactivation fixture");}return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
