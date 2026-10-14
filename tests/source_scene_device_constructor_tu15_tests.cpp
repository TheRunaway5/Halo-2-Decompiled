#include "halo2/source_scene_device_constructor_tu15.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <algorithm>
using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool value,const char* why){if(!value)throw std::runtime_error(why);}
std::vector<std::byte> bytes(const std::string& hex){
  check(hex.size()%2==0,"Incomplete original byte bank");std::vector<std::byte> out;
  for(std::size_t i=0;i<hex.size();i+=2)out.push_back(std::byte(std::stoul(hex.substr(i,2),nullptr,16)));
  return out;
}
}
int main(int argc,char** argv)try{
  check(argc==2,"Supply complete original TU1.5 constructor fixture");std::ifstream file(argv[1]);if(!file)return 77;
  Json fixture;file>>fixture;check(fixture.at("xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong visual reference image");
  unsigned count=0,stores=0,instructions=0;
  for(const auto& row:fixture.at("cases")){
    SourceRuntimeMemory memory;
    for(const auto& region:row.at("regions")){const auto data=bytes(region.at("bytes"));memory.mount(region.at("address"),data,SourceRuntimeMemory::Access::ReadWrite,"Declared original constructor input");}
    SourcePhysicsWorldRuntimeAbi abi{};const auto& input=row.at("input");const auto& expected=row.at("expected");
    abi.gp=input.at("gp").get<decltype(abi.gp)>();abi.xmm=input.at("xmm").get<decltype(abi.xmm)>();abi.eflags=input.at("eflags");
    abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};std::vector<std::uint32_t> trace;std::vector<SourcePhysicsContactRecordStore> writes;
    SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto pc){trace.push_back(pc);};
    services.required_owner=[](auto&,const auto&,auto&,const auto&){throw std::runtime_error("Original constructor has no external owner");};
    source_run_scene_device_constructor_tu15(memory,row.at("entry"),abi,*abi.floating_point,services,[&](const auto& s){writes.push_back(s);});
    check(abi.gp==expected.at("gp").get<decltype(abi.gp)>() && abi.xmm==expected.at("xmm").get<decltype(abi.xmm)>() && abi.eflags==expected.at("eflags"),"Complete constructor ABI differs from original");
    check(trace==expected.at("trace").get<decltype(trace)>() && !trace.empty(),"Original instruction trace differs");
    const auto body=bytes(expected.at("body"));check(std::ranges::equal(memory.view(0x62000000,body.size()),body),"Constructed or untouched original body bytes differ");
    check(writes.size()==expected.at("stores").size(),"Original store count differs");
    for(unsigned i=0;i<writes.size();++i){const auto& e=expected.at("stores").at(i);const auto& w=writes[i];
      check(w.instruction==e.at("instruction") && w.address==e.at("address") && w.before==bytes(e.at("before")) && w.written==bytes(e.at("written")),"Original constructor ordered store differs");}
    ++count;stores+=unsigned(writes.size());instructions+=unsigned(trace.size());
  }
  check(count==344,"Incomplete original constructor/group fixture");
  std::cout<<"PASS "<<count<<" original TU1.5 DEVI/MACH callbacks / "<<instructions<<" instructions / "<<stores<<" ordered stores, full GP/XMM/flags and untouched body bytes\n";
  return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
