#include "halo2/source_shader_rgb.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <cfenv>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace halo2;
namespace {
void check(bool value,const char *message) {
    if(!value)throw std::runtime_error(message);
}
nlohmann::json read(const char *path) {
    std::ifstream file(path);check(bool(file),"Missing original shader RGB corpus");
    return nlohmann::json::parse(file);
}
std::array<float,3> rgb(const nlohmann::json &bits) {
    std::array<float,3> value;
    for(unsigned n=0;n<3;++n)value[n]=std::bit_cast<float>(bits.at(n).get<std::uint32_t>());
    return value;
}
struct HostRounding {
    int before=std::fegetround();
    ~HostRounding(){std::fesetround(before);}
};
}
int main(int argc,char **argv)try {
    check(argc==3,"Supply whole original RGB fixture and camouflage state compositions");
    const auto original=read(argv[1]),binder=read(argv[2]);
    HostRounding rounding;
    std::size_t conversions=0,compositions=0,mapped=0;
    for(const int host:{FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO}) {
        check(std::fesetround(host)==0,"Host rounding control unavailable");
        for(const auto &row:original.at("cases")) {
            if(source_shader_rgb(rgb(row.at("rgb_bits")),row.at("fpcw"))!=row.at("packed_rgb"))
                throw std::runtime_error("Original RGB packing differs at case "+std::to_string(conversions));
            ++conversions;
        }
        for(const auto &row:binder.at("state_cases")) {
            auto actual=row.at("initial_state75").get<std::uint32_t>();
            // This comparison exercises only the reached RGB conversion and
            // alpha merge. Importing/running the record dispatch is separate.
            if(!row.at("mappings").empty()) {
                check(row.at("mappings")==nlohmann::json::array({"00020028"}),
                    "Authored camouflage state range changed");
                actual=source_shader_rgb_state_word(actual,rgb(row.at("color40_bits")),row.at("requested_fpcw"));
                ++mapped;
            }
            check(actual==row.at("state75"),"Original camouflage RGB/alpha merge differs");
            ++compositions;
        }
    }
    check(conversions==8156*4 && compositions==5040*4 && mapped==2016*4,
        "Original RGB/state coverage changed");
    for(const auto control:{std::uint16_t(0x007f),std::uint16_t(0x017f),std::uint16_t(0x027e)}) {
        bool rejected=false;
        try{(void)source_shader_rgb({0,.25f,1},control);}catch(const std::runtime_error&){rejected=true;}
        check(rejected,"Unadmitted x87 precision or exception control was accepted");
    }
    for(const auto value:{std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}) {
        bool rejected=false;
        try{(void)source_shader_rgb({0,value,1},0x027f);}catch(const std::runtime_error&){rejected=true;}
        check(rejected,"Unadmitted nonfinite RGB input was accepted");
    }
    std::cout<<"8156 whole original RGB cases and 5040 original state compositions passed under four host rounding modes\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
