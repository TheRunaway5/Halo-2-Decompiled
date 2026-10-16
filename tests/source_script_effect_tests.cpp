#include "halo2/source_script_effect.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <fstream>
#include <iostream>
using namespace halo2;
int main(int argc,char **argv)try {
    if(argc!=2)throw FormatError("Supply original script marker-effect fixture");
    std::ifstream file(argv[1]);nlohmann::json original;file>>original;
    if(original.at("xbe_sha256")!="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd")
        throw FormatError("Script marker-effect fixture has the wrong original image");
    unsigned checked=0,submitted=0;
    for(const auto &row:original.at("cases")) {
        unsigned lookups=0;
        const auto request=source_script_effect_new_on_object_marker(row.at("effect"),row.at("object"),
            row.at("marker"),[&](auto object,auto marker) {
                const auto &call=row.at("lookups").at(lookups);
                if(call.at(0)!=object || call.at(1)!=marker || call.at(2)!=1 || call.at(3)!=0)
                    throw FormatError("Original marker request arguments differ");
                const auto world=std::bit_cast<Transform>(row.at("world").at(lookups).get<std::array<std::uint32_t,13>>());
                const auto count=row.at("counts").at(lookups).get<std::uint16_t>();
                ++lookups;return SourceScriptEffectMarker{count,world};
            });
        if(lookups!=row.at("lookups").size() || bool(request)!=!row.at("submissions").empty())
            throw FormatError("Original marker request admission/call order differs");
        if(request) {
            const auto &expected=row.at("submissions").at(0);
            if(request->parameters(0)!=expected.at("parameters").get<std::array<std::uint32_t,24>>() ||
                std::bit_cast<std::array<std::uint32_t,14>>(request->seeds)!=expected.at("seeds").get<std::array<std::uint32_t,14>>())
                throw FormatError("Complete original script marker-effect parameters/seeds differ at case "+std::to_string(checked));
            const auto placed=request->parameters(0x12345678);
            if(placed[7]!=0x12345678)throw FormatError("Marker request lost its actual seed backing address");
            ++submitted;
        }
        ++checked;
    }
    if(checked!=192 || !submitted)throw FormatError("Original marker-effect fixture coverage missing");
    std::cout<<checked<<" original marker request cases; "<<submitted<<" complete96-byte parameters/two28-byte seeds exact\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
