#include "halo2/death_cycle_runtime_tu15.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

namespace {
void check(bool value,const char *message) {if(!value)throw std::runtime_error(message);}
std::vector<std::byte> bytes(const nlohmann::json &encoded) {
  if(encoded.is_object()) {
    std::vector<std::byte> result(encoded.at("size"),std::byte(encoded.at("fill").get<unsigned>()));
    for(const auto &patch:encoded.at("patches")) {
      const auto data=bytes(patch.at("bytes"));const auto offset=patch.at("offset").get<std::size_t>();
      check(offset<=result.size() && data.size()<=result.size()-offset,"Fixture patch escapes its region");
      std::copy(data.begin(),data.end(),result.begin()+offset);
    }
    return result;
  }
  const auto text=encoded.get<std::string>();
  std::vector<std::byte> result;
  for(std::size_t n=0;n<text.size();n+=2)result.push_back(std::byte(std::stoul(text.substr(n,2),nullptr,16)));
  return result;
}
}
int main(int argc,char **argv)try {
  check(argc==2,"Supply original TU1.5 helper fixtures");
  std::ifstream input(argv[1]);if(!input)return 77;
  const auto fixture=nlohmann::json::parse(input);
  check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong original image");
  std::size_t count{},stores{};
  for(const auto &c:fixture.at("cases")) {
    halo2::SourceRuntimeMemory memory;
    for(const auto &region:c.at("regions"))memory.mount(region.at("address"),bytes(region.at("bytes")),
        halo2::SourceRuntimeMemory::Access::ReadWrite,"Declared original helper input");
    halo2::SourcePhysicsWorldRuntimeAbi abi{};
    abi.gp=c.at("gp").get<decltype(abi.gp)>();abi.xmm=c.at("xmm").get<decltype(abi.xmm)>();
    abi.eflags=c.at("flags");abi.floating_point=halo2::SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
    std::size_t index{};
    struct Stop{};bool stopped{};
    halo2::SourcePhysicsWorldRuntimeServices services;
    services.instruction=[&](std::uint32_t pc) {
      if(c.contains("stop") && c.at("stop")!=0xdbfccu && c.at("stop")!=0xd9ca4u && pc==c.at("stop"))throw Stop{};
    };
    services.required_owner=[&](auto &,const auto &call,auto &,const auto &) {
      if(c.contains("stop") && call.target==c.at("stop"))throw Stop{};
      throw std::runtime_error("Unexpected physical-block lower boundary");
    };
    try {halo2::source_run_death_cycle_runtime_tu15(memory,c.at("entry"),abi,{0x27f,0x1f80},services,[&](const auto &store) {
      check(index<c.at("stores").size(),"Extra native store");const auto &expected=c.at("stores")[index++];
      check(store.instruction==expected.at("instruction") && store.address==expected.at("address") &&
          store.before==bytes(expected.at("before")) && store.written==bytes(expected.at("after")),"Original helper ordered store differs");
    });}catch(const Stop &){stopped=true;}
    check(stopped==(c.contains("stop") && c.at("stop")!=0xdbfccu && c.at("stop")!=0xd9ca4u),"Original physical-block stop differs");
    check(index==c.at("stores").size(),"Missing native store");
    check(abi.gp==c.at("final_gp").get<decltype(abi.gp)>(),"Original helper GP ABI differs");
    check(abi.xmm==c.at("final_xmm").get<decltype(abi.xmm)>(),"Original helper SSE ABI differs");
    // AF for shifts is undefined; compare the defined arithmetic flags here.
    check((abi.eflags&0x8c5u)==(c.at("final_flags").get<unsigned>()&0x8c5u),"Original helper flags differ");
    const auto final=bytes(c.at("final_bytes"));
    check(std::ranges::equal(memory.view(0x70000000u,final.size()),final),"Original helper retained bytes differ");
    stores+=index;++count;
  }
  check(count==224,"Original helper coverage changed");
  std::cout<<count<<" original helper cases, "<<stores<<" ordered stores, retained bytes and GP/SSE ABI match\n";
  return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
