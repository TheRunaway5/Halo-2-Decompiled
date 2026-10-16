#include "halo2/source_campaign_cinematic_tu15.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
using namespace halo2;
int main(int argc,char **argv)try {
    if(argc!=2)throw FormatError("Supply original unit-time fixture");
    std::ifstream input(argv[1]);if(!input)return 77;
    const auto fixture=nlohmann::json::parse(input);unsigned count{};
    for(const auto &row:fixture.at("cases")) {
        const auto seconds=std::bit_cast<float>(row.at("seconds").get<std::uint32_t>());
        const auto actual=source_campaign_unit_animation_ticks_tu15(seconds);
        if(std::bit_cast<std::uint16_t>(actual)!=row.at("result_short").get<std::uint16_t>())
            throw FormatError("Original unit-time conversion differs at case "+std::to_string(count));
        ++count;
    }
    std::cout<<"PASS "<<count<<" original unit-time float32/x87/clamp/short conversions\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
