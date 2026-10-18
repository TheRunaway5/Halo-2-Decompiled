#include "halo2/source_particle_world_execution_tu15.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
using namespace halo2;using Json=nlohmann::json;
namespace {
Json load(const std::filesystem::path &p){std::ifstream f(p);return Json::parse(f);}
Json state(const SourcePhysicsWorldRuntimeAbi &a){return Json{{"gp",a.gp},{"xmm",a.xmm},{"eflags",a.eflags},{"fs_base",a.fs_base},{"x87_depth",a.x87_depth},{"x87_status",a.x87_status},{"x87_control",a.floating_point->x87_control_word},{"mxcsr",a.floating_point->mxcsr}};}
template<class F>void regions(const std::filesystem::path &p,F &&visit){std::ifstream f(p,std::ios::binary);if(!f)throw FormatError("Missing captured original World bank");while(f.peek()!=EOF){unsigned address,size;f.read(reinterpret_cast<char*>(&address),4);f.read(reinterpret_cast<char*>(&size),4);if(!f || !size || size>0x40000000u)throw FormatError("Invalid captured World bank extent");std::vector<std::byte> bytes(size);f.read(reinterpret_cast<char*>(bytes.data()),size);if(!f)throw FormatError("Truncated captured World bank");visit(address,std::span<const std::byte>(bytes));}}
}
int main(int argc,char **argv)try {
    if(argc!=2 && argc!=3)throw FormatError("Supply actual before/after main40E80 capture and optional new output directory");
    const std::filesystem::path capture=argv[1],out=argc==3?argv[2]:"";
    if(!std::filesystem::exists(capture/"before.bin") || !std::filesystem::exists(capture/"after.bin"))return 77;
    if(!out.empty()){if(std::filesystem::exists(out))throw FormatError("Preserve earlier World execution evidence");std::filesystem::create_directories(out);}
    SourceRuntimeMemory memory;regions(capture/"before.bin",[&](unsigned p,auto b){memory.mount(p,b,SourceRuntimeMemory::Access::ReadWrite,"retained actual live main40E80 input");});
    const auto input=load(capture/"before-abi.json");SourcePhysicsWorldRuntimeAbi abi{};
    abi.gp=input.at("gp").get<decltype(abi.gp)>();abi.xmm=input.at("xmm").get<decltype(abi.xmm)>();abi.eflags=input.at("eflags");abi.fs_base=input.at("fs_base");
    abi.x87_depth=input.at("x87_depth");abi.x87_status=input.at("x87_status");abi.floating_point=SourcePhysicsFactoryFloatingPoint{input.at("x87_control"),input.at("mxcsr")};
    if(abi.gp[0]!=0 || !source_particle_world_execution_tu15_owns_entry(0x40e80) || source_particle_world_execution_tu15_owns_entry(0x12345678))throw FormatError("World executor lost its actual main/closed entry boundary");
    const auto before=state(abi);std::vector<unsigned> trace;unsigned missing{},site{};std::string error;
    SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto pc){trace.push_back(pc);};
    services.required_owner=[&](auto &,const auto &call,auto &,const auto &){missing=call.target;site=call.instruction;throw FormatError("Captured collection requires an additional actual World service");};
    try{source_run_particle_world_execution_tu15(memory,0x40e80,abi,*abi.floating_point,services);}catch(const std::exception &e){error=e.what();}
    std::size_t bytes{};std::vector<unsigned> different;
    if(error.empty())regions(capture/"after.bin",[&](unsigned p,auto b){const auto actual=memory.view(p,b.size());bytes+=b.size();if(!std::ranges::equal(actual,b))different.push_back(p);});
    const auto expected=load(capture/"after-abi.json");const bool abi_exact=state(abi)==expected;
    if(!out.empty())std::ofstream(out/"report.json")<<Json{{"completed",error.empty()},{"error",error},{"missing_owner",missing},{"missing_site",site},{"entry",0x40e80},{"before",before},{"after",state(abi)},{"instructions",trace},{"retained_bytes_compared",bytes},{"different_banks",different},{"captured_return_abi_exact",abi_exact},{"game_returns_supplied",Json::array()},{"gpu_returns_supplied",Json::array()},{"live_world_gpu_handoff_claimed",false},{"full_goal_complete",false}}.dump(2)<<'\n';
    if(!error.empty()){std::cerr<<error<<" at0x"<<std::hex<<site<<" ->0x"<<missing<<'\n';return 1;}
    if(!different.empty() || !abi_exact)throw FormatError("Closed World execution differs from its actual live collection capture");
    std::cout<<"PASS actual retained main40E80 collection through the particle/world executor: "<<trace.size()<<" instructions, "<<bytes<<" source bytes and complete return ABI exact\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
