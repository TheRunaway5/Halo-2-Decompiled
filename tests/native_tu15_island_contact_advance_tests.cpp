#include "halo2/native_tu15_island_contact_advance.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <xmmintrin.h>
namespace {
using namespace halo2;using J=nlohmann::json;
void check(bool b,const std::string&s){if(!b)throw std::runtime_error(s);}
std::vector<std::byte>bytes(const std::string&s){std::vector<std::byte>b;for(std::size_t n=0;n<s.size();n+=2)b.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return b;}
std::string hex(std::span<const std::byte>b){const char*d="0123456789abcdef";std::string s;for(auto v:b){auto n=std::to_integer<unsigned>(v);s+=d[n>>4];s+=d[n&15];}return s;}
struct Fault{};struct ObserverFault{};
struct Memory:SourcePhysicsContactMemory{
 std::map<std::uint32_t,std::vector<std::byte>>banks;mutable J reads=J::array();bool logging{},raw_logging{};J fault;mutable unsigned hits{};std::vector<SourcePhysicsContactRecordStore>raw;
 auto at(std::uint32_t p,std::size_t n)const{auto i=banks.upper_bound(p);check(i!=banks.begin(),"Contact advance input absent");--i;check(std::uint64_t(p-i->first)+n<=i->second.size(),"Contact advance extent absent");return i;}
 void read(std::uint32_t p,std::span<std::byte>b)const override{if(!fault.is_null()&&fault.value("access",std::string{})!="write"&&p==fault.at("address")&&b.size()==fault.at("bytes")&&++hits==fault.at("occurrence"))throw Fault{};if(logging)reads.push_back(J::array({p,b.size()}));auto i=at(p,b.size());std::copy_n(i->second.begin()+p-i->first,b.size(),b.begin());}
 void read_vector(std::uint32_t p,std::span<std::byte,16>b)const override{read(p,b);}
 void write(std::uint32_t p,std::span<const std::byte>b)override{if(!fault.is_null()&&fault.value("access",std::string{})=="write"&&p==fault.at("address")&&b.size()==fault.at("bytes")&&++hits==fault.at("occurrence"))throw Fault{};auto i=at(p,b.size());if(raw_logging)raw.push_back({0,p,std::vector<std::byte>(i->second.begin()+p-i->first,i->second.begin()+p-i->first+b.size()),std::vector<std::byte>(b.begin(),b.end())});std::copy(b.begin(),b.end(),banks.at(i->first).begin()+p-i->first);}
};
std::uint16_t cw(){std::uint16_t v;asm volatile("fnstcw %0":"=m"(v));return v;}
std::uint16_t sw(){std::uint16_t v;asm volatile("fnstsw %0":"=a"(v));return v;}
}
int main(int argc,char**argv){try{check(argc==2,"Contact advance oracle input required");std::ifstream input(argv[1]);const auto fixture=J::parse(input);check(fixture.at("original_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&fixture.at("body_sites")==437,"Different contact advance authority");std::size_t count{},total_reads{},total_stores{};
 for(const auto&test:fixture.at("cases")){const auto name=test.at("name").get<std::string>();Memory before;for(const auto&b:test.at("banks"))before.banks.emplace(b.at("address"),bytes(b.at("before")));before.fault=test.value("fault",J{});
  for(bool reading:{false,true}){if(reading&&test.contains("observer_fault"))continue;auto memory=before;memory.logging=reading;memory.raw_logging=!reading&&!before.fault.is_null();std::vector<SourcePhysicsContactRecordStore>stores;SourcePhysicsContactRecordObserver observer;if(!reading&&before.fault.is_null())observer=[&](const auto&event){stores.push_back(event);if(test.contains("observer_fault")&&stores.size()==test.at("observer_fault"))throw ObserverFault{};};NativeTu15PhysicsFrameFp fp{test.at("before_fp")[0],test.at("before_fp")[1],test.at("before_fp")[2]};NativeTu15IslandContactAdvanceServices services;bool reached{},failed{},observed{};
   const auto boundary=[&](const char*kind,std::uint32_t owner,std::uint32_t receiver,const J&args){const auto&expected=test.at("boundary");check(!expected.is_null()&&expected.at("kind")==kind&&expected.at("owner")==owner&&expected.at("receiver")==receiver&&expected.at("arguments")==args,name+" actual contact advance dependency ABI differs");reached=true;throw std::runtime_error("genuine-advance-owner");};services.advance_motion=[&](auto receiver,auto owner,auto time,auto config,auto&){boundary("motion",owner.identity,receiver.identity,J::array({time,config.value}));};services.process_agent=[&](auto receiver,auto owner,auto first,auto second,auto config,auto packet,auto&){boundary("process",owner.identity,receiver.identity,J::array({first.identity,second.identity,config.identity,packet.identity}));};const auto host_cw=cw(),host_sw=sw();const auto mxcsr=_mm_getcsr();NativeTu15IslandContactAdvanceStats stats;
   try{native_tu15_advance_island_contact_time(memory,{test.at("island")},{test.at("configuration")},{test.at("agent_entry")},{test.at("output")},test.at("time_bits"),fp,0,services,observer,&stats);}catch(const Fault&){failed=true;}catch(const ObserverFault&){observed=true;}catch(const std::runtime_error&e){if(std::string(e.what())!="genuine-advance-owner")throw;}
   check(failed==!before.fault.is_null()&&observed==test.contains("observer_fault")&&reached==!test.at("boundary").is_null(),name+" contact advance completion differs");check(cw()==host_cw&&sw()==host_sw&&_mm_getcsr()==mxcsr,name+" contact advance leaked host FP");
   if(reading){J expected=J::array();for(const auto&r:test.at("reads"))expected.push_back(J::array({r.at("address"),r.at("bytes")}));if(memory.reads!=expected){std::size_t n{};while(n<memory.reads.size()&&n<expected.size()&&memory.reads[n]==expected[n])++n;throw std::runtime_error(name+" contact advance read order differs at "+std::to_string(n)+" native "+(n<memory.reads.size()?memory.reads[n].dump():"end")+" original "+(n<expected.size()?expected[n].dump():"end"));}total_reads+=memory.reads.size();}
   else{if(!before.fault.is_null())stores=memory.raw;const auto&expected=test.at("writes");check(stores.size()==expected.size(),name+" contact advance store count differs");for(std::size_t n=0;n<stores.size();++n){const auto&a=stores[n];const auto&b=expected[n];check((!before.fault.is_null()||a.instruction==b.at("instruction"))&&a.address==b.at("address")&&hex(a.before)==b.at("before").get<std::string>()&&hex(a.written)==b.at("written").get<std::string>(),name+" contact advance store differs");}total_stores+=stores.size();}
   for(const auto&b:test.at("banks"))check(hex(memory.banks.at(b.at("address")))==b.at("after").get<std::string>(),name+" contact advance retained bank differs");const auto&after=test.at("after_fp");check(fp.control_word==after[0]&&fp.status==after[1]&&fp.mxcsr==after[2],name+" contact advance FP differs native="+J::array({fp.control_word,fp.status,fp.mxcsr}).dump()+" original="+after.dump());
  }++count;
 }std::cout<<count<<" original contact advance comparisons, "<<total_reads<<" ordered reads and "<<total_stores<<" ordered stores passed; real owner returns and live integration remain separate\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
