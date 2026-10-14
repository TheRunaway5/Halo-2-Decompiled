#include "halo2/physics_live_support_tu15.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
std::vector<std::byte> bytes(const std::string &hex){
  std::vector<std::byte> out;
  for(std::size_t i=0;i<hex.size();i+=2)out.push_back(std::byte(std::stoul(hex.substr(i,2),nullptr,16)));
  return out;
}
}
int main(int argc,char **argv)try {
  check(argc==2,"Supply unchanged original team-admission fixture");
  std::ifstream input(argv[1]);if(!input)return 77;
  const auto fixture=nlohmann::json::parse(input);
  check(source_physics_live_support_tu15_owns_entry(fixture.at("entry")),"Missing actual engine team-admission owner");
  unsigned count{};
  for(const auto &row:fixture.at("cases")) {
    SourceRuntimeMemory memory;
    if(row.contains("initial_banks")) {
      for(const auto &bank:row.at("initial_banks"))
        memory.mount(bank.at("address"),bytes(bank.at("hex")),SourceRuntimeMemory::Access::ReadWrite,"declared pre-spawn bank");
    } else memory.mount(row.at("bank_address"),bytes(row.at("initial_hex")),SourceRuntimeMemory::Access::ReadWrite,"declared callback stack");
    SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("entry_gp").get<decltype(abi.gp)>();abi.eflags=row.at("entry_flags");
    std::vector<std::uint32_t> trace;SourcePhysicsWorldRuntimeServices services;
    services.instruction=[&](auto address){trace.push_back(address);};
    services.required_owner=[](auto&,auto&,auto&,auto&){throw std::runtime_error("Unexpected team-admission child owner");};
    source_run_physics_live_support_tu15(memory,fixture.at("entry"),abi,{0x27f,0x1f80},services);
    check(abi.gp==row.at("exit_gp").get<decltype(abi.gp)>(),"Original callback GP/AL/stack cleanup differs");
    check((abi.eflags&0xcd5u)==(row.at("exit_flags").get<unsigned>()&0xcd5u),"Original callback flags differ");
    check(trace==row.at("instructions").get<decltype(trace)>(),"Original callback instruction path differs");
    if(row.contains("final_banks")) {
      for(const auto &bank:row.at("final_banks")) {
        const auto final=bytes(bank.at("hex"));
        check(std::ranges::equal(memory.view(bank.at("address"),final.size()),final),"Original complete team-reset bank differs");
      }
    } else {
      const auto final=bytes(row.at("final_hex"));
      check(std::ranges::equal(memory.view(row.at("bank_address"),final.size()),final),"Original callback stack bytes differ");
    }
    ++count;
  }
  check(count==fixture.value("case_count",256u),"Original team-owner corpus incomplete");
  std::cout<<"PASS "<<count<<" original team-owner GP/AL/flags/stack/full-memory controls\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
