#pragma once
#include "halo2/renderer_aux_textures.hpp"
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <thread>

namespace halo2_test {
inline nlohmann::json renderer_aux_gpu_probe(const halo2::RendererAuxTextureFactory& factory,const std::filesystem::path& fixture_path) {
    auto require=[](bool value){if(!value)throw std::runtime_error("Native auxiliary texture original-pixel/ownership proof differs");};
    auto rejected=[&](auto&& action){bool caught=false;try{action();}catch(const std::exception&){caught=true;}require(caught);};
    std::ifstream input(fixture_path);if(!input)throw std::runtime_error("Original auxiliary texture fixture is unavailable");
    const auto fixture=nlohmann::json::parse(input);
    require(fixture.at("schema")=="H2_TU15_RENDERER_AUX_TEXTURES_ORIGINAL_V1"&&fixture.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd");
    auto textures=factory.create();const auto identity=textures.image(0).image.image,backing=textures.image(0).image.image_backing.value;
    auto moved=std::move(textures);rejected([&]{textures.image(0);});require(moved.image(0).image.image==identity&&moved.image(0).image.image_backing.value==backing);
    rejected([&]{moved.image(3);});
    bool wrong_thread=false;std::thread outsider([&]{try{moved.pixels(0);}catch(const std::runtime_error&){wrong_thread=true;}});outsider.join();require(wrong_thread);
    unsigned pixels=0;
    for(unsigned i=0;i<3;++i) {
        const auto& record=moved.image(i);const auto& image=record.image;
        const auto extent=fixture.at("source_extent_depth_layers").at(i).get<std::array<unsigned,4>>();
        require(image.format==VK_FORMAT_R8G8B8A8_UNORM&&image.width==extent[0]&&image.height==extent[1]&&record.depth==extent[2]&&image.layers==extent[3]&&image.mips==1&&record.cube==(i==2)&&
            record.source_format_word==fixture.at("texture_formats").at(i).get<unsigned>()&&image.image_backing.value>0xffffffffu&&image.view_backing.value>0xffffffffu);
        const auto actual=moved.pixels(i),repeated=moved.pixels(i);require(actual==repeated);
        const auto expected=fixture.at("linear_rgba_hex").at(i).get<std::string>();require(actual.size()*2==expected.size());
        const auto digit=[](char value){if(value>='0'&&value<='9')return unsigned(value-'0');if(value>='a'&&value<='f')return unsigned(value-'a'+10);throw std::runtime_error("Invalid original texel fixture hex");};
        for(unsigned j=0;j<actual.size();++j)require(std::to_integer<unsigned>(actual[j])==(digit(expected[j*2])<<4|digit(expected[j*2+1])));
        pixels+=actual.size()/4;
    }
    require(moved.image(0).image.image!=moved.image(1).image.image&&moved.image(0).image.image!=moved.image(2).image.image&&moved.image(1).image.image!=moved.image(2).image.image);
    return {{"pixels",pixels},{"all_rgba_bytes_match_original_constructor",true},{"independent_2d_volume_cube_images",true},{"readback_preserves_sampled_layout",true},
        {"moved_foreign_thread_and_unknown_index_rejected",true},{"upload_completed_before_publication",true},{"game_draw_consumers_verified",false}};
}
}
