#include "halo2/source_water_reflection_tu15.hpp"
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

int main(int argc,char **argv)try {
    using namespace halo2;
    if(argc!=2)throw std::runtime_error("Supply the original TU1.5 reflection fixture");
    std::ifstream input(argv[1]);
    if(!input)throw std::runtime_error("Original reflection fixture is unavailable");
    const auto fixture=nlohmann::json::parse(input);
    if(fixture.at("schema")!="HALO2_WATER_REFLECTION_TU15_ORIGINAL_FIXTURE_V1" ||
       fixture.at("original_xbe_sha256")!="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" ||
       fixture.at("entry")!=0x3bcf0 || !fixture.at("original_instruction_patches").empty() ||
       fixture.at("cases").size()!=128)
        throw std::runtime_error("Original reflection authority or complete fixture domain changed");
    unsigned cases{};
    for(const auto &row:fixture.at("cases")) {
        const auto words=row.at("input_words").get<std::array<std::uint32_t,78>>();
        const auto expected=row.at("expected_words").get<std::array<std::uint32_t,125>>();
        std::array<std::byte,0x74> camera;
        SourceViewProjectionBank bank;
        std::memcpy(camera.data(),words.data(),sizeof(camera));
        std::memcpy(&bank,words.data()+29,sizeof(bank));
        const auto saved_camera=camera;
        const auto saved_bank=std::bit_cast<std::array<std::uint32_t,48>>(bank);
        const auto result=source_water_reflection_tu15(camera,bank,std::bit_cast<float>(words[77]));
        std::array<std::uint32_t,125> actual;
        std::memcpy(actual.data(),result.submitting_camera.data(),sizeof(camera));
        std::memcpy(actual.data()+29,&result.rendering,sizeof(bank));
        std::memcpy(actual.data()+77,result.constants.data(),sizeof(result.constants));
        for(unsigned at=0;at<actual.size();++at)if(actual[at]!=expected[at])
            throw std::runtime_error("Reflection differs from original case "+std::to_string(cases)+
                " word "+std::to_string(at));
        if(camera!=saved_camera || std::bit_cast<std::array<std::uint32_t,48>>(bank)!=saved_bank)
            throw std::runtime_error("Reflection changed its submitting source inputs");
        ++cases;
    }
    std::cout<<"TU1.5 reflection: "<<cases<<" cases, "<<cases*125
             <<" exact words; raw camera, full render bank, 12F80 constants and immutable inputs\n";
    return 0;
} catch(const std::exception &error) {
    std::cerr<<error.what()<<'\n';return 1;
}
