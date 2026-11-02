#include "halo2/source_loop_detail_control_tu15.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
int main(int argc,char** argv)try {
#if !((defined(__i386__) || defined(__x86_64__)) && (defined(__GNUC__) || defined(__clang__)))
    return 77;
#endif
    if(argc!=2)throw std::runtime_error("Supply original centered-detail prefix fixture");
    const auto fixture=nlohmann::json::parse(std::ifstream(argv[1]));
    if(fixture.at("original_xbe_sha256")!="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" ||
        fixture.at("cases").size()!=240 || !fixture.at("supplied_callee_results").empty() ||
        !fixture.at("source_instruction_patches").empty())throw std::runtime_error("Centered-detail reference authority differs");
    unsigned index{};
    const auto scalar=[](const auto& value){return std::bit_cast<float>(value.template get<std::uint32_t>());};
    for(const auto& row:fixture.at("cases")) {
        auto seed=row.at("seed").get<std::uint32_t>();const auto& b=row.at("bounds");
        std::array<std::byte,68> parent{};const auto hex=row.at("parent").get<std::string>();
        for(unsigned n=0;n<68;++n)parent[n]=std::byte(std::stoul(hex.substr(n*2,2),nullptr,16));
        std::vector<Vec3> listeners;for(const auto& p:row.at("listeners"))listeners.push_back({scalar(p[0]),scalar(p[1]),scalar(p[2])});
        const auto value=source_loop_detail_centered_tu15(parent,{{scalar(b[0]),scalar(b[1])},
            {scalar(b[2]),scalar(b[3])},{scalar(b[4]),scalar(b[5])}},listeners,seed);
        nlohmann::json actual;
        if(value)actual=nlohmann::json::array({std::bit_cast<std::uint32_t>(value->control),
            std::bit_cast<std::uint32_t>(value->gain_db),std::bit_cast<std::uint32_t>(value->position.x),
            std::bit_cast<std::uint32_t>(value->position.y),std::bit_cast<std::uint32_t>(value->position.z)});
        if(actual!=row.at("result") || seed!=row.at("seed_after"))
            throw std::runtime_error("Centered detail differs from original prefix "+std::to_string(index));
        ++index;
    }
    std::cout<<"PASS "<<index<<" unchanged-original centered detail prefixes and random postimages\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
