#include "halo2/source_secondary_trigger_intent_tu15.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
int main(int argc,char**argv)try {
    if(argc!=2)throw std::runtime_error("Supply independent original secondary-intent fixture");
    std::ifstream file(argv[1]);const auto fixture=nlohmann::json::parse(file);
    if(fixture.at("original_xbe_sha256")!="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd")
        throw std::runtime_error("Secondary intent authority changed");
    for(const auto& row:fixture.at("cases"))
        if(halo2::source_secondary_trigger_intent_tu15(row[0].get<std::uint32_t>(),row[1].get<bool>())!=row[2].get<std::uint32_t>())
            throw std::runtime_error("Native secondary intent differs from original complete flag producer");
    std::cout<<"PASS "<<fixture.at("cases").size()<<" original secondary-intent flag cases\n";return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
