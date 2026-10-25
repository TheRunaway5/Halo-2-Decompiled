#include "halo2/kernels.hpp"
#include <bit>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <xmmintrin.h>
using Json=nlohmann::json;
int main(int argc,char **argv)try {
    if(argc!=3)throw std::runtime_error("Supply original multiplier fixture and bounded result path");
    std::ifstream input(argv[1]);const auto fixture=Json::parse(input);
    if(fixture.at("schema")!="H2_DAMAGE_MULTIPLIER_TU15_V1")throw std::runtime_error("Wrong multiplier fixture");
    Json results=Json::array();unsigned differences{};
    for(const auto &row:fixture.at("cases")) {
        std::vector<halo2::DamageGroup> groups;
        for(const auto &entry:row.at("groups")) {
            halo2::DamageGroup group;group.name=entry.at("name");
            for(const auto &modifier:entry.at("modifiers"))
                group.modifiers.push_back({modifier.at("name"),std::bit_cast<float>(modifier.at("bits").get<std::uint32_t>())});
            groups.push_back(std::move(group));
        }
        const auto keys=row.at("keys").get<std::array<std::uint32_t,4>>();
        const auto saved_mxcsr=_mm_getcsr();unsigned short saved_control{},control{};
        const unsigned short wanted=row.at("x87_control_word");
        asm volatile("fnstcw %0":"=m"(saved_control));
        asm volatile("fldcw %0"::"m"(wanted));
        _mm_setcsr(row.at("mxcsr"));
        const auto result=halo2::damage_multiplier(groups,keys[0],keys[1],keys[2],keys[3]);
        const auto actual_bits=std::bit_cast<std::uint32_t>(result),actual_mxcsr=_mm_getcsr();
        asm volatile("fnstcw %0":"=m"(control));
        _mm_setcsr(saved_mxcsr);asm volatile("fldcw %0"::"m"(saved_control));
        const bool equal=actual_bits==row.at("expected_bits") && actual_mxcsr==row.at("expected_mxcsr") && control==wanted;
        differences+=!equal;
        results.push_back({{"id",row.at("id")},{"name",row.at("name")},{"passed",equal},
            {"expected_bits",row.at("expected_bits")},{"actual_bits",actual_bits},
            {"expected_mxcsr",row.at("expected_mxcsr")},{"actual_mxcsr",actual_mxcsr},
            {"x87_control_word",control}});
    }
    Json report{{"passed",differences==0},{"cases",results.size()},{"differences",differences},{"results",results},
        {"scope","Pure native return bits and FP controls/status only; no original GP/stack or full gameplay acceptance"}};
    std::ofstream output(argv[2]);output<<report.dump(2)<<'\n';
    std::cout<<"Multiplier cases="<<results.size()<<" differences="<<differences<<'\n';
    return differences?1:0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
