#include "halo2/source_combat_action_tu15.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
int main(int argc,char **argv)try {
  if(argc!=2)throw std::runtime_error("Supply original cache vertex-view fixture");
  nlohmann::json fixture;std::ifstream(argv[1])>>fixture;unsigned cases{};
  for(const auto &row:fixture.at("cases")) {
    SourceRuntimeMemory memory;
    for(const auto &bank:row.at("initial_banks")) {
      const auto hex=bank.at("hex").get<std::string>();std::vector<std::byte> bytes;
      for(unsigned n=0;n<hex.size();n+=2)bytes.push_back(std::byte(std::stoul(hex.substr(n,2),nullptr,16)));
      memory.mount(bank.at("address"),std::move(bytes),SourceRuntimeMemory::Access::ReadWrite,"declared original CPU-view fixture");
    }
    SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("entry_gp").get<decltype(abi.gp)>();abi.eflags=0x202;
    source_native_cache_vertex_view_tu15(memory,abi);
    if(abi.gp!=row.at("exit_gp").get<decltype(abi.gp)>() || (abi.eflags&0xcd5u)!=(row.at("eflags").get<unsigned>()&0xcd5u))
      throw std::runtime_error("Native CPU vertex view differs from whole original GP/defined flags");
    const auto raw=memory.view(row.at("epoch_address"),4);std::uint32_t epoch{};
    for(unsigned n=0;n<4;++n)epoch|=std::to_integer<std::uint32_t>(raw[n])<<(n*8);
    if(epoch!=row.at("epoch"))throw std::runtime_error("CPU vertex view changed the actual cache epoch");
    ++cases;
  }
  if(cases!=180)throw std::runtime_error("Original vertex-view cases were omitted");
  std::cout<<"PASS "<<cases<<" whole-original CPU vertex views; GPU cache command excluded from native claim\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
