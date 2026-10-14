#include "halo2/source_sandbox_special_tu15.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
void check(bool ok,const std::string &why) {if(!ok)throw std::runtime_error(why);}
std::vector<std::byte> bytes(const std::string &hex) {
  check(hex.size()%2==0,"Malformed original byte bank");std::vector<std::byte> result;
  for(unsigned n=0;n<hex.size();n+=2)result.push_back(std::byte(std::stoul(hex.substr(n,2),nullptr,16)));
  return result;
}
}
int main(int argc,char **argv)try {
  check(argc==2,"Supply unchanged TU1.5 sandbox fixture");
  std::ifstream file(argv[1]);if(!file)return 77;
  const auto fixture=nlohmann::json::parse(file);
  check(fixture.at("original_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd",
      "Special movement fixture has the wrong original authority");
  unsigned count{};
  for(const auto &c:fixture.at("cases")) {
    const auto context="case "+std::to_string(count)+" "+c.at("controls").dump()+": ";
    SourceRuntimeMemory memory;
    for(const auto &bank:c.at("initial_banks"))memory.mount(bank.at("address"),bytes(bank.at("hex")),
        SourceRuntimeMemory::Access::ReadWrite,"explicit original movement ABI bank");
    SourcePhysicsWorldRuntimeAbi abi{};abi.gp=c.at("entry_gp").get<decltype(abi.gp)>();abi.eflags=0x202;
    std::vector<std::uint32_t> trace;SourcePhysicsWorldRuntimeServices services;
    services.instruction=[&](auto pc){trace.push_back(pc);};
    services.required_owner=[](auto &,const auto &,auto &,const auto &) {
      throw std::runtime_error("Special movement unexpectedly requested a supplied service");
    };
    source_run_sandbox_special_tu15(memory,c.at("entry"),abi,{0x27f,0x1f80},services);
    check(abi.gp==c.at("exit_gp").get<decltype(abi.gp)>(),context+"GP registers differ");
    check((abi.eflags&0xcd5u)==(c.at("eflags").get<unsigned>()&0xcd5u),context+"defined flags differ");
    check(trace==c.at("instructions").get<decltype(trace)>(),context+"original instruction path differs");
    for(unsigned n=0;n<8;++n) {
      const auto hex=c.at("exit_xmm")[n].get<std::string>();
      for(unsigned k=0;k<4;++k)check(abi.xmm[n][k]==std::stoul(hex.substr(24-k*8,8),nullptr,16),
          context+"SSE register differs");
    }
    for(const auto &bank:c.at("final_banks")) {
      const auto expected=bytes(bank.at("hex"));const auto actual=memory.view(bank.at("address"),expected.size());
      check(std::ranges::equal(actual,expected),context+"retained original memory differs at "+
          std::to_string(bank.at("address").get<unsigned>()));
    }
    ++count;
  }
  check(count>0,"Original special movement fixture is empty");
  std::cout<<"PASS unchanged TU1.5 sandbox owners: "<<count<<" instruction/GP/SSE/flag/memory cases\n";
  return 0;
}catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
