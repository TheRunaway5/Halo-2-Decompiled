#include "halo2/source_campaign_camera_tu15.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
using namespace halo2;
static std::vector<std::byte> decode(const std::string &hex) {
    std::vector<std::byte> bytes;
    for(std::size_t n=0;n<hex.size();n+=2)bytes.push_back(std::byte(std::stoul(hex.substr(n,2),nullptr,16)));
    return bytes;
}
int main(int argc,char **argv)try {
    if(argc!=2)throw FormatError("Supply original camera-time fixture");
    std::ifstream input(argv[1]);if(!input)return 77;const auto f=nlohmann::json::parse(input);
    unsigned queries{},ticks{};
    for(const auto &row:f.at("queries")) {
        const auto value=source_campaign_camera_time_value_tu15(row.at("frames").get<std::int32_t>(),
            std::bit_cast<float>(row.at("delta").get<std::uint32_t>()));
        if(std::bit_cast<std::uint16_t>(value)!=row.at("result_short").get<std::uint16_t>())
            throw FormatError("Original camera-time value differs at case "+std::to_string(queries));
        ++queries;
    }
    for(const auto &row:f.at("ticks")) {
        SourceRuntimeMemory m;constexpr std::uint32_t director=0x2100000,clock=0x2101000;
        m.mount(0x51d000,std::vector<std::byte>(0x2000),SourceRuntimeMemory::Access::ReadWrite,"declared pointer inputs");
        m.mount(director,decode(row.at("before").get<std::string>()),SourceRuntimeMemory::Access::ReadWrite,"declared director");
        const auto expected_clock=decode(row.at("clock").get<std::string>());
        m.mount(clock,expected_clock,SourceRuntimeMemory::Access::ReadWrite,"declared clock");
        m.write(0x51df6c,std::as_bytes(std::span(&director,1)));m.write(0x51df54,std::as_bytes(std::span(&clock,1)));
        source_campaign_camera_game_tick_tu15(m);
        const auto expected=decode(row.at("after").get<std::string>());
        if(!std::ranges::equal(m.view(director,expected.size()),expected) ||
            !std::ranges::equal(m.view(clock,expected_clock.size()),expected_clock))
            throw FormatError("Original director tick bytes differ at case "+std::to_string(ticks));
        ++ticks;
    }
    std::cout<<"PASS "<<queries<<" original camera-time values and "<<ticks<<" complete bounded director tick banks\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
