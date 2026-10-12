#include "halo2/death_startup_runtime.hpp"
#include "halo2/source_runtime_image.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <set>

namespace {
using namespace halo2;
using Json=nlohmann::json;
std::vector<std::byte> bytes(const std::string &hex) {
  if(hex.size()%2)throw std::runtime_error("Odd fixture byte string");
  std::vector<std::byte> out;out.reserve(hex.size()/2);
  for(std::size_t i=0;i<hex.size();i+=2)
    out.push_back(std::byte(std::stoul(hex.substr(i,2),nullptr,16)));
  return out;
}
void check(bool value,const std::string &message) {
  if(!value)throw std::runtime_error(message);
}
}
int main(int argc,char **argv) {try {
  if(argc!=3 || !std::filesystem::is_regular_file(argv[1]) ||
      !std::filesystem::is_regular_file(argv[2]))return 77;
  std::ifstream input(argv[1]);const auto fixture=Json::parse(input);
  auto memory=halo2::load_source_runtime_image_1_1(argv[2]);
  const auto base=fixture.at("caller_abi").at("scratch_address_u32").get<std::uint32_t>();
  const auto size=fixture.at("caller_abi").at("scratch_bytes").get<unsigned>();
  memory.mount(base,std::vector<std::byte>(size),halo2::SourceRuntimeMemory::Access::ReadWrite,
               "declared original division caller stack");
  std::set<std::uint32_t> reached;
  halo2::SourceDeathWorldSchedulerServices services;
  services.runtime.instruction=[&](auto p){reached.insert(p);};
  std::size_t count=0,stores=0;
  for(const auto &sample:fixture.at("cases")) {
    const auto label=sample.at("id").get<std::string>();
    const auto initial=bytes(sample.at("input").at("scratch_80_hex"));
    check(initial.size()==size,label+": caller stack size differs");memory.write(base,initial);
    halo2::SourceUnitAnimationRuntimeAbi abi{};
    abi.gp=sample.at("input").at("gp_u32").get<std::array<std::uint32_t,8>>();
    abi.eflags=sample.at("input").at("eflags_u32");
    std::size_t written=0;
    const halo2::SourcePhysicsContactRecordObserver observe=[&](const auto &store) {
      const auto &expected=sample.at("stores").at(written++);
      check(store.instruction==expected.at("instruction") && store.address==expected.at("address") &&
          store.before==bytes(expected.at("before_hex")) && store.written==bytes(expected.at("written_hex")),
          label+": original ordered stack write differs");++stores;
    };
    halo2::source_run_death_startup_1_1(memory,0x327290,abi,{0x27f,0x1f80},services,observe);
    check(abi.gp==sample.at("output").at("gp_u32").get<std::array<std::uint32_t,8>>(),
          label+": original registers or return stack differ");
    const auto expected=bytes(sample.at("output").at("scratch_80_hex"));
    const auto actual=memory.view(base,size);
    check(std::equal(actual.begin(),actual.end(),expected.begin(),expected.end()) &&
          written==sample.at("stores").size(),label+": final caller stack or write count differs");
    const auto numerator=std::stoull(sample.at("input").at("numerator_hex").get<std::string>(),nullptr,16);
    const auto denominator=std::stoull(sample.at("input").at("denominator_hex").get<std::string>(),nullptr,16);
    check((std::uint64_t(abi.gp[2])<<32|abi.gp[0])==numerator/denominator,
          label+": independent unsigned quotient differs");
    // Undefined DIV/MUL/shift flags are deliberately not compared between CPUs.
    ++count;
  }
  check(count==fixture.at("case_count") && reached.size()==43 && stores==count*2,
        "Original division path or store coverage differs");
  for(const auto &address:fixture.at("executed_original_addresses"))
    check(reached.contains(std::stoul(address.get<std::string>(),nullptr,16)),"Original division instruction omitted");
  std::cout<<count<<" original unsigned division cases, all43 instructions and "<<stores
           <<" exact ordered writes agree with independent unsigned arithmetic\n";
  return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
