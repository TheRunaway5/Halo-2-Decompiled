#include "halo2/source_campaign_screen_tu15.hpp"
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
    if(argc!=3)throw FormatError("Supply original screen-bank fixture and TU15_XBE");
    std::ifstream input(argv[1]);if(!input || !std::filesystem::is_regular_file(argv[2]))return 77;
    const auto fixture=nlohmann::json::parse(input);
    if(fixture.at("xbe_sha256")!="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd")throw FormatError("Original screen fixture image differs");
    auto m=load_source_runtime_image(argv[2],SourcePhysicsContactImage::TitleUpdate15);
    constexpr std::uint32_t bank=0x2100000,stack=0x21fff00,stop=0x3000000;
    m.mount(bank,std::vector<std::byte>(0x100000),SourceRuntimeMemory::Access::ReadWrite,"declared screen bank/caller stack");
    m.write(0x515ec0,std::as_bytes(std::span(&bank,1)));unsigned count{};
    const auto execute=[&](std::uint32_t root) {
        m.write(stack,std::as_bytes(std::span(&stop,1)));SourcePhysicsWorldRuntimeAbi abi{};abi.gp[4]=stack;abi.eflags=2;
        SourcePhysicsWorldRuntimeServices services;services.required_owner=[](auto &,auto &,auto &,auto &){throw FormatError("Complete screen kernel reached an undeclared owner");};
        source_run_campaign_screen_tu15(m,root,abi,{0x27f,0x1f80},services);
        if(abi.gp[4]!=stack+4)throw FormatError("Complete screen kernel stack differs");
    };
    for(const auto &row:fixture.at("cases")) {
        m.write(bank,decode(row.at("before")));m.write(0x4c1eb0,decode(row.at("target_before")));
        const auto clock_bytes=decode(row.at("clock_hex"));const Bytes c(clock_bytes);
        const auto clock=std::bit_cast<double>(std::uint64_t(c.u32(0))|(std::uint64_t(c.u32(4))<<32));
        source_campaign_screen_clock_tu15(m,clock);
        const auto op=row.at("op").get<std::string>();
        if(op=="start")source_campaign_screen_start_tu15(m,row.at("reset"));
        else if(op=="set" || op=="set2")source_campaign_crossfade_set_tu15(m,Bytes(decode(row.at("value_hex"))).f32(0),Bytes(decode(row.at("strength_hex"))).f32(0));
        else execute(op=="update"?0x1fd00u:op=="ack"?0x1fce0u:0x1fcb0u);
        if(!std::ranges::equal(m.view(bank,0x3fc),decode(row.at("after"))) ||
           !std::ranges::equal(m.view(0x4c1eb0,16),decode(row.at("target_after"))) ||
           !std::ranges::equal(m.view(0x4920c0,8),clock_bytes) || !std::ranges::equal(m.view(0x4c6860,8),clock_bytes))
            throw FormatError("Complete original screen bytes differ at case "+std::to_string(count)+" "+op);
        ++count;
    }
    std::cout<<"PASS "<<count<<" complete original screen-bank/target-flag comparisons; whole update/stop/ack without external owners\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
