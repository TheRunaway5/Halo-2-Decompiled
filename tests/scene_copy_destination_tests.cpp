#include "halo2/source_scene_copy.hpp"
#include "halo2/water.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <fstream>
#include <iostream>
using namespace halo2;
namespace {
void check(bool value,const std::string &label){if(!value)throw FormatError(label);}
float number(const nlohmann::json &word){return std::bit_cast<float>(word.get<std::uint32_t>());}
void same(const ShaderVector &actual,const nlohmann::json &expected,const std::string &label) {
    for(unsigned axis=0;axis<4;++axis)
        check(std::bit_cast<std::uint32_t>(actual[axis])==expected.at(axis),label+": word"+std::to_string(axis)+" differs");
}
}
int main(int argc,char **argv)try {
    check(argc==2,"Supply unchanged original scene-copy destination fixture");
    std::ifstream file(argv[1]);check(bool(file),"Missing original destination fixture");
    const auto fixture=nlohmann::json::parse(file);
    check(fixture.at("schema")=="original-scene-copy-destinations-v1","Unexpected original destination fixture");
    check(fixture.at("instruction_patches").empty() && fixture.at("supplied_returns").empty(),"Destination evidence substitutes source behavior");
    const auto &inputs=fixture.at("inputs");const auto &results=fixture.at("versions").at("retail").at("results");
    check(inputs.size()==37 && results.size()==37,"Original destination coverage changed");
    unsigned attributes=0,water_calls=0,taps=0;
    for(unsigned i=0;i<37;++i) {
        const auto &input=inputs.at(i);const auto &row=results.at(i);const std::string label=input.at("name");
        check(row.at("complete_original_return")==true && row.at("stop_before").is_null(),label+": incomplete original call");
        SourceSceneCopyContext context{input.at("camera_rectangle_shorts").get<std::array<std::int16_t,4>>(),
            row.at("destination_dimensions").at(0),row.at("destination_dimensions").at(1),number(row.at("near_bits")),number(row.at("far_bits"))};
        const auto frame=source_scene_copy_frame(context);
        const auto water_program=water_copy_program();
        for(unsigned word=0;word<60;++word) {
            check(frame.program.words[word]==row.at("pixel_program_words").at(word),label+": original copy program differs");
            check(water_program.words[word]==frame.program.words[word],label+": water copy program differs");
        }
        for(unsigned corner=0;corner<4;++corner) {
            const auto &vertex=frame.vertices[corner];const auto &expected=row.at("vertices").at(corner);
            for(unsigned attribute=0;attribute<16;++attribute) {
                const auto &value=expected.at("attributes").at(attribute);
                check(value.at("index")==attribute && value.at("returned")==bool(attribute<6),label+": original attribute admission changed");
                if(attribute>=6)continue;
                same(attribute==0?vertex.position:attribute==5?vertex.corner:vertex.texture_coordinates[attribute-1],value.at("value_bits"),label);
                ++attributes;
            }
        }
        if(context.destination_width==512 && context.destination_height==256) {
            ++water_calls;
            const float source_width=row.at("source_dimensions").at(0),source_height=row.at("source_dimensions").at(1);
            const float width=float(int(context.camera_rectangle[3])-int(context.camera_rectangle[1]));
            const float height=float(int(context.camera_rectangle[2])-int(context.camera_rectangle[0]));
            const auto offsets=water_copy_taps(width/source_width,height/source_height);
            // Project actual original first-corner source-pixel coordinates
            // into this host's normalized relative-offset interface. The
            // original callback does not perform this normalization.
            for(unsigned tap=0;tap<4;++tap) {
                const auto &raw=row.at("vertices").at(0).at("attributes").at(tap+1).at("value_bits");
                const float x=(number(raw.at(0))-float(context.camera_rectangle[1]))/source_width;
                const float y=(number(raw.at(1))-float(context.camera_rectangle[0]))/source_height;
                check(std::bit_cast<std::uint32_t>(offsets[tap][0])==std::bit_cast<std::uint32_t>(x),label+": normalized water X tap differs");
                check(std::bit_cast<std::uint32_t>(offsets[tap][1])==std::bit_cast<std::uint32_t>(y),label+": normalized water Y tap differs");
                check(offsets[tap][2]==0 && offsets[tap][3]==1,label+": water tap Z/W differs");
                ++taps;
            }
        }
    }
    check(attributes==888 && water_calls==24 && taps==96,"Destination/native water projection coverage changed");
    std::cout<<attributes<<" original attributes across37 complete destination calls and60 program words per call passed; "
             <<taps<<" normalized water tap vectors from24 original512x256 calls passed\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
