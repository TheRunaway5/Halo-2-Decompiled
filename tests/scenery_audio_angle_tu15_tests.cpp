#include "halo2/source_object_loop_audio_tu15.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
struct Memory final:SourcePhysicsContactMemory {
    std::array<std::byte,0x2000> stack{};
    std::map<std::uint32_t,std::array<std::byte,4>> constants;
    void read(std::uint32_t at,std::span<std::byte> out)const override {
        if(at>=0x100d000u && std::uint64_t(at)+out.size()<=0x100f000u)
            std::copy_n(stack.begin()+at-0x100d000u,out.size(),out.begin());
        else if(out.size()==4 && constants.contains(at))std::ranges::copy(constants.at(at),out.begin());
        else throw std::runtime_error("Angle component read an undeclared original operand");
    }
    void write(std::uint32_t at,std::span<const std::byte> in)override {
        if(at<0x100d000u || std::uint64_t(at)+in.size()>0x100f000u)
            throw std::runtime_error("Angle component wrote outside its ABI stack");
        std::ranges::copy(in,stack.begin()+at-0x100d000u);
    }
};
}
int main(int argc,char**argv)try {
    if(argc!=2)throw std::runtime_error("Supply independent original18B580 CPU fixture");
    std::ifstream file(argv[1]);const auto fixture=nlohmann::json::parse(file);
    if(fixture.at("original_xbe_sha256")!="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd")
        throw std::runtime_error("Scenery angle authority changed");
    if(!source_object_loop_audio_tu15_owns_entry(0x18b580))
        throw std::runtime_error("Reached sound-scenery18B580 owner is absent");
    Memory memory;
    for(auto it=fixture.at("operands").begin();it!=fixture.at("operands").end();++it) {
        const auto hex=it.value().get<std::string>();std::array<std::byte,4> bytes;
        for(unsigned n=0;n<4;++n)bytes[n]=std::byte(std::stoul(hex.substr(n*2,2),nullptr,16));
        memory.constants.emplace(std::stoul(it.key(),nullptr,16),bytes);
    }
    const auto before=memory.constants;
    for(const auto& row:fixture.at("cases")) {
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp[0]=0x71392481u;abi.gp[4]=0x100e000u;
        abi.xmm[1][0]=row[0].get<std::uint32_t>();abi.eflags=2;
        source_run_object_loop_audio_tu15(memory,0x18b580,abi,{0x27f,0x1f80},{});
        if((abi.gp[0]&65535u)!=row[1].get<unsigned>())
            throw std::runtime_error("Native sound-scenery angle differs from original complete CPU body");
    }
    if(memory.constants!=before)throw std::runtime_error("Scenery angle changed readonly original data");
    std::cout<<"PASS "<<fixture.at("cases").size()<<" complete original sound-scenery angle results\n";
    return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
