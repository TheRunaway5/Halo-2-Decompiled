#pragma once

#include "halo2/object_lighting.hpp"
#include "halo2/path_tracing/scene.hpp"
#include "halo2/path_tracing/surface_material.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <cmath>

inline nlohmann::json heretic_ship_source_fixture(const halo2::Cache& cache) {
    using namespace halo2;
    namespace pt=halo2::path_tracing;
    using Finish=SourceSurfaceMaterial;
    const auto check_finish=[](const Material& material,Finish expected) {
        const auto response=pt::material_response(material);
        const auto profile=pt::surface_material_response(expected);
        auto neutral=material;
        neutral.source_surface_material=Finish::Unknown;
        const auto legacy=pt::material_response(neutral);
        if(material.source_surface_material!=expected || response.response[0]!=profile.roughness ||
            response.detail[2]!=pt::pack_surface_material_response(profile,profile) ||
            std::bit_cast<float>(response.detail[3])!=profile.roughness ||
            !reflection_finish_matches(legacy,response,profile) ||
            response.base!=legacy.base || response.glancing_tint!=legacy.glancing_tint ||
            response.emission!=legacy.emission || response.texture!=legacy.texture)
            throw FormatError("The Heretic ship finish lost its response or changed authored appearance: "+material.name);
        return response;
    };
    const auto load_opaque=[&](std::string_view name) {
        const auto* tag=cache.query_tag({.group=tag_groups::shader,.name=name,.require_address=true});
        if(!tag)throw FormatError("The Heretic lost its source ship finish: "+std::string(name));
        auto material=cache.load_material(tag->datum);
        if(!prepare_dynamic_material(cache,material,{},true,0.f) || !pt::traces_material(material))
            throw FormatError("The Heretic ship finish lost its opaque dynamic pass: "+material.name);
        return material;
    };
    struct ShipSurface {
        std::string_view shader, cube;
        Finish finish;
        unsigned stage;
        float facing,glancing;
    };
    constexpr std::array fixtures{
        ShipSurface{"scenarios\\objects\\covenant\\military\\capital_ship\\shaders\\capital_ship_a",
            "diffuse_metal",Finish::CoatedMetal,3,.4f,.8f},
        ShipSurface{"scenarios\\objects\\covenant\\military\\capital_ship\\shaders\\capital_ship_b",
            "diffuse_metal",Finish::CoatedMetal,3,.4f,.8f},
        ShipSurface{"scenarios\\objects\\covenant\\military\\battle_cruiser\\shaders\\battle_cruiser_a",
            "diffuse_metal",Finish::CoatedMetal,1,1.f,1.f},
        ShipSurface{"scenarios\\objects\\covenant\\military\\battle_cruiser\\shaders\\battle_cruiser_b",
            "diffuse_metal",Finish::CoatedMetal,1,1.f,1.f},
        ShipSurface{"scenarios\\objects\\covenant\\military\\battle_cruiser\\shaders\\battle_cruiser_c",
            "diffuse_metal",Finish::CoatedMetal,1,1.f,1.f},
        ShipSurface{"objects\\cinematics\\human\\inamberclad\\shaders\\inamberclad_aft",
            "prt_diffuse_specular",Finish::Metal,1,178.f/255.f,102.f/255.f},
        ShipSurface{"objects\\cinematics\\human\\inamberclad\\shaders\\inamberclad_details",
            "prt_diffuse_specular",Finish::Metal,1,178.f/255.f,102.f/255.f},
        ShipSurface{"objects\\cinematics\\human\\inamberclad\\shaders\\inamberclad_fore",
            "prt_diffuse_specular",Finish::Metal,1,178.f/255.f,102.f/255.f},
        ShipSurface{"objects\\cinematics\\human\\pillarofautumn\\shaders\\poa_class",
            "prt_diffuse_specular",Finish::Metal,1,178.f/255.f,102.f/255.f},
        ShipSurface{"objects\\vehicles\\longsword\\shaders\\longsword_hull",
            "diffuse_metal",Finish::Metal,3,.5f,.85f},
        ShipSurface{"objects\\vehicles\\longsword\\shaders\\longsword_glass",
            "diffuse_metal",Finish::Lens,1,51.f/255.f,102.f/255.f}};
    auto rows=nlohmann::json::array();
    for(const auto& fixture:fixtures) {
        const auto material=load_opaque(fixture.shader);
        const auto* owner=pt::reflection_pass(material);
        const auto surface=pt::qualified_surface(material);
        const auto expected_family=fixture.stage==1?pt::SurfaceFamily::TextureDetail:pt::SurfaceFamily::DynamicDetail;
        if(!surface || surface->family!=expected_family || !owner ||
            owner->forward_vertex!=(fixture.stage==1?6u:4u) || owner->forward_pass_index>=owner->passes.size())
            throw FormatError("The Heretic ship lost its qualified reflection pass: "+material.name);
        const auto& pass=owner->passes[owner->forward_pass_index];
        const auto binding=std::ranges::find_if(pass.textures,[&](const auto& value) {
            return value.stage==fixture.stage && !value.external && value.parameter!=255;
        });
        if(binding==pass.textures.end())throw FormatError("The Heretic ship lost its authored cube binding: "+material.name);
        const auto bitmap_remap=std::ranges::find_if(pass.bitmap_remaps,[&](const auto& value) {
            return (value.destination&255)==binding->parameter;
        });
        const auto* bitmap=bitmap_remap!=pass.bitmap_remaps.end() && bitmap_remap->source<owner->bitmaps.size()?
            cache.find_tag({owner->bitmaps[bitmap_remap->source].datum},tag_groups::bitmap):nullptr;
        const auto maps=pt::find_environment_maps(std::span(&material,1));
        const auto& frames=owner->animated_textures[fixture.stage];
        const auto* cube=frames.empty()? (fixture.stage==1?&owner->detail:&owner->auxiliary):
            &frames.at(owner->texture_frames[fixture.stage]);
        if(!bitmap || std::string_view(bitmap->name).substr(bitmap->name.find_last_of('\\')+1)!=fixture.cube ||
            maps[0]!=cube || cube->layers!=6 || cube->width!=64 || cube->mips.size()!=30)
            throw FormatError("The Heretic ship selected the wrong authored reflection cube: "+material.name);
        const auto response=check_finish(material,fixture.finish);
        auto original=material;
        original.source_surface_material=Finish::Unknown;
        const auto authored=pt::material_response(original);
        if(std::abs(authored.response[1]-fixture.facing)>1.e-6f || std::abs(authored.response[2]-fixture.glancing)>1.e-6f)
            throw FormatError("The Heretic ship lost its authored facing or glancing strength: "+material.name);
        rows.push_back({{"name",material.name},{"cube",bitmap->name},{"stage",fixture.stage},
            {"roughness",response.response[0]},{"diffuse_retention",response.detail[2]},
            {"authored_facing",fixture.facing},{"authored_glancing",fixture.glancing},
            {"facing",response.response[1]},{"glancing",response.response[2]}});
    }
    struct UnreflectedSurface {std::string_view shader, bitmap;Finish finish;};
    for(const auto& fixture:{
        UnreflectedSurface{"objects\\cinematics\\human\\pillarofautumn\\shaders\\poa_class_engine",
            "objects\\cinematics\\human\\pillarofautumn\\bitmaps\\poa_class_engines",Finish::Metal},
        UnreflectedSurface{"scenarios\\shaders\\solo\\spacestation\\exterior_gun_sheath",
            "scenarios\\bitmaps\\solo\\spacestation\\ss_exterior_gun_sheath",Finish::Metal},
        UnreflectedSurface{"scenarios\\shaders\\solo\\spacestation\\exterior_gun_bar",
            "scenarios\\bitmaps\\solo\\spacestation\\ss_exterior_gun_bar",Finish::Metal},
        UnreflectedSurface{"scenarios\\shaders\\solo\\spacestation\\panel_mech_tile",
            "scenarios\\bitmaps\\solo\\spacestation\\panel_mech_tile",Finish::Metal},
        UnreflectedSurface{"scenarios\\shaders\\solo\\spacestation\\panel_floor_rubber",
            "scenarios\\bitmaps\\solo\\spacestation\\panel_floor_rubber",Finish::Rubber}}) {
        const auto material=load_opaque(fixture.shader);
        const auto response=check_finish(material,fixture.finish);
        if(!std::ranges::any_of(material.bitmaps,[&](const auto& binding) {
            const auto* bitmap=cache.find_tag({binding.datum},tag_groups::bitmap);
            return bitmap && bitmap->name==fixture.bitmap;
        }))throw FormatError("The Heretic ship finish lost its authored surface bitmap: "+material.name);
        if(pt::reflection_pass(material) || pt::find_environment_maps(std::span(&material,1))[0])
            throw FormatError("The Heretic ship finish added an unauthored reflection cube: "+material.name);
        rows.push_back({{"name",material.name},{"bitmap",fixture.bitmap},{"roughness",response.response[0]},
            {"diffuse_retention",response.detail[2]}});
    }
    for(const auto name:{"scenarios\\objects\\covenant\\military\\capital_ship\\shaders\\capital_ship_c",
             "scenarios\\objects\\covenant\\military\\capital_ship\\shaders\\antigrav_effects",
             "objects\\cinematics\\human\\inamberclad\\shaders\\inamberclad_numbers",
             "objects\\cinematics\\human\\pillarofautumn\\shaders\\poa_engine_exhaust",
             "objects\\vehicles\\longsword\\shaders\\longsword_afterburners",
             "objects\\cinematics\\matte_paintings\\shaders\\high_charity_exterior"}) {
        const auto* tag=cache.query_tag({.group=tag_groups::shader,.name=name,.require_address=true});
        if(!tag)throw FormatError("The Heretic lost a source ship overlay or matte: "+std::string(name));
        auto material=cache.load_material(tag->datum);
        if(material.source_surface_material!=Finish::Unknown ||
            !prepare_dynamic_material(cache,material,{},true,0.f) || pt::material_response(material).detail[3])
            throw FormatError("The Heretic ship finish included an authored overlay, emitter or matte: "+material.name);
    }
    return rows;
}
