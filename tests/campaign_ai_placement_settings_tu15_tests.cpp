#include "halo2/source_campaign_ai_placement_settings_tu15.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace {
using namespace halo2;
void check(bool value,const char *message) {if(!value)throw std::runtime_error(message);}
std::vector<std::byte> unhex(const std::string &text) {
  check(text.size()%2==0,"Odd fixture hex");std::vector<std::byte> bytes;
  for(unsigned i=0;i<text.size();i+=2)bytes.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));
  return bytes;
}
std::string hex(std::span<const std::byte> bytes) {
  constexpr char digits[]="0123456789abcdef";std::string text;
  for(const auto byte:bytes) {const auto n=std::to_integer<unsigned>(byte);text+=digits[n>>4];text+=digits[n&15];}
  return text;
}
void mount(SourceRuntimeMemory &memory,unsigned at,unsigned bytes) {
  memory.mount(at,std::vector<std::byte>(bytes),SourceRuntimeMemory::Access::ReadWrite,
      "Original campaign placement-settings component inputs");
}
void run_case(const nlohmann::json &input,const nlohmann::json &table,int missing=-1) {
  SourceRuntimeMemory memory;
  mount(memory,0x2000000,0x10000);mount(memory,0x4f0414,4);
  mount(memory,0x4f3218,4);mount(memory,0x5020b7,1);
  memory.mount(table.at(0),unhex(table.at(1)),SourceRuntimeMemory::Access::ReadOnly,
      "Authenticated original placement-settings switch");
  for(const auto &patch:input.at("patches"))memory.write(patch.at(0),unhex(patch.at(1)));
  if(missing>=0)memory.release(std::array{0x4f0414u,0x4f3218u,0x5020b7u,0x20ba30u}.at(missing));
  SourcePhysicsWorldRuntimeAbi abi{};
  abi.gp=input.at("gp").get<decltype(abi.gp)>();
  abi.xmm=input.at("xmm").get<decltype(abi.xmm)>();
  abi.eflags=0x202;abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
  std::vector<unsigned> trace;nlohmann::json stores=nlohmann::json::array();
  SourcePhysicsWorldRuntimeServices services{};
  services.instruction=[&](unsigned pc){trace.push_back(pc);};
  services.required_owner=[](auto&,const auto&,auto&,const auto&){
    throw std::runtime_error("Complete settings/lookup delegated an unowned dependency");};
  bool rejected=false;
  try{source_run_campaign_ai_placement_settings_tu15(memory,0x20b980,abi,*abi.floating_point,services,
      [&](const auto &store){stores.push_back(nlohmann::json::array({store.instruction,
          store.address,hex(store.before),hex(store.written)}));});}
  catch(const std::exception&){if(missing<0)throw;rejected=true;}
  if(missing>=0) {
    check(rejected,"Missing original placement-settings owner was supplied");
    check(!trace.empty()&&trace.back()==std::array{0x1ecc87u,0x20b98eu,0x20ba19u,0x20b9b3u}.at(missing),
        "Missing settings owner failed outside its actual original read");
    return;
  }
  check(abi.gp==input.at("final_gp").get<decltype(abi.gp)>(),"Placement-settings complete GPR ABI differs");
  check(abi.xmm==input.at("final_xmm").get<decltype(abi.xmm)>(),"Placement-settings complete SIMD ABI differs");
  // AF is architecturally undefined after TEST/AND/SHL. The other flags follow
  // the original instructions, including partial-register and COMISS paths.
  check((abi.eflags&~0x10u)==(input.at("final_flags").get<unsigned>()&~0x10u),"Placement-settings defined flags differ");
  check(trace==input.at("trace").get<std::vector<unsigned>>(),"Placement-settings original instruction path differs");
  check(stores==input.at("stores"),"Placement-settings ordered writes or original call-stack footprint differ");
  check(hex(memory.view(0x2003000,16))==input.at("output").get<std::string>(),"Placement-settings output bytes differ");
}
}
int main(int argc,char **argv)try {
  check(argc==2,"Expected independently executed original fixture");
  nlohmann::json fixture;std::ifstream(argv[1])>>fixture;
  check(fixture.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd",
      "Wrong placement-settings authority");
  check(fixture.at("instruction_patches").empty()&&fixture.at("supplied_dependency_returns").empty(),
      "Placement-settings fixture supplies an original outcome");
  check(source_campaign_ai_placement_settings_tu15_owns_entry(0x20b980)&&source_campaign_ai_placement_settings_tu15_owns_entry(0x1ecc80),
      "Campaign dispatcher lost the reached placement-settings owners");
  for(const auto &input:fixture.at("cases"))run_case(input,fixture.at("switch_table"));
  const auto &cases=fixture.at("cases");
  const auto control=std::ranges::find_if(cases,[](const auto &c){return c.at("mode")==1&&
      c.at("difficulty")==0&&c.at("selector")==1&&c.at("lookup")==2&&c.at("debug")==0;});
  check(control!=cases.end(),"Missing reached-selector control input");
  for(int missing=0;missing<4;++missing)run_case(*control,fixture.at("switch_table"),missing);
  SourceRuntimeMemory missing;SourcePhysicsWorldRuntimeAbi abi{};
  abi.gp[4]=0x200f000;bool rejected=false;
  try{source_run_campaign_ai_placement_settings_tu15(missing,0x20b980,abi,{0x27f,0x1f80},{});}
  catch(const std::exception&){rejected=true;}
  check(rejected,"Missing original settings storage was supplied");
  std::cout<<"PASS "<<fixture.at("cases").size()<<" complete original placement-settings paths, GPR/SIMD ABI, defined flags, ordered writes and 5 missing-owner controls\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
