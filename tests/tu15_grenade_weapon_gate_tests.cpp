#include "halo2/physics_live_support_tu15.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}}
int main(int argc,char **argv)try {
  check(argc==2,"Supply original TU1.5 weapon-gate fixture");
  std::ifstream file(argv[1]);if(!file)return 77;
  const auto fixture=nlohmann::json::parse(file);unsigned cases{};
  check(source_physics_live_support_tu15_owns_entry(0x106510),"Missing TU1.5 106510 dispatcher entry");
  for(const auto &c:fixture.at("cases")) {
    SourceRuntimeMemory m;
    const auto mount=[&](std::uint32_t p,std::size_t n){m.mount(p,std::vector<std::byte>(n),SourceRuntimeMemory::Access::ReadWrite,"declared gate ABI backing");};
    mount(0x4ecbd0,4);mount(0x4f0414,4);mount(0x1000000,0x10000);
    const auto put=[&](std::uint32_t p,std::uint32_t v){std::array<std::byte,4>b;for(unsigned k=0;k<4;++k)b[k]=std::byte(v>>(k*8));m.write(p,b);};
    put(0x4ecbd0,0x1000000);put(0x4f0414,0x1001000);put(0x1000044,0x1002000);
    put(0x1002014,0x1003000);put(0x1003000,0x12340002);put(0x1001028,0x1004000);
    put(0x100412c,c.at("flags"));put(0x1003178,std::uint32_t(c.at("state").get<std::int32_t>()));put(0x100f000,0x11000000);
    const auto before=m.checkpoint();
    SourcePhysicsWorldRuntimeAbi abi{};abi.gp=c.at("entry_gp").get<decltype(abi.gp)>();abi.eflags=0x202;
    std::vector<std::uint32_t> trace;SourcePhysicsWorldRuntimeServices services;
    services.instruction=[&](auto p){trace.push_back(p);};
    services.required_owner=[](auto&,auto&,auto&,auto&){throw std::runtime_error("Unexpected gate child owner");};
    source_run_physics_live_support_tu15(m,0x106510,abi,{0x27f,0x1f80},services);
    check(abi.gp==c.at("exit_gp").get<decltype(abi.gp)>(),"Original weapon-gate GP output differs");
    check((abi.eflags&0xcd5u)==(c.at("eflags").get<unsigned>()&0xcd5u),"Original weapon-gate flags differ");
    check(trace==c.at("instructions").get<decltype(trace)>(),"Original weapon-gate instruction path differs");
    check(m.checkpoint()==before,"Weapon gate mutated original input backing");++cases;
  }
  std::cout<<"PASS original TU1.5 grenade weapon gate: "<<cases<<" full instruction/GP/flag cases\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
