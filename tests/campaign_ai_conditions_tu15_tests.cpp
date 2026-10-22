#include "halo2/source_campaign_ai_conditions_tu15.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace {
using namespace halo2;
void check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
std::vector<std::byte> unhex(const std::string &text) {
  check(text.size()%2==0,"Odd condition fixture hex");std::vector<std::byte> bytes;
  for(unsigned i=0;i<text.size();i+=2)bytes.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));
  return bytes;
}
std::string hex(std::span<const std::byte> bytes) {
  constexpr char digits[]="0123456789abcdef";std::string text;
  for(auto byte:bytes){const auto value=std::to_integer<unsigned>(byte);text+=digits[value>>4];text+=digits[value&15];}
  return text;
}
void run_case(const nlohmann::json &fixture,const nlohmann::json &input) {
  SourceRuntimeMemory memory;
  for(const auto &bank:fixture.at("banks"))memory.mount(bank.at(0),
      std::vector<std::byte>(bank.at(1).get<unsigned>()),SourceRuntimeMemory::Access::ReadWrite,
      "Declared original AI condition component input");
  for(const auto &table:fixture.at("tables"))memory.mount(table.at(0),unhex(table.at(1)),
      SourceRuntimeMemory::Access::ReadOnly,"Authenticated original AI condition dispatch data");
  for(const auto &patch:fixture.at("initial_patches"))memory.write(patch.at(0),unhex(patch.at(1)));
  for(const auto &patch:input.at("patches"))memory.write(patch.at(0),unhex(patch.at(1)));
  SourcePhysicsWorldRuntimeAbi abi{};
  abi.gp=input.at("gp").get<decltype(abi.gp)>();abi.xmm=input.at("xmm").get<decltype(abi.xmm)>();
  abi.eflags=0x202;abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
  std::vector<unsigned> trace;nlohmann::json stores=nlohmann::json::array();
  unsigned unavailable{};
  SourcePhysicsWorldRuntimeServices services{};
  services.instruction=[&](unsigned pc){trace.push_back(pc);};
  services.required_owner=[&](auto&,const auto &call,auto&,const auto&){
    unavailable=call.target;throw std::runtime_error("Required original AI condition service unavailable");};
  try{source_run_campaign_ai_conditions_tu15(memory,input.at("entry"),abi,*abi.floating_point,services,
      [&](const auto &store){stores.push_back(nlohmann::json::array({store.instruction,
          store.address,hex(store.before),hex(store.written)}));});}
  catch(const std::exception &error){if(!input.contains("stop_target"))throw std::runtime_error(
      "condition case "+input.at("name").get<std::string>()+": "+error.what());}
  check(unavailable==input.value("stop_target",0u),"AI condition supplied a missing dependency or delegated an owned path");
  check(abi.gp==input.at("final_gp").get<decltype(abi.gp)>(),"AI condition complete GPR ABI differs");
  check(abi.xmm==input.at("final_xmm").get<decltype(abi.xmm)>(),"AI condition complete SIMD ABI differs");
  check((abi.eflags&~0x10u)==(input.at("final_flags").get<unsigned>()&~0x10u),"AI condition defined flags differ");
  check(trace==input.at("trace").get<std::vector<unsigned>>(),"AI condition original instruction path differs");
  check(stores==input.at("stores"),"AI condition ordered writes/caller stack differ");
  check(hex(memory.view(0x2026000,16))==input.at("output").get<std::string>(),"AI condition observable output differs");
}
}
int main(int argc,char **argv)try {
  check(argc==2,"Expected independently executed original condition fixture");
  nlohmann::json fixture;std::ifstream(argv[1])>>fixture;
  check(fixture.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd",
      "Wrong AI condition authority");
  check(fixture.at("instruction_patches").empty()&&fixture.at("supplied_dependency_returns").empty(),
      "Original condition fixture supplies a gameplay outcome");
  for(const auto &input:fixture.at("cases"))run_case(fixture,input);
  SourceRuntimeMemory missing;SourcePhysicsWorldRuntimeAbi abi{};abi.gp[4]=0x203f000;
  bool rejected=false;
  try{source_run_campaign_ai_conditions_tu15(missing,0x29afd0,abi,{0x27f,0x1f80},{});}
  catch(const std::exception&){rejected=true;}
  check(rejected,"AI condition supplied absent source banks");
  std::cout<<"PASS "<<fixture.at("cases").size()<<" original AI condition/predicate paths, existing-owner reuse, GPR/SIMD, defined flags and ordered writes\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
