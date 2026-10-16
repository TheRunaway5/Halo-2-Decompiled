#pragma once
#include "halo2/renderer_debug_texture.hpp"
#include <array>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <thread>

namespace halo2_test {
inline nlohmann::json debug_texture_gpu_probe(const halo2::RendererDebugTextureFactory& factory) {
    auto require=[](bool value){if(!value)throw std::runtime_error("Native debugger texture ownership/texel proof differs");};
    auto rejected=[&](auto&& action){bool caught=false;try{action();}catch(const std::exception&){caught=true;}require(caught);};
    auto texture=factory.create();const auto original=texture.image();
    require(!texture.content_initialized());rejected([&]{texture.pixels();});
    rejected([&]{texture.upload_linear(std::array<std::byte,2>{});});
    require(!texture.content_initialized());
    auto moved=std::move(texture);rejected([&]{texture.image();});
    require(moved.image().image==original.image&&moved.image().image_backing==original.image_backing&&
        original.width==256&&original.height==256&&original.mips==1&&original.layers==1&&
        original.image_backing.value>0xffffffffu&&original.view_backing.value>0xffffffffu);
    bool foreign=false;std::thread other([&]{try{moved.image();}catch(const std::runtime_error&){foreign=true;}});other.join();require(foreign);
    std::vector<std::byte> words(65536*2);
    for(unsigned i=0;i<65536;++i){words[i*2]=std::byte(i);words[i*2+1]=std::byte(i>>8);}
    moved.upload_linear(words);require(moved.content_initialized());require(moved.pixels()==words);
    // A second complete producer exercises the read->write->read transition.
    for(unsigned i=0;i<65536;++i){const auto word=65535u-i;words[i*2]=std::byte(word);words[i*2+1]=std::byte(word>>8);}
    moved.upload_linear(words);require(moved.pixels()==words);
    const auto mapping=moved.components();const auto format=moved.image().format;
    if(format==VK_FORMAT_R4G4B4A4_UNORM_PACK16)require(mapping.r==VK_COMPONENT_SWIZZLE_G&&mapping.g==VK_COMPONENT_SWIZZLE_B&&
        mapping.b==VK_COMPONENT_SWIZZLE_A&&mapping.a==VK_COMPONENT_SWIZZLE_R);
    else if(format==VK_FORMAT_B4G4R4A4_UNORM_PACK16)require(mapping.r==VK_COMPONENT_SWIZZLE_G&&mapping.g==VK_COMPONENT_SWIZZLE_R&&
        mapping.b==VK_COMPONENT_SWIZZLE_A&&mapping.a==VK_COMPONENT_SWIZZLE_B);
    else require(format==VK_FORMAT_R8G8B8A8_UNORM&&mapping.r==VK_COMPONENT_SWIZZLE_IDENTITY&&mapping.g==VK_COMPONENT_SWIZZLE_IDENTITY&&
        mapping.b==VK_COMPONENT_SWIZZLE_IDENTITY&&mapping.a==VK_COMPONENT_SWIZZLE_IDENTITY);
    return {{"source_texel_values",65536},{"complete_gpu_upload_readbacks",2},{"raw_source_words_exact",true},
        {"selected_vk_format",format},{"source_a4r4g4b4_view_mapping_configured",true},
        {"constructor_content_undefined",true},{"unproduced_readback_and_partial_upload_rejected",true},
        {"move_and_foreign_thread_guards",true},{"game_sampled_consumers_verified",false}};
}
}
