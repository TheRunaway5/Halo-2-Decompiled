#include "halo2/native_tu15_look_direction.hpp"
#include <algorithm>
#include <filesystem>
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
    std::map<std::uint32_t,std::vector<std::byte>>banks;bool log_reads{};mutable J reads=J::array();J fault;mutable unsigned fault_count{};
    auto at(std::uint32_t p,std::size_t n)const{auto i=banks.upper_bound(p);check(i!=banks.begin(),"Missing look bank");--i;check(std::uint64_t(p-i->first)+n<=i->second.size(),"Missing look extent");return i;}
    void read(std::uint32_t p,std::span<std::byte>b)const override{if(!fault.is_null()&&p==fault.at("address")&&b.size()==fault.at("bytes")&&++fault_count==fault.at("occurrence"))throw Fault{};if(log_reads)reads.push_back(J::array({p,b.size()}));auto i=at(p,b.size());std::copy_n(i->second.begin()+p-i->first,b.size(),b.begin());}
    void read_vector(std::uint32_t p,std::span<std::byte,16>b)const override{read(p,b);}
    void write(std::uint32_t p,std::span<const std::byte>b)override{auto i=at(p,b.size());std::copy(b.begin(),b.end(),banks.at(i->first).begin()+p-i->first);}
};
std::uint16_t control(){std::uint16_t value;asm volatile("fnstcw %0":"=m"(value));return value;}
std::uint16_t status(){std::uint16_t value;asm volatile("fnstsw %0":"=a"(value));return value;}
}
int main(int argc,char**argv){try{
 check(argc==2,"Required physical direction input absent");std::ifstream file(argv[1]);const auto fixture=J::parse(file);
 check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&fixture.at("backend")=="unchanged-tu15-native-look-direction-v1","Original look authority missing");
 std::size_t count{},reads{},writes{};
 for(const auto&c:fixture.at("cases")){
  const auto name=c.at("name").get<std::string>();Memory before;for(const auto&b:c.at("banks"))before.banks.emplace(b.at("address"),bytes(b.at("before")));if(c.contains("fault"))before.fault=c.at("fault");
  for(bool read_pass:{false,true}){
   if(read_pass&&c.contains("observer_fault"))continue;auto m=before;m.log_reads=read_pass;std::vector<SourcePhysicsContactRecordStore>stores;
   SourcePhysicsContactRecordObserver observer;if(!read_pass&&!c.contains("fault"))observer=[&](const auto&e){stores.push_back(e);if(c.contains("observer_fault")&&stores.size()==c.at("observer_fault"))throw ObserverFault{};};
   NativeTu15PhysicsFrameFp fp{c.at("before_fp")[0],c.at("before_fp")[1],c.at("before_fp")[2]};const auto cw=control(),sw=status();const auto mx=_mm_getcsr();bool fault{},observer_fault{};
   try{native_tu15_look_direction(m,c.at("direction"),c.at("input_index"),fp,0,observer);}catch(const Fault&){fault=true;}catch(const ObserverFault&){observer_fault=true;}
   check(fault==c.contains("fault")&&observer_fault==c.contains("observer_fault"),name+" look completion differs");
   check(control()==cw&&status()==sw&&_mm_getcsr()==mx,name+" leaked host FP");
   if(read_pass){J expected=J::array();for(const auto&r:c.at("reads"))expected.push_back(J::array({r.at("address"),r.at("bytes")}));check(m.reads==expected,name+" look ordered reads differ native="+m.reads.dump()+" original="+expected.dump());reads+=m.reads.size();}
   else if(!fault){const auto&expected=c.at("writes");check(stores.size()==expected.size(),name+" look store count differs");for(std::size_t n=0;n<stores.size();++n){const auto&a=stores[n];const auto&b=expected[n];check(a.instruction==b.at("instruction")&&a.address==b.at("address")&&hex(a.before)==b.at("before").get<std::string>()&&hex(a.written)==b.at("written").get<std::string>(),name+" look ordered stores differ");}writes+=stores.size();}
   for(const auto&b:c.at("banks"))check(hex(m.banks.at(b.at("address")))==b.at("after").get<std::string>(),name+" look retained bytes differ");
   const auto after=J::array({fp.control_word,fp.status,fp.mxcsr});check(after==c.at("after_fp"),name+" look complete FP differs native="+after.dump()+" original="+c.at("after_fp").dump());
  }++count;
 }
 std::cout<<count<<" unchanged look direction cases, "<<reads<<" reads, "<<writes<<" stores passed\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
