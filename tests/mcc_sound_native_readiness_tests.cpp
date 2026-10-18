#include "halo2/physics_live_support_tu15.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>

namespace {
using namespace halo2;
using Json=nlohmann::json;
void need(bool value,const char* why){if(!value)throw std::runtime_error(why);}
std::vector<std::byte> bytes(std::string_view text) {
    need(text.size()%2==0,"Incomplete original fixture bytes");std::vector<std::byte> out;
    for(unsigned n=0;n<text.size();n+=2)out.push_back(std::byte(std::stoul(std::string(text.substr(n,2)),nullptr,16)));
    return out;
}
struct Memory final:SourcePhysicsContactMemory {
    std::uint32_t base{};std::vector<std::byte> data;
    void read(std::uint32_t at,std::span<std::byte> out)const override {
        need(at>=base&&std::uint64_t(at)-base+out.size()<=data.size(),"NONE query touched an unowned cache/hardware resource");
        std::copy_n(data.begin()+(at-base),out.size(),out.begin());
    }
    void write(std::uint32_t at,std::span<const std::byte> in)override {
        need(at>=base&&std::uint64_t(at)-base+in.size()<=512,"NONE query changed chunk/cache/hardware resources");
        std::copy(in.begin(),in.end(),data.begin()+(at-base));
    }
};
}
int main(int argc,char**argv)try {
    if(argc!=2||!std::filesystem::exists(argv[1]))return 77;
    Json fixture;std::ifstream(argv[1])>>fixture;
    need(fixture.at("schema")=="H2_MCC_NONE_SOUND_READINESS_TU15_V1"&&
         fixture.at("original_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd",
         "Wrong original sound readiness reference");
    need(source_physics_live_support_tu15_owns_entry(0x21f260),"Complete genuine original readiness owner missing");
    for(const auto& c:fixture.at("cases")) {
        Memory memory;memory.base=c.at("address");memory.data=bytes(c.at("before").get<std::string>());
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp=c.at("input_gp").get<decltype(abi.gp)>();abi.eflags=c.at("input_eflags");
        std::vector<unsigned> trace;SourcePhysicsWorldRuntimeServices services;
        services.instruction=[&](auto pc){trace.push_back(pc);};
        services.required_owner=[](auto&,const auto&,auto&,const auto&){throw std::runtime_error("NONE query unexpectedly requested another resource owner");};
        source_run_physics_live_support_tu15(memory,0x21f260,abi,{0x27f,0x1f80},services);
        need(abi.gp==c.at("original_gp").get<decltype(abi.gp)>(),"NONE original GP/RET12 ABI differs");
        const auto mask=c.at("eflags_mask").get<unsigned>();
        need((abi.eflags&mask)==(c.at("original_eflags").get<unsigned>()&mask),"NONE original integer flags differ");
        need(memory.data==bytes(c.at("after").get<std::string>()),"NONE original stack/chunk bytes differ");
        need(abi.gp[0]==0&&std::ranges::find(trace,0x21f2c4u)==trace.end()&&std::ranges::find(trace,0x21f2a3u)==trace.end(),
             "Unloaded resource was allocated/queried as ready");
    }
    std::cout<<"Actual TU1.5 NONE readiness differential passed: "<<fixture.at("cases").size()<<" original CPU cases; no resource pools mapped\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
