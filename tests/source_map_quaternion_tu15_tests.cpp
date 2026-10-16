#include "halo2/source_map_world_support_tu15.hpp"
#include "halo2/physics_phantom_world_tu15.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <map>
using namespace halo2;using Json=nlohmann::json;
namespace {
void check(bool yes,const std::string &why){if(!yes)throw std::runtime_error(why);}
std::vector<std::byte> bytes(const std::string &hex){std::vector<std::byte> out;for(unsigned n=0;n<hex.size();n+=2)out.push_back(std::byte(std::stoul(hex.substr(n,2),nullptr,16)));return out;}
struct Memory:SourcePhysicsContactMemory {
 std::map<std::uint32_t,std::vector<std::byte>> banks;
 auto at(std::uint32_t address)const {auto p=banks.upper_bound(address);check(p!=banks.begin(),"Missing quaternion source bank");--p;check(address-p->first<p->second.size(),"Quaternion source bank extent");return p;}
 void read(std::uint32_t address,std::span<std::byte> out)const override{for(auto &b:out){auto p=at(address);b=p->second[address++-p->first];}}
 void write(std::uint32_t address,std::span<const std::byte> in)override{for(auto b:in){auto p=at(address);banks.at(p->first)[address++-p->first]=b;}}
};
}
int main(int argc,char **argv)try {
 if(argc!=2 || !std::filesystem::exists(argv[1]))return 77;
 std::ifstream file(argv[1]);Json fixture;file>>fixture;
 check(fixture.at("reference")=="English TU1.5 only" && fixture.at("original_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong original quaternion reference");
 unsigned cases{};
 for(unsigned owner=0;owner<2;++owner)for(const auto &c:fixture.at("cases")) {
  Memory m;for(const auto &region:c.at("regions"))m.banks.emplace(region.at("address").get<std::uint32_t>(),bytes(region.at("before")));
  const auto &input=c.at("input"),&expected=c.at("original");SourcePhysicsWorldRuntimeAbi a{};
  a.gp=input.at("gp").get<decltype(a.gp)>();a.xmm=input.at("xmm").get<decltype(a.xmm)>();a.eflags=input.at("eflags");a.fs_base=input.at("fs_base");a.x87_status=input.at("x87_status");
  check(input.at("x87_values").empty(),"Quaternion test requires its declared empty input stack");
  a.floating_point=SourcePhysicsFactoryFloatingPoint{input.at("x87_control_word").get<std::uint16_t>(),input.at("mxcsr").get<std::uint32_t>()};
  SourcePhysicsWorldRuntimeServices services;services.required_owner=[](auto &,const auto &,auto &,const auto &){throw std::runtime_error("Original quaternion callback has no external callees");};
  if(owner==0)source_run_map_world_support_tu15(m,c.at("entry"),a,*a.floating_point,services);
  else {auto fp=*a.floating_point;source_run_physics_phantom_world_tu15_runtime(m,c.at("entry"),a,fp,services);}
  const auto where="Original quaternion case "+std::to_string(cases)+": ";
  check(a.gp==expected.at("gp").get<decltype(a.gp)>(),where+"GP");check(a.xmm==expected.at("xmm").get<decltype(a.xmm)>(),where+"SSE");
  const auto mask=c.at("eflags_mask").get<std::uint32_t>();check((a.eflags&mask)==(expected.at("eflags").get<std::uint32_t>()&mask),where+"EFLAGS");
  check(a.x87_depth==expected.at("x87_values").size(),where+"x87 depth");
  check(a.x87_status==expected.at("x87_status").get<std::uint16_t>(),where+"x87 status native="+std::to_string(a.x87_status)+" original="+expected.at("x87_status").dump());
  check(a.floating_point && a.floating_point->x87_control_word==expected.at("x87_control_word").get<std::uint16_t>() && a.floating_point->mxcsr==expected.at("mxcsr").get<std::uint32_t>(),where+"FP controls/status");
  for(const auto &region:c.at("regions"))check(m.banks.at(region.at("address").get<std::uint32_t>())==bytes(region.at("after")),where+"complete matrix/input/stack memory");
  ++cases;
 }
 check(cases==16,"Incomplete original quaternion case roster");std::cout<<"PASS 16 original TU1.5 map/world quaternion callbacks, complete memory/GP/SSE/x87/FP comparison\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
