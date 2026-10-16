#include "halo2/source_object_functions_tu15.hpp"
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
    if(argc!=3)throw FormatError("Supply original object-function fixture and TU15_XBE");
    std::ifstream input(argv[1]);if(!input || !std::filesystem::is_regular_file(argv[2]))return 77;
    const auto f=nlohmann::json::parse(input);if(f.at("xbe_sha256")!="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd")throw FormatError("Original function fixture image differs");
    auto m=load_source_runtime_image(argv[2],SourcePhysicsContactImage::TitleUpdate15);
    m.mount(0x2100000,std::vector<std::byte>(0x100000),SourceRuntimeMemory::Access::ReadWrite,"declared original function banks/headers/stack");
    const auto put=[&](std::uint32_t at,const auto &v){m.write(at,std::as_bytes(std::span(&v,1)));};
    put(0x51daf4u,0x2100000u);put(0x51df54u,0x2104000u);put(0x2104002u,std::uint16_t{30});
    put(0x4ecbd0u,0x2105000u);put(0x2105044u,0x2105100u);put(0x2105114u,0x2106000u);put(0x2105120u,0x2107000u);
    unsigned count{};
    for(const auto &r:f.at("cases")) {
        m.write(0x2100000u,decode(r.at("before")));m.write(0x2106000u,decode(r.at("bodies_before").at(0)));m.write(0x2107000u,decode(r.at("bodies_before").at(1)));put(0x2104008u,r.at("tick").get<std::uint32_t>());
        constexpr std::uint32_t stack=0x21fff00u,stop=0x3000000u;put(stack,stop);SourcePhysicsWorldRuntimeAbi a{};a.gp[4]=stack;a.eflags=2;
        const auto object=r.at("object").get<std::uint32_t>(),name=r.at("name").get<std::uint32_t>();const auto op=r.at("op").get<std::string>();std::uint32_t root{};
        if(op=="set") {put(stack+4,object);put(stack+8,name);put(stack+12,r.at("value").get<std::uint32_t>());a.xmm[4][0]=r.at("frames");root=0x10fc30;}
        else if(op=="clear"){a.gp[7]=object;a.gp[6]=name;root=0x10fd50;}
        else {put(stack+4,name);put(stack+8,0x2108000u);put(stack+12,0x2108004u);put(0x2108000,0x12345678u);put(0x2108004,0x11223344u);a.gp[3]=object;root=0x10fb70;}
        SourcePhysicsWorldRuntimeServices services;services.required_owner=[](auto &,auto &,auto &,auto &){throw FormatError("Complete object-function kernel reached an undeclared owner");};
        source_run_object_functions_tu15(m,root,a,{0x27f,0x1f80},services);
        const auto word=[&](auto at){return Bytes(m.view(at,4)).u32(0);};
        if(!std::ranges::equal(m.view(0x2100000,0x300),decode(r.at("after"))) ||
           !std::ranges::equal(m.view(0x2106000,0x200),decode(r.at("bodies_after").at(0))) ||
           !std::ranges::equal(m.view(0x2107000,0x200),decode(r.at("bodies_after").at(1))) ||
           (op=="get" && ((a.gp[0]&255u)!=r.at("found") || word(0x2108000)!=r.at("value_out") || word(0x2108004)!=r.at("slot_out"))))
            throw FormatError("Original object-function result/bank/body differs at case "+std::to_string(count));
        ++count;
    }
    std::cout<<"PASS "<<count<<" complete original shader-function bank/body/query comparisons including32-slot replacement and clear\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
