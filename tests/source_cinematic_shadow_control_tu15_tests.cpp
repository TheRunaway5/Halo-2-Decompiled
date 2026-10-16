#include "halo2/source_cinematic_shadow_control_tu15.hpp"
#include "halo2/source_runtime_image.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
using namespace halo2;
static std::vector<std::byte> decode(const std::string &s) {
    std::vector<std::byte> out;for(std::size_t n=0;n<s.size();n+=2)out.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return out;
}
int main(int argc,char **argv)try {
    if(argc!=3)throw FormatError("Supply original shadow-control fixture and TU15_XBE");
    std::ifstream input(argv[1]);if(!input || !std::filesystem::is_regular_file(argv[2]))return 77;
    const auto fixture=nlohmann::json::parse(input);
    if(fixture.at("xbe_sha256")!="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd")
        throw FormatError("Original shadow-control image differs");
    auto memory=load_source_runtime_image(argv[2],SourcePhysicsContactImage::TitleUpdate15);
    memory.mount(0x2100000,std::vector<std::byte>(0x100000),SourceRuntimeMemory::Access::ReadWrite,"declared original caller stack");
    const auto put=[&](auto at,const auto &value){memory.write(at,std::as_bytes(std::span(&value,1)));};
    unsigned count{};
    for(const auto &row:fixture.at("cases")) {
        memory.write(0x4c6894,decode(row.at("before")));
        if(row.at("parsed").get<bool>()) {
            constexpr std::uint32_t stack=0x21fff00,stop=0x3000000;put(stack,stop);put(stack+4,row.at("radius").get<std::uint32_t>());
            SourcePhysicsWorldRuntimeAbi abi{};abi.gp[4]=stack;abi.gp[3]=row.at("enabled").get<bool>();
            abi.gp[6]=row.at("object");abi.gp[7]=row.at("marker");abi.eflags=2;
            SourcePhysicsWorldRuntimeServices services;services.required_owner=[](auto &,auto &,auto &,auto &){throw FormatError("Complete shadow control reached an undeclared callee");};
            source_run_cinematic_shadow_control_tu15(memory,0x30e10,abi,{0x27f,0x1f80},services);
            if(abi.gp[4]!=stack+8)throw FormatError("Original shadow-control caller stack differs");
        }
        if(!std::ranges::equal(memory.view(0x4c6894,0xc4),decode(row.at("after"))))
            throw FormatError("Complete original shadow bank differs at case "+std::to_string(count));
        ++count;
    }
    if(count<240)throw FormatError("Incomplete original shadow-control fixture");
    std::cout<<"PASS "<<count<<" complete original cinematic-shadow bank comparisons\n";
    std::cout<<"LIMIT parsed-argument boundary; no projected shadow geometry or rendered shadow pixels\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
