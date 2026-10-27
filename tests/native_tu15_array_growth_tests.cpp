#include "halo2/native_tu15_array_allocator.hpp"
#ifdef HALO2_NATIVE_ARRAY_RESERVE
#include "halo2/native_tu15_array_reserve.hpp"
#endif
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <set>
#include <stdexcept>
#include <xmmintrin.h>
namespace {
using namespace halo2;using J=nlohmann::json;
void check(bool value,const std::string&why){if(!value)throw std::runtime_error(why);}
std::vector<std::byte> unhex(const std::string&s){std::vector<std::byte>out;for(unsigned i=0;i<s.size();i+=2)out.push_back(std::byte(std::stoul(s.substr(i,2),nullptr,16)));return out;}
std::string hex(std::span<const std::byte>b){constexpr char digits[]="0123456789abcdef";std::string out;for(auto v:b){const auto n=std::to_integer<unsigned>(v);out+=digits[n>>4];out+=digits[n&15];}return out;}
std::vector<std::byte> file(const std::filesystem::path&p){std::ifstream input(p,std::ios::binary);check(bool(input),"Missing owned original bank");std::vector<char>raw((std::istreambuf_iterator<char>(input)),{});std::vector<std::byte>out(raw.size());std::transform(raw.begin(),raw.end(),out.begin(),[](auto c){return std::byte(std::uint8_t(c));});return out;}
struct Fault{};struct Required{};
struct Memory:SourcePhysicsContactMemory {
  std::map<std::uint32_t,std::vector<std::byte>>banks;mutable J reads=J::array();J fault;mutable unsigned fault_count{};bool log_reads{};
  auto at(std::uint32_t p,std::size_t n)const{auto i=banks.upper_bound(p);check(i!=banks.begin(),"Unbacked native array operand");--i;check(std::uint64_t(p-i->first)+n<=i->second.size(),"Unbacked native array extent");return i;}
  void read(std::uint32_t p,std::span<std::byte>out)const override{if(!fault.is_null()&&!fault.value("write",false)&&p==fault.at("address")&&out.size()==fault.at("bytes")&&++fault_count==fault.at("occurrence"))throw Fault{};if(log_reads)reads.push_back(J::array({p,out.size()}));const auto i=at(p,out.size());std::copy_n(i->second.begin()+p-i->first,out.size(),out.begin());}
  void write(std::uint32_t p,std::span<const std::byte>in)override{if(!fault.is_null()&&fault.value("write",false)&&p==fault.at("address")&&in.size()==fault.at("bytes")&&++fault_count==fault.at("occurrence"))throw Fault{};const auto i=at(p,in.size());std::copy(in.begin(),in.end(),banks.at(i->first).begin()+p-i->first);}
};
std::uint16_t status(){std::uint16_t value;asm volatile("fnstsw %0":"=a"(value));return value;}
std::uint16_t control(){std::uint16_t value;asm volatile("fnstcw %0":"=m"(value));return value;}
}
int main(int argc,char**argv)try{
  check(argc==2,"Supply owned physical original array fixture");const auto path=std::filesystem::path(argv[1]);std::ifstream input(path);const auto fixture=J::parse(input);
  check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&fixture.at("oracle")=="unchanged-tu15-physical-array-and-real-pool-v1","Actual TU1.5 array/pool original required");
  std::size_t cases{},stores{},read_count{},faults{},mutations{};
  for(const auto&c:fixture.at("cases")){
    const auto name=c.at("name").get<std::string>();
    for(bool read_pass:{false,true}){
      if(read_pass&&!c.value("mutations",J::array()).empty())continue;
      Memory memory;for(const auto&bank:c.at("banks"))check(memory.banks.emplace(bank.at("address"),file(path.parent_path()/bank.at("before_file").get<std::string>())).second,"Duplicate actual array bank");memory.fault=c.value("fault",J{});memory.log_reads=read_pass;
      J actual_stores=J::array(),calls=J::array();std::set<unsigned>applied;
      SourcePhysicsContactRecordObserver observe;
      if(!read_pass)observe=[&](const auto&event){actual_stores.push_back(J{{"instruction",event.instruction},{"address",event.address},{"before",hex(event.before)},{"written",hex(event.written)}});unsigned index{};for(const auto&mutation:c.value("mutations",J::array())){if(mutation.at("instruction")==event.instruction&&applied.insert(index).second){const auto bytes=unhex(mutation.at("written"));memory.write(mutation.at("address"),bytes);++mutations;}++index;}};
      NativeTu15ArrayGrowthMemory array(memory,{c.value("header",0u)},observe);NativeTu15ArrayAllocatorMemory router(memory);NativeTu15PhysicsFrameFp fp{c.at("before_fp")[0],c.at("before_fp")[1],c.at("before_fp")[2]};
      NativeTu15ArrayAllocatorServices lower;NativeTu15ArrayGrowthServices growth;
      const auto allocation=[&](const NativeTu15ArrayAllocationRequest&r,auto&){calls.push_back(J{{"owner",r.owner},{"receiver",r.receiver},{"bytes",r.bytes},{"memory_class",r.memory_class}});if(r.owner!=0x233250u)throw Required{};NativeTu15PoolMemory pool(memory,r.receiver,observe);return native_tu15_pool_allocate(pool,r.bytes);};
      const auto release=[&](const NativeTu15ArrayReleaseRequest&r,auto&){calls.push_back(J{{"owner",r.owner},{"receiver",r.receiver},{"data",r.data},{"bytes",r.bytes},{"memory_class",r.memory_class}});if(r.owner!=0x233420u)throw Required{};NativeTu15PoolMemory pool(memory,r.receiver,observe);if(!native_tu15_pool_release(pool,r.data,r.bytes))throw Required{};};
      lower.allocate_owner=allocation;lower.release_owner=release;
      growth.allocate=[&](const auto&r,auto&f){calls.push_back(J{{"owner",r.owner},{"receiver",r.receiver},{"bytes",r.bytes},{"memory_class",r.memory_class}});if(r.owner!=0x149da0u)throw Required{};return native_tu15_route_array_allocation(router,r.bytes,r.memory_class,f,lower);};
      growth.release=[&](const auto&r,auto&f){calls.push_back(J{{"owner",r.owner},{"receiver",r.receiver},{"data",r.data},{"bytes",r.bytes},{"memory_class",r.memory_class}});if(r.owner!=0x149e00u)throw Required{};native_tu15_route_array_release(router,r.data,r.bytes,r.memory_class,f,lower);};
      const auto host_sw=status(),host_cw=control();const auto host_mx=_mm_getcsr();bool failed=false,required=false;std::uint32_t result{};NativeTu15ArrayGrowthActivity activity;
      try{const auto entry=c.at("entry").get<std::uint32_t>();const auto args=c.at("arguments").get<std::vector<std::uint32_t>>();const auto receiver=c.value("receiver",0u);
        if(entry==0x2e3250u)native_tu15_grow_array(array,args[1],fp,growth,&activity);
#ifdef HALO2_NATIVE_ARRAY_RESERVE
        else if(entry==0x2e31e0u){
          if(c.value("retained_arguments",false)){
            const auto input=[&](unsigned offset){std::array<std::byte,4>b{};memory.read(0x200f000u+offset,b);std::uint32_t n{};for(unsigned i=0;i<4;++i)n|=std::to_integer<unsigned>(b[i])<<(i*8u);return n;};
            native_tu15_reserve_array(memory,NativeTu15ArrayReserveArguments{
              [&]{return input(12);},[&]{return input(8);},[&]{return NativeTu15ArrayHeader{input(4)};}},fp,growth,observe,&activity);
          }else native_tu15_reserve_array(memory,NativeTu15ArrayHeader{args[0]},args[1],args[2],fp,growth,observe,&activity);
        }
#endif
        else if(entry==0x2e2e30u)native_tu15_copy_array_bytes(array,args[0],args[1],args[2],&activity);
        else if(entry==0x149da0u)result=growth.allocate({receiver,entry,args[0],args[1]},fp);
        else if(entry==0x149e00u)growth.release({receiver,entry,args[0],args[1],args[2]},fp);
        else if(entry==0x233250u)result=allocation({receiver,entry,args[0],args[1]},fp);
        else if(entry==0x233420u)release({receiver,entry,args[0],args[1],args[2]},fp);
        else throw std::runtime_error("Unauthenticated native array test owner");
      }catch(const Fault&){failed=true;}catch(const Required&){required=true;}
      check(failed==c.value("declared_fault",false)&&required==c.value("required_owner",false),name+" source array completion differs");
      check(host_sw==status()&&host_cw==control()&&host_mx==_mm_getcsr(),name+" native integer array leaked host FP");
      check(J::array({fp.control_word,fp.status,fp.mxcsr})==c.at("after_fp"),name+" complete array FP differs");
      if(!failed&&!required&&(c.at("entry")==0x233250u||c.at("entry")==0x149da0u))check(result==c.at("return_word"),name+" actual allocation result differs");
      check(calls==c.at("allocator_calls"),name+" actual allocator request order differs");
      if(read_pass){J expected=J::array();for(const auto&r:c.at("reads"))expected.push_back(J::array({r.at("address"),r.at("bytes")}));if(memory.reads!=expected){std::size_t i{};while(i<memory.reads.size()&&i<expected.size()&&memory.reads[i]==expected[i])++i;throw std::runtime_error(name+" array read order differs at "+std::to_string(i)+" native="+(i<memory.reads.size()?memory.reads[i].dump():"end")+" original="+(i<expected.size()?expected[i].dump():"end"));}read_count+=memory.reads.size();}
      else{if(actual_stores!=c.at("writes")){std::size_t i{};while(i<actual_stores.size()&&i<c.at("writes").size()&&actual_stores[i]==c.at("writes")[i])++i;throw std::runtime_error(name+" array store differs at "+std::to_string(i)+" native="+(i<actual_stores.size()?actual_stores[i].dump():"end")+" original="+(i<c.at("writes").size()?c.at("writes")[i].dump():"end"));}stores+=actual_stores.size();check(applied.size()==c.value("mutations",J::array()).size(),name+" declared mutation phase not reached");}
      for(const auto&bank:c.at("banks"))check(memory.banks.at(bank.at("address"))==file(path.parent_path()/bank.at("after_file").get<std::string>()),name+" complete array/pool bank differs");
    }
    ++cases;faults+=c.value("declared_fault",false);
  }
  check(cases>20,"Insufficient physical array coverage");std::cout<<J{{"passed",true},{"cases",cases},{"ordered_stores",stores},{"ordered_reads",read_count},{"fault_prefixes",faults},{"observer_mutations",mutations},{"allocator_outcomes_supplied",false}}.dump()<<'\n';return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
