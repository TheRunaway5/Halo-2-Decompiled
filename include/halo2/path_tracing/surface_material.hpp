#pragma once
#include "halo2/path_tracing/heretic_material.hpp"
#include "halo2/path_tracing/surface.hpp"
#include <algorithm>
#include <bit>

namespace halo2::path_tracing {
// Model shader roots often share a generic global material ID across parts,
// or omit it altogether. These authored shader owners identify the visible
// finish within those models. Keep the lookup scoped to the original tags.
inline SourceSurfaceMaterial model_surface_material(std::string_view name) {
    using Class=SourceSurfaceMaterial;
    const auto slash=name.find_last_of('\\');
    if(slash==std::string_view::npos)return Class::Unknown;
    const auto owner=name.substr(0,slash),leaf=name.substr(slash+1);
    if(const auto finish=heretic_surface_material(owner,leaf);finish!=Class::Unknown)return finish;
    if(owner=="objects\\weapons\\rifle\\smg\\shaders") {
        if(leaf=="smg_painted_metal" || leaf=="smg_lod")return Class::CoatedMetal;
        if(leaf=="smg_bare_metal")return Class::Metal;
        if(leaf=="smg_rubber_grip")return Class::Rubber;
    }
    if(owner=="objects\\weapons\\rifle\\battle_rifle\\shaders") {
        if(leaf=="battle_rifle_metal")return Class::Metal;
        if(leaf=="battle_rifle_lod")return Class::CoatedMetal;
        if(leaf=="battle_rifle_plastic")return Class::Polymer;
        if(leaf=="battle_rifle_scope")return Class::Rubber;
        if(leaf=="battle_rifle_lens")return Class::Lens;
    }
    if(owner=="objects\\vehicles\\ghost\\shaders") {
        if(leaf=="ghost_shell")return Class::CoatedMetal;
        if(leaf=="ghost_metal")return Class::Metal;
    }
    if(owner=="objects\\vehicles\\warthog\\shaders") {
        if(leaf=="chassis")return Class::CoatedMetal;
        if(leaf=="metal" || leaf=="tires_hubless")return Class::Metal;
        if(leaf=="tires")return Class::Rubber;
        if(leaf=="vinyl")return Class::Polymer;
    }
    if(owner=="objects\\characters\\grunt\\shaders") {
        if(leaf=="grunt_torso" || leaf=="grunt_backpack")return Class::Polymer;
        if(leaf=="grunt_head")return Class::Skin;
    }
    if(owner=="objects\\characters\\jackal\\shaders") {
        if(leaf=="jackal_torso" || leaf=="jackal_legs" ||
           leaf=="jackal_armor_torso" || leaf=="jackal_armor_legs")return Class::Polymer;
        if(leaf=="jackal_head" || leaf=="jackal_arms")return Class::Skin;
    }
    if(owner=="objects\\characters\\elite\\shaders") {
        if(leaf=="helmet" || leaf=="arms" || leaf=="torso" || leaf=="legs")return Class::CoatedMetal;
    }
    if(owner=="objects\\characters\\marine\\shaders") {
        if(leaf.starts_with("head_"))return Class::Skin;
        if(leaf=="marine_standard" || leaf=="marine_dress")return Class::Fabric;
        if(leaf=="packs")return Class::Polymer;
        if(leaf=="helmet_standard" || leaf=="odst_armor")return Class::Metal;
        if(leaf=="eyes_blue" || leaf=="eyes_brown" || leaf=="odst_visor" ||
           leaf=="helmet_pilot_visor")return Class::Lens;
    }
    return Class::Unknown;
}
// Use whole authored name tokens. For bitmaps and shader-name fallback, only
// pass the tag leaf so a storage directory cannot turn every asset into metal.
inline SourceSurfaceMaterial surface_material_class(std::string_view name) {
    using Class=SourceSurfaceMaterial;
    const auto token=[&](std::string_view expected) {
        for(auto left=name;!left.empty();) {
            const auto split=left.find('_');
            if(left.substr(0,split)==expected)return true;
            if(split==std::string_view::npos)break;
            left.remove_prefix(split+1);
        }
        return false;
    };
    if(token("water"))return Class::Water;
    if(token("visor") || token("lens") || token("glass"))return Class::Lens;
    if(token("polished") || token("chrome") || token("mirror"))return Class::Polished;
    if(token("skin") || token("flesh"))return Class::Skin;
    if(token("fabric") || token("cloth") || token("canvas"))return Class::Fabric;
    if(token("polymer") || token("plastic") || token("vinyl"))return Class::Polymer;
    if(token("rubber") || token("tire") || token("tires"))return Class::Rubber;
    if(token("metal") && (token("painted") || token("coated")))return Class::CoatedMetal;
    if(token("metal"))return Class::Metal;
    if(token("grass") || token("weeds"))return Class::Grass;
    if(token("sand") || token("beach"))return Class::Sand;
    if(token("dirt") || token("soil"))return Class::Soil;
    if(token("concrete") || token("stucco"))return Class::Concrete;
    if(token("stone") || token("rock") || token("rocks") || token("cliff") ||
       token("cliffs") || token("sandstone") || token("slate"))return Class::Stone;
    return Class::Unknown;
}
inline SourceSurfaceMaterial surface_material_tag_class(std::string_view name) {
    return surface_material_class(name.substr(name.find_last_of('\\')+1));
}
inline SourceSurfaceMaterial terrain_bitmap_class(std::string_view name) {
    const auto category=surface_material_tag_class(name);
    if(category!=SourceSurfaceMaterial::Unknown)return category;
    // These authored detail-map libraries describe their physical category
    // in the collection path. Their bitmap leaves name only the pattern.
    if(name.starts_with("scenarios\\bitmaps\\detail_maps\\nature\\grass\\"))return SourceSurfaceMaterial::Grass;
    if(name.starts_with("scenarios\\bitmaps\\detail_maps\\concrete\\"))return SourceSurfaceMaterial::Concrete;
    return SourceSurfaceMaterial::Unknown;
}

struct SurfaceMaterialResponse {
    std::uint8_t normal=255,grazing=255;
    float roughness{};
    float max_facing=1.f,max_glancing=1.f;
};
inline constexpr SurfaceMaterialResponse surface_material_response(SourceSurfaceMaterial material) {
    using Class=SourceSurfaceMaterial;
    switch(material) {
    case Class::Sand:return {201,201,.89f,.035f,.08f};
    case Class::Soil:return {222,222,.84f,.045f,.10f};
    case Class::Grass:return {237,255,.78f,.05f,.12f};
    case Class::Concrete:return {244,244,.76f,.07f,.14f};
    case Class::Stone:return {244,244,.80f,.065f,.14f};
    case Class::Rubber:return {214,214,.90f,.04f,.08f};
    case Class::Metal:return {255,255,.53f,.44f,.70f};
    case Class::Polished:return {255,255,.26f};
    case Class::CoatedMetal:return {250,255,.51f,.28f,.52f};
    case Class::Polymer:return {224,234,.76f,.075f,.16f};
    case Class::Fabric:return {224,244,.90f,.025f,.065f};
    case Class::Skin:return {232,245,.74f,.055f,.10f};
    case Class::Lens:return {255,255,.18f};
    default:return {};
    }
}
inline constexpr bool uses_terrain_blend(SurfaceFamily family) {
    return family==SurfaceFamily::TerrainBlend || family==SurfaceFamily::TerrainBlendThreeStage;
}
inline constexpr std::uint32_t pack_surface_material_response(
    SurfaceMaterialResponse first,SurfaceMaterialResponse second) {
    if(first.normal==255 && first.grazing==255 && second.normal==255 && second.grazing==255)return 0;
    return std::uint32_t(first.normal)|(std::uint32_t(first.grazing)<<8)|
        (std::uint32_t(second.normal)<<16)|(std::uint32_t(second.grazing)<<24);
}
inline void apply_surface_material_response(const Material& material,SurfaceFamily family,
    std::array<std::uint32_t,4>& detail,std::array<float,4>& response) {
    const auto root=surface_material_response(material.source_surface_material);
    auto first=root,second=root;
    if(uses_terrain_blend(family)) {
        first=surface_material_response(material.source_terrain_materials[0]);
        second=surface_material_response(material.source_terrain_materials[1]);
    }
    detail[2]=pack_surface_material_response(first,second);
    // A terrain blend has one stable specular lobe. The same authored alpha
    // still selects its local diffuse response without destabilizing history.
    auto finish=first.roughness>=second.roughness?first:second;
    if(finish.roughness==0.f)finish=root;
    detail[3]=std::bit_cast<std::uint32_t>(finish.roughness);
    if(finish.roughness==0.f) {
        // Legacy cube gain encodes tint/strength, not a physical mirror finish.
        if(material.source_surface_material!=SourceSurfaceMaterial::Water)
            response[0]=std::max(response[0],.60f);
        return;
    }
    response[0]=finish.roughness;
    // Scale both ends together so the authored angular balance and all tint
    // and mask information survive, including completely unlit reflection.
    float scale=1.f;
    if(response[1]>finish.max_facing)scale=finish.max_facing/response[1];
    if(response[2]>finish.max_glancing)scale=std::min(scale,finish.max_glancing/response[2]);
    response[1]*=scale;
    response[2]*=scale;
}
}
