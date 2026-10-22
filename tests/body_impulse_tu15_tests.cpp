#include "halo2/source_body_impulse_tu15.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/cache.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
void check(bool yes,const char *message){if(!yes)throw FormatError(message);}
std::vector<std::byte> bytes(const std::string &hex) {
  std::vector<std::byte> result;
  for(std::size_t i=0;i<hex.size();i+=2)result.push_back(std::byte(std::stoul(hex.substr(i,2),nullptr,16)));
  return result;
}
}
int main(int argc,char **argv)try {
  check(argc==2,"Supply original TU1.5 impulse fixture");
  std::ifstream input(argv[1]);const auto fixture=nlohmann::json::parse(input);
  check(fixture.at("reference_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd",
      "Wrong original impulse reference");
  unsigned cases{},instructions{};
  for(const auto &row:fixture.at("cases")) {
    SourceRuntimeMemory memory;std::vector<std::byte> storage(0x4000);
    memory.mount(0x1000000u,storage,SourceRuntimeMemory::Access::ReadWrite,"owned impulse numerical fixture");
    memory.write(0x1001000u,bytes(row.at("body_before")));
    memory.write(0x1001200u,bytes(row.at("impulse")));
    memory.write(0x1001220u,bytes(row.at("point")));
    const std::array<std::uint32_t,3> stack{0x1100000u,0x1001200u,0x1001220u};
    memory.write(0x1003000u,std::as_bytes(std::span(stack)));
    SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("gp").get<decltype(abi.gp)>();abi.eflags=0x202u;
    SourcePhysicsWorldRuntimeServices services;std::vector<std::uint32_t> trace;
    services.instruction=[&](auto pc){trace.push_back(pc);};
    source_run_body_impulse_tu15(memory,row.at("entry"),abi,{0x27f,0x1f80},services);
    check(abi.gp==row.at("returned_gp").get<decltype(abi.gp)>(),"Original impulse register/stack ABI differs");
    const auto expected=bytes(row.at("body_after"));
    const auto actual=memory.view(0x1001000u,expected.size());
    check(std::equal(expected.begin(),expected.end(),actual.begin()),"Original impulse body bytes differ");
    check(trace==row.at("instructions").get<decltype(trace)>(),"Original impulse instruction path differs");
    ++cases;instructions+=trace.size();
  }
  check(cases==32,"Incomplete original impulse cohort");
  std::cout<<"PASS "<<cases<<" original impulse cases, "<<instructions<<" instructions, "<<cases*272<<" exact body bytes\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
