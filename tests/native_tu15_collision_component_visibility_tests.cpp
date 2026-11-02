#include "halo2/native_tu15_collision_component.hpp"
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
struct Fault{};
std::vector<std::byte>bytes(const std::string&s){std::vector<std::byte>b;for(std::size_t n=0;n<s.size();n+=2)b.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return b;}
struct Memory:SourcePhysicsContactMemory{
 std::map<std::uint32_t,std::vector<std::byte>>banks;mutable J reads=J::array();J fault;mutable unsigned occurrence{};
 void read(std::uint32_t p,std::span<std::byte>b)const override{if(!fault.is_null()&&p==fault.at("address")&&b.size()==fault.at("bytes")&&++occurrence==fault.at("occurrence"))throw Fault{};auto i=banks.upper_bound(p);check(i!=banks.begin(),"Missing visibility bank");--i;check(std::uint64_t(p-i->first)+b.size()<=i->second.size(),"Missing visibility extent");std::copy_n(i->second.begin()+p-i->first,b.size(),b.begin());reads.push_back(J::array({p,b.size()}));}
 void write(std::uint32_t,std::span<const std::byte>)override{throw std::runtime_error("Read-only visibility wrote data");}
};
}
int main(int argc,char**argv){try{check(argc==2,"Supply visibility physical fixture");std::ifstream input(argv[1]);const auto fixture=J::parse(input);check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Eligibility TU1.5 authority missing");std::size_t count{},reads{};
 for(const auto&c:fixture.at("cases")){const auto name=c.at("name").get<std::string>();Memory m;for(const auto&bank:c.at("banks"))m.banks.emplace(bank.at("address"),bytes(bank.at("before")));m.fault=c.value("fault",J{});bool failed{},result{};unsigned prior_mxcsr=_mm_getcsr();std::uint16_t prior_cw,prior_sw;asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(prior_cw),"=a"(prior_sw));
  try{result=c.at("entry")==0x186e70?native_tu15_collision_channel_enabled(m,c.at("channel"),c.at("instance")):native_tu15_collision_component_enabled(m,c.at("key"));}catch(const Fault&){failed=true;}
  std::uint16_t after_cw,after_sw;asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(after_cw),"=a"(after_sw));check(prior_mxcsr==_mm_getcsr()&&prior_cw==after_cw&&prior_sw==after_sw,name+" visibility modified host FP");check(failed==c.contains("fault"),name+" visibility fault prefix differs");if(!failed)check(result==bool(c.at("boundary_gp")[0].get<std::uint32_t>()&0xffu),name+" visibility result differs");J expected=J::array();for(const auto&read:c.at("reads"))expected.push_back(J::array({read.at("address"),read.at("bytes")}));check(m.reads==expected,name+" visibility ordered read differs native="+m.reads.dump()+" original="+expected.dump());for(const auto&bank:c.at("banks"))check(m.banks.at(bank.at("address"))==bytes(bank.at("after")),name+" visibility retained bytes changed");++count;reads+=m.reads.size();
 }std::cout<<count<<" complete/read-fault original visibility cases and "<<reads<<" ordered reads passed\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
