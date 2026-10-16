#include "halo2/source_unit_cinematic_tu15.hpp"
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
    if(argc!=3)throw FormatError("Supply original emotional-state fixture and TU15_XBE");
    std::ifstream input(argv[1]);if(!input || !std::filesystem::is_regular_file(argv[2]))return 77;
    const auto f=nlohmann::json::parse(input);if(f.at("xbe_sha256")!="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd")throw FormatError("Original emotional fixture image differs");
    auto m=load_source_runtime_image(argv[2],SourcePhysicsContactImage::TitleUpdate15);m.mount(0x2100000,std::vector<std::byte>(0x100000),SourceRuntimeMemory::Access::ReadWrite,"declared unit/clock/header/stack");
    const auto put=[&](std::uint32_t at,const auto &v){m.write(at,std::as_bytes(std::span(&v,1)));};
    put(0x4ecbd0u,0x2103000u);put(0x2103044u,0x2103100u);put(0x2103114u,0x2100000u);put(0x51df54u,0x2102000u);unsigned count{};
    for(const auto &r:f.at("cases")) {
        m.write(0x2100000,decode(r.at("before")));const auto clock=decode(r.at("clock"));m.write(0x2102000,clock);
        if(r.at("object").get<std::uint32_t>()!=0xffffffffu) {
            const auto seconds=source_unit_cinematic_emotion_seconds_tu15(r.at("frames").get<std::int16_t>(),Bytes(m.view(0x46a17c,4)).f32(0));
            if(std::bit_cast<std::uint32_t>(seconds)!=r.at("call_arguments").at(0).at(1))throw FormatError("Original emotional duration conversion differs");
            constexpr std::uint32_t stack=0x21fff00,stop=0x3000000;put(stack,stop);put(stack+4,r.at("weight").get<std::uint32_t>());put(stack+8,std::bit_cast<std::uint32_t>(seconds));
            SourcePhysicsWorldRuntimeAbi a{};a.gp[4]=stack;a.gp[0]=r.at("object");a.gp[1]=r.at("name");a.eflags=2;
            SourcePhysicsWorldRuntimeServices services;services.required_owner=[](auto &,auto &,auto &,auto &){throw FormatError("Complete emotional-state owners reached an undeclared callee");};
            source_run_unit_cinematic_tu15(m,0x111080,a,{0x27f,0x1f80},services);
            if(a.gp[4]!=stack+12)throw FormatError("Original emotional-state stack differs");
        }
        if(!std::ranges::equal(m.view(0x2100000,0x600),decode(r.at("after"))) || !std::ranges::equal(m.view(0x2102000,0x24),clock))
            throw FormatError("Complete original emotional body/clock differs at case "+std::to_string(count));
        ++count;
    }
    std::cout<<"PASS "<<count<<" complete original emotional body/clock comparisons and signed-short duration conversion\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
