#include "halo2/death_object_phases.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

namespace {
using Json=nlohmann::json;
void check(bool ok,const char *why) { if(!ok)throw std::runtime_error(why); }
std::uint32_t hex(const Json &j) { return std::stoul(j.get<std::string>(),nullptr,16); }
void put(halo2::SourceRuntimeMemory &memory,std::uint32_t address,std::uint32_t value) {
  std::array<std::byte,4> b;
  for(unsigned i=0;i<4;++i)b[i]=std::byte(value>>(8*i));
  memory.write(address,b);
}
std::uint32_t word(const halo2::SourceRuntimeMemory &memory,std::uint32_t address) {
  const auto b=memory.view(address,4);std::uint32_t value=0;
  for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(b[i])<<(8*i);
  return value;
}
struct Pending {};
}
int main(int argc,char **argv) {try {
  check(argc==2,"Supply the unchanged-original session gate capture");
  if(!std::filesystem::is_regular_file(argv[1]))return 77;
  std::ifstream input(argv[1]);const auto fixture=Json::parse(input);
  const auto &images=fixture.at("images");
  check(images.size()==2 && images[0].at("cases")==images[1].at("cases"),
      "Both original executable gate captures must agree");
  check(fixture.at("instruction_patches").empty() && fixture.at("supplied_lower_results").empty(),
      "Original gate capture must not patch instructions or supply lower outcomes");
  const auto &declared=fixture.at("declared_input");
  const auto game=hex(declared.at("game")),stack=hex(declared.at("stack")),stop=hex(declared.at("stop"));
  const auto entry=hex(images[0].at("entry")),slot=hex(images[0].at("session_slot"));
  check(entry==0xa8620 && halo2::source_death_object_phases_owns_entry_1_1(entry),
      "The actual shared dispatcher must own the complete original routine");
  const auto &cases=images[0].at("cases");check(cases.size()==256,"Complete original byte sweep");
  unsigned returns=0,pending=0;
  for(unsigned value=0;value<256;++value) {
    const auto &expected=cases.at(value);
    check(expected.at("input_byte")==value,"Original gate sweep order");
    halo2::SourceRuntimeMemory memory;
    memory.mount(slot,std::vector<std::byte>(4),halo2::SourceRuntimeMemory::Access::ReadWrite,"declared game pointer");
    memory.mount(game,std::vector<std::byte>(0x200),halo2::SourceRuntimeMemory::Access::ReadWrite,"declared session packet");
    memory.mount(stack-0x100,std::vector<std::byte>(0x120,std::byte{0xcd}),
        halo2::SourceRuntimeMemory::Access::ReadWrite,"declared original stack scratch");
    put(memory,slot,game);put(memory,stack,stop);put(memory,stack+4,hex(declared.at("object")));
    memory.mutable_view(game+0xc,1)[0]=std::byte(value);
    halo2::SourceUnitAnimationRuntimeAbi abi{};abi.eflags=2;
    abi.gp=declared.at("initial_gp").get<decltype(abi.gp)>();
    halo2::SourceUnitAnimationRuntimeAbi actual{};
    halo2::SourceDeathWorldSchedulerServices services{};
    unsigned instructions=0,lower_calls=0;
    services.runtime.instruction=[&](std::uint32_t){++instructions;};
    services.runtime.required_owner=[&](auto &,const auto &call,auto &state,const auto &) {
      ++lower_calls;
      check(value==5 && call.instruction==0xa8670 && call.target==0xa8720 && !call.tail,
          "Original active path must stop at its genuine first lower");
      actual=state;
      std::vector<std::uint32_t> arguments;
      for(unsigned i=0;i<6;++i)arguments.push_back(word(memory,state.gp[4]+4+i*4));
      check(expected.at("pending_arguments")==arguments,"Original pending six-argument ABI");
      check(word(memory,arguments[3])==expected.at("actual_count_store").get<std::uint32_t>(),
          "Original local count initialization");
      throw Pending{}; // No lower executes or supplies a return value.
    };
    bool stopped=false;
    try {
      // This owner executes integer instructions only; use explicit native
      // FP controls instead of zero-initializing the platform control words.
      halo2::source_run_death_object_phases_1_1(memory,entry,abi,{0x37f,0x1f80},services);
      actual=abi;++returns;
    } catch(const Pending &) {stopped=true;++pending;}
    check(stopped==(expected.at("stopped_at")=="first_lower_A8720"),"Original gate exit kind");
    check(lower_calls==unsigned(stopped),"No invented lower callbacks on inactive paths");
    check(actual.gp==expected.at("exit_gp").get<decltype(actual.gp)>(),"Complete original GP state");
    check(actual.eflags==expected.at("exit_eflags").get<std::uint32_t>(),"Complete original flags");
    check(instructions==expected.at("instruction_count").get<unsigned>(),"Original executed instruction count");
  }
  check(returns==255 && pending==1,"Original inactive returns and unsatisfied active branch");
  std::cout<<"256 native session gates match both unchanged original images: 255 real RET4 returns, one genuine pending lower\n";
  return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
