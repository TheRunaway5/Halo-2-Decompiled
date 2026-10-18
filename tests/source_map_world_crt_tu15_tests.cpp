#include "halo2/source_map_world_support_tu15.hpp"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <cstring>
namespace {
using namespace halo2;using J=nlohmann::json;
void check(bool yes,const char*why){if(!yes)throw std::runtime_error(why);}
std::vector<std::byte> bytes(const std::string&s){std::vector<std::byte>v;for(unsigned i=0;i<s.size();i+=2)v.push_back(std::byte(std::stoul(s.substr(i,2),nullptr,16)));return v;}
struct Memory final:SourcePhysicsContactMemory {
 std::map<unsigned,std::vector<std::byte>> banks;
 auto find(unsigned address)const{auto p=banks.upper_bound(address);check(p!=banks.begin(),"Missing original CRT memory owner");--p;check(address-p->first<p->second.size(),"Original CRT memory extent differs");return p;}
 void read(unsigned address,std::span<std::byte>out)const override{for(auto&b:out){auto p=find(address);b=p->second[address++-p->first];}}
 void write(unsigned address,std::span<const std::byte>in)override{for(auto b:in){auto p=find(address);check(p->first==0x56b170,"Original CRT wrote outside its mutable destination");banks.at(p->first)[address++-p->first]=b;}}
};
}
int main(int argc,char**argv)try{
 if(argc!=2||!std::filesystem::exists(argv[1]))return 77;std::ifstream input(argv[1]);J fixture;input>>fixture;
 check(fixture.at("original_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&fixture.at("complete_source_bytes")=="dd05a0ab4600d9ffd91d70b15600c3","Wrong original CRT reference");
 check(source_map_world_support_tu15_owns_entry(0x382230),"Missing complete382230 CRT owner");unsigned count=0;
 for(const auto&c:fixture.at("cases")){
  Memory m;for(const auto&r:c.at("regions"))m.banks.emplace(r.at("address").get<unsigned>(),bytes(r.at("before")));
  const auto&before=c.at("input");const auto&expected=c.at("original");SourcePhysicsWorldRuntimeAbi abi{};abi.gp=before.at("gp").get<decltype(abi.gp)>();abi.xmm=before.at("xmm").get<decltype(abi.xmm)>();abi.eflags=before.at("eflags");abi.fs_base=before.at("fs_base");abi.x87_status=before.at("x87_status");abi.x87_depth=before.at("x87_values").size();for(unsigned n=0;n<abi.x87_depth;++n){const auto raw=bytes(before["x87_values"][n]);check(raw.size()==10,"CRT live x87 encoding differs");std::memcpy(&abi.x87[n],raw.data(),10);}
  abi.floating_point=SourcePhysicsFactoryFloatingPoint{before.at("x87_control_word").get<std::uint16_t>(),before.at("mxcsr").get<unsigned>()};std::vector<unsigned>trace;unsigned stores=0;SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto pc){trace.push_back(pc);};services.required_owner=[](auto&,const auto&,auto&,const auto&){throw std::runtime_error("CRT callback has no external callees");};
  source_run_map_world_support_tu15(m,0x382230,abi,*abi.floating_point,services,[&](const auto&w){check(w.instruction==0x382238&&w.address==0x56b170&&w.written.size()==4,"CRT store instruction or extent differs");++stores;});
  check(abi.gp==expected.at("gp").get<decltype(abi.gp)>(),"Original CRT GP/RET stack differs");check(abi.xmm==expected.at("xmm").get<decltype(abi.xmm)>(),"Original CRT SSE state differs");const unsigned mask=c.at("eflags_mask");check((abi.eflags&mask)==(expected.at("eflags").get<unsigned>()&mask),"Original CRT integer flags differ");
  check(abi.x87_depth==expected.at("x87_values").size()&&abi.x87_status==expected.at("x87_status").get<std::uint16_t>(),"Original CRT x87 depth/status differs");for(unsigned n=0;n<abi.x87_depth;++n){const auto raw=bytes(expected["x87_values"][n]);check(!std::memcmp(&abi.x87[n],raw.data(),10),"Original CRT live x87 register changed");}
  check(abi.floating_point&&abi.floating_point->x87_control_word==expected.at("x87_control_word").get<std::uint16_t>()&&abi.floating_point->mxcsr==expected.at("mxcsr").get<unsigned>(),"Original CRT FP controls differ");check(trace==std::vector<unsigned>{0x382230,0x382236,0x382238,0x38223e}&&stores==1,"Original CRT complete instruction/store trace differs");for(const auto&r:c.at("regions"))check(m.banks.at(r.at("address").get<unsigned>())==bytes(r.at("after")),"Original CRT full operand/output/stack memory differs");++count;
 }
 check(count==24,"Incomplete original CRT control/ABI fixture roster");std::cout<<"PASS24 original382230 CRT callbacks: complete GP/SSE/x87/FP controls, memory, instructions and stores\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
