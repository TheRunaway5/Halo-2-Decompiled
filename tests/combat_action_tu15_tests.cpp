#include "halo2/source_combat_action_tu15.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
std::vector<std::byte> bytes(const std::string &hex){std::vector<std::byte>b;for(unsigned n=0;n<hex.size();n+=2)b.push_back(std::byte(std::stoul(hex.substr(n,2),nullptr,16)));return b;}
}
int main(int argc,char **argv)try {
  check(argc==2,"Supply original combat action fixture");std::ifstream file(argv[1]);if(!file)return 77;
  const auto fixture=nlohmann::json::parse(file);unsigned cases{};
  for(const auto &c:fixture.at("cases")) {
    SourceRuntimeMemory m;
    for(const auto &b:c.at("initial_banks"))m.mount(b.at("address"),bytes(b.at("hex")),SourceRuntimeMemory::Access::ReadWrite,"declared original action ABI bank");
    SourcePhysicsWorldRuntimeAbi abi{};abi.gp=c.at("entry_gp").get<decltype(abi.gp)>();abi.eflags=0x202;
    std::vector<std::uint32_t> trace;SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto p){trace.push_back(p);};
    services.required_owner=[&](auto &memory,const auto &call,auto &state,const auto &observe){
      if(call.instruction==0x1d2eau && call.target==0x407c00u && !call.tail)
        source_native_cache_bitmap_view_tu15(memory,state,{0x27f,0x1f80},services,observe);
      else throw std::runtime_error("Unexpected combat action child owner");
    };
    source_run_combat_action_tu15(m,c.value("entry",fixture.at("entry").get<unsigned>()),abi,{0x27f,0x1f80},services);
    check(abi.gp==c.at("exit_gp").get<decltype(abi.gp)>(),"Original movement/facing GP output differs");
    check((abi.eflags&0xcd5u)==(c.at("eflags").get<unsigned>()&0xcd5u),"Original movement/facing flags differ");
    check(trace==c.at("instructions").get<decltype(trace)>(),"Original movement/facing instruction path differs");
    for(unsigned n=0;n<8;++n){const auto hex=c.at("exit_xmm")[n].get<std::string>();for(unsigned k=0;k<4;++k)check(abi.xmm[n][k]==std::stoul(hex.substr(24-k*8,8),nullptr,16),"Original movement/facing SSE output differs");}
    for(const auto &b:c.at("final_banks")){const auto expected=bytes(b.at("hex"));const auto actual=m.view(b.at("address"),expected.size());check(std::ranges::equal(actual,expected),"Original movement/facing retained memory differs");}
    ++cases;
  }
  std::cout<<"PASS original TU1.5 combat/bitmap caller: "<<cases<<" instruction/GP/SSE/flag/observed-memory cases\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
