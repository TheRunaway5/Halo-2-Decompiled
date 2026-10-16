#include "halo2/source_script_effect_tu15.hpp"
#include "halo2/source_runtime_image.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <cstring>
#include <fstream>
#include <iostream>
using namespace halo2;
int main(int argc,char **argv)try {
    if(argc!=3)throw FormatError("Supply original script-marker fixture and TU15_XBE");
    std::ifstream file(argv[1]);if(!file || !std::filesystem::is_regular_file(argv[2]))return 77;
    const auto fixture=nlohmann::json::parse(file);
    if(fixture.at("xbe_sha256")!="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd")
        throw FormatError("Original script-effect fixture image differs");
    auto memory=load_source_runtime_image(argv[2],SourcePhysicsContactImage::TitleUpdate15);
    constexpr auto stack=0x21fff00u,stop=0x3000000u;
    memory.mount(0x2100000,std::vector<std::byte>(0x100000),SourceRuntimeMemory::Access::ReadWrite,"declared original script caller stack");
    const auto words=[&]<std::size_t N>(std::uint32_t at) {
        std::array<std::uint32_t,N> result;
        std::memcpy(result.data(),memory.view(at,sizeof(result)).data(),sizeof(result));return result;
    };
    const auto put=[&](std::uint32_t at,const auto &value){memory.write(at,std::as_bytes(std::span(&value,1)));};
    unsigned checked=0,submitted=0;
    for(const auto &row:fixture.at("cases")) {
        const std::array caller{stop,row.at("effect").get<std::uint32_t>()};put(stack,caller);
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp[4]=stack;abi.gp[0]=row.at("marker");
        abi.gp[7]=row.at("object");abi.eflags=2;
        unsigned lookups=0,allocations=0;
        SourcePhysicsWorldRuntimeServices services;
        services.required_owner=[&](auto &,const auto &call,auto &state,const auto &) {
            const auto sp=state.gp[4];const auto ret=words.template operator()<1>(sp)[0];
            if(call.target==0xbad60u) {
                const auto args=words.template operator()<3>(sp+4u);
                const auto &expected=row.at("lookups").at(lookups);
                if(args[0]!=expected.at(0) || args[1]!=expected.at(1) || args[2]!=1 || (state.gp[1]&255))
                    throw FormatError("Compiled original marker lookup ABI differs");
                const auto world=row.at("world").at(lookups).get<std::array<std::uint32_t,13>>();
                memory.write(state.gp[3]+0x38u,std::as_bytes(std::span(world)));
                state.gp[0]=row.at("counts").at(lookups);state.gp[4]+=16u;++lookups;
            }else if(call.target==0x178e30u) {
                auto parameters=words.template operator()<24>(words.template operator()<1>(sp+4u)[0]);
                const auto seeds=words.template operator()<14>(parameters[7]);parameters[7]=0;
                const auto &expected=row.at("submissions").at(allocations);
                if(parameters!=expected.at("parameters").get<std::array<std::uint32_t,24>>() ||
                    seeds!=expected.at("seeds").get<std::array<std::uint32_t,14>>())
                    throw FormatError("Compiled original complete script-effect parameters/seeds differ");
                state.gp[0]=0xffffffffu;state.gp[4]+=8u;++allocations;
            }else throw FormatError("Compiled script-effect kernel reached an undeclared owner");
            if(ret!=call.instruction+5u)throw FormatError("Original script-effect return address differs");
        };
        source_run_script_effect_tu15(memory,0x2aa360u,abi,{0x27f,0x1f80},services);
        if(lookups!=row.at("lookups").size() || allocations!=row.at("submissions").size() || abi.gp[4]!=stack+8u)
            throw FormatError("Compiled original script-effect gating/call order/stack differs");
        ++checked;submitted+=allocations;
    }
    if(checked!=192 || submitted!=72)throw FormatError("Script-effect kernel coverage missing");
    std::cout<<"PASS "<<checked<<" compiled complete original script-effect cases; "<<submitted<<" complete packets/seeds exact\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
