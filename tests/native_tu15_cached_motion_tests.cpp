#include "halo2/native_tu15_cached_motion.hpp"
#include "halo2/native_tu15_kinematic_motion.hpp"
#include <algorithm>
#include <cstring>
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
struct Prefix{};
struct Memory:SourcePhysicsContactMemory{
 std::map<std::uint32_t,std::vector<std::byte>>banks;std::string phase;unsigned ordinal{};mutable unsigned attempts{};unsigned writes{};bool logging{},raw_logging{};mutable J reads=J::array();std::vector<SourcePhysicsContactRecordStore>raw;
 auto at(std::uint32_t p,std::size_t n)const{auto i=banks.upper_bound(p);check(i!=banks.begin(),"Missing cached motion bank");--i;check(std::uint64_t(p-i->first)+n<=i->second.size(),"Missing cached motion extent");return i;}
 void read(std::uint32_t p,std::span<std::byte>b)const override{if(phase=="before_read"&&attempts++==ordinal)throw Prefix{};if(logging)reads.push_back(J::array({p,b.size()}));auto i=at(p,b.size());std::copy_n(i->second.begin()+p-i->first,b.size(),b.begin());}
 void read_vector(std::uint32_t p,std::span<std::byte,16>b)const override{read(p,b);}
 void write(std::uint32_t p,std::span<const std::byte>b)override{if(phase=="before_write"&&writes++==ordinal)throw Prefix{};auto i=at(p,b.size());if(raw_logging)raw.push_back({0,p,std::vector<std::byte>(i->second.begin()+p-i->first,i->second.begin()+p-i->first+b.size()),std::vector<std::byte>(b.begin(),b.end())});std::copy(b.begin(),b.end(),banks.at(i->first).begin()+p-i->first);}
};
struct Host {
 alignas(16)std::array<std::byte,108>outer{},expected{};unsigned mxcsr;
 Host():mxcsr(_mm_getcsr()){asm volatile("fnsave %0":"=m"(outer)::"memory");asm volatile("fld1\n\tfldz\n\tfldpi":::"memory","st","st(1)","st(2)");asm volatile("fnsave %0":"=m"(expected)::"memory");std::fill(expected.begin()+12,expected.begin()+24,std::byte{});asm volatile("frstor %0"::"m"(expected):"memory");}
 ~Host(){asm volatile("frstor %0"::"m"(outer):"memory");_mm_setcsr(mxcsr);}
 void verify(){alignas(16)std::array<std::byte,108>actual{};asm volatile("fnsave %0":"=m"(actual)::"memory");asm volatile("frstor %0"::"m"(actual):"memory");check(actual==expected&&_mm_getcsr()==mxcsr,"Cached motion leaked108-byte host FP state");}
};
}
int main(int argc,char**argv){try{check(argc==2,"Physical cached motion constraint fixture required");std::ifstream input(argv[1]);const auto f=J::parse(input);check(f.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&f.at("body_sites")==381,"Different native cached motion authority");std::size_t count{},reads{},writes{};
 for(const auto&test:f.at("cases")){const auto name=test.at("name").get<std::string>();Memory before;for(const auto&b:test.at("banks"))before.banks.emplace(b.at("address"),bytes(b.at("before")));const auto fault=test.value("fault",J::object());before.phase=fault.value("phase",std::string{});before.ordinal=fault.value("ordinal",0u);
  for(bool reading:{false,true}){if(reading&&before.phase=="after_write")continue;auto m=before;m.logging=reading;m.raw_logging=!reading&&(before.phase=="before_read"||before.phase=="before_write");std::vector<SourcePhysicsContactRecordStore>stores;J estimates=J::array();unsigned observed{};SourcePhysicsContactRecordObserver observer;if(!reading&&!m.raw_logging)observer=[&](const auto&s){stores.push_back(s);if(before.phase=="after_write"&&observed++==before.ordinal)throw Prefix{};};NativeTu15PhysicsFrameFp fp{test.at("before_fp")[0],test.at("before_fp")[1],test.at("before_fp")[2]};const auto&args=test.at("arguments");NativeTu15CachedMotionStats stats;Host host;bool stopped{},alignment{};try{if(test.at("entry")==0x8fdb0)native_tu15_apply_prescribed_motion_forces(fp);else native_tu15_advance_cached_motion(m,{test.at("gp")[1]},args[0],args[1],fp,test.at("live_x87").size(),observer,[&](std::uint32_t input,std::uint32_t estimate){estimates.push_back(J::array({input,estimate}));},&stats);}catch(const Prefix&){stopped=true;}catch(const std::runtime_error&e){alignment=std::string(e.what())=="Native TU1.5 cached motion requires aligned vector storage";if(!alignment)throw;}host.verify();J expected_estimates=J::array();for(const auto&e:test.value("rsqrt_estimates",J::array()))expected_estimates.push_back(J::array({e.at("input"),e.at("estimate")}));check(estimates==expected_estimates,name+" native RSQRTSS estimate differs");check(stopped==!before.phase.empty()&&alignment==test.value("alignment_fault",false),name+" cached motion constraint completion differs");
   if(reading){J expected=J::array();for(const auto&r:test.at("reads"))expected.push_back(J::array({r.at("address"),r.at("bytes")}));if(m.reads!=expected){std::size_t n{};while(n<m.reads.size()&&n<expected.size()&&m.reads[n]==expected[n])++n;throw std::runtime_error(name+" cached motion read differs at "+std::to_string(n)+" native "+(n<m.reads.size()?m.reads[n].dump():"end")+" original "+(n<expected.size()?expected[n].dump():"end"));}reads+=m.reads.size();}
   else{if(m.raw_logging)stores=m.raw;const auto&expected=test.at("writes");check(stores.size()==expected.size(),name+" cached motion store count differs "+std::to_string(stores.size())+"/"+std::to_string(expected.size()));for(std::size_t n=0;n<stores.size();++n){const auto&a=stores[n];const auto&b=expected[n];check((m.raw_logging||a.instruction==b.at("instruction"))&&a.address==b.at("address")&&hex(a.before)==b.at("before").get<std::string>()&&hex(a.written)==b.at("written").get<std::string>(),name+" cached motion ordered store differs at "+std::to_string(n)+" native="+hex(a.written)+" original="+b.at("written").get<std::string>());}writes+=stores.size();}
   for(const auto&b:test.at("banks")){const auto&actual=m.banks.at(b.at("address"));const auto expected=bytes(b.at("after"));if(actual!=expected){std::size_t n{};while(n<actual.size()&&actual[n]==expected[n])++n;throw std::runtime_error(name+" cached motion retained bytes differ at "+std::to_string(b.at("address").get<std::uint32_t>()+n));}}const auto&after=test.at("after_fp");check(fp.control_word==after[0]&&fp.status==after[1]&&fp.mxcsr==after[2],name+" cached motion complete FP differs native="+J::array({fp.control_word,fp.status,fp.mxcsr}).dump()+" original="+after.dump());
  }++count;
 }std::cout<<count<<" unchanged original cached motion calls/prefixes, "<<reads<<" ordered reads "<<writes<<" stores passed\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
