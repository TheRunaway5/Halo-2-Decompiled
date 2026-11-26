#include "halo2/path_tracing/source_ambient.hpp"
#include "halo2/path_tracing/material_alias.hpp"
#include "halo2/path_tracing/surface_material.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/pstencil/combined_environment.hpp"
#include "halo2/pstencil/environment.hpp"
#include "halo2/vertex_program.hpp"
#include "halo2/world_material.hpp"
#include "../scalar_combiners.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <fstream>
#include <iostream>
#include <random>

using namespace halo2;
namespace pt=halo2::path_tracing;
namespace {
bool reflection_finish_matches(const pt::GpuMaterial& source,const pt::GpuMaterial& tuned,
    pt::SurfaceMaterialResponse finish) {
    const auto& a=source.response;
    const auto& b=tuned.response;
    constexpr float tolerance=1.e-6f;
    const bool within=b[1]<=std::min(a[1],finish.max_facing)+tolerance &&
        b[2]<=std::min(a[2],finish.max_glancing)+tolerance;
    const bool balanced=std::abs(a[1]*b[2]-a[2]*b[1])<tolerance;
    const bool preserved=std::abs(b[1]-std::min(a[1],finish.max_facing))<tolerance ||
        std::abs(b[2]-std::min(a[2],finish.max_glancing))<tolerance;
    return within && balanced && preserved && a[3]==b[3];
}
}
#include "dynamic_npc_source_fixture.hpp"
#include "heretic_ship_source_fixture.hpp"
#include "chamber_reflection_source_fixture.hpp"
namespace {
nlohmann::json check_carapace_reflections(const Cache& cache) {
    constexpr std::array names{
        "objects\\characters\\elite\\shaders\\torso",
        "objects\\characters\\elite\\shaders\\helmet",
        "objects\\characters\\elite\\shaders\\arms",
        "objects\\characters\\elite\\shaders\\legs"};
    nlohmann::json rows=nlohmann::json::array();
    for(const auto name:names) {
        const auto source=std::ranges::find_if(cache.tags(),[&](const auto& tag) {
            return tag.group=="shad" && tag.address && tag.name==name;
        });
        if(source==cache.tags().end())throw FormatError("The Heretic lost its source carapace shader");
        auto material=cache.load_material(source->datum);
        material.selected_shader_lod=0;
        if(!prepare_dynamic_material(cache,material,{},true,0.f))
            throw FormatError("The Heretic source carapace shader did not prepare: "+material.name);
        const auto reflection=pt::material_response(material);
        if(!(reflection.texture[3]&pt::nearby_scene_reflection_flag) ||
           !pt::find_environment_maps(std::span(&material,1)).front())
            throw FormatError("The Heretic source carapace did not carry its masked reflection: "+material.name);
        material.source_surface_material=SourceSurfaceMaterial::Unknown;
        const auto legacy=pt::material_response(material);
        if((legacy.texture[3]&pt::nearby_scene_reflection_flag) ||
           !reflection_finish_matches(legacy,reflection,pt::surface_material_response(SourceSurfaceMaterial::CoatedMetal)) ||
           legacy.base!=reflection.base || legacy.glancing_tint!=reflection.glancing_tint)
            throw FormatError("The Heretic local reflection lost the paint finish's response or authored tint");
        rows.push_back(name);
    }
    return rows;
}

nlohmann::json check_default_lighting(const Cache& cache,std::uint32_t datum,std::mt19937& random) {
    auto material=cache.load_material(datum);material.selected_shader_lod=0;
    if(!prepare_default_object_material(cache,material,0.f))
        throw FormatError("Actual default object source did not prepare");
    const auto surface=pt::qualified_surface(material);
    if(!surface || surface->family!=pt::SurfaceFamily::OpaqueDetail || !surface->default_object_ambient ||
       !pt::qualified_unlit_white_tint(material) || pt::source_ambient_owner(material))
        throw FormatError("Armory default object pass lost its qualified albedo or ambient separation: "+material.name);

    constexpr unsigned samples=8192;
    std::uniform_real_distribution<float> channel(0,1);
    float ambient_error{},alpha_dependency{};
    for(unsigned sample=0;sample<samples;++sample) {
        auto words=material.forward_program.words;
        for(unsigned n=10;n<=25;++n)words[n]=random();
        words[43]=random();words[44]=random();
        std::array<ShaderVector,4> textures;ShaderVector v0,v1;
        for(auto& texture:textures)for(auto& c:texture)c=channel(random);
        for(auto& c:v0)c=channel(random);
        for(auto& c:v1)c=channel(random);
        const auto full=halo2_test::scalar_combiners(words,textures,v0,v1);
        // Zero the two direction-dependent diffuse coefficients. The real
        // combiner retains only its fixed ambient term even with arbitrary T2/T3.
        words[11]&=0xff000000u;words[19]&=0xff000000u;
        const auto ambient_only=halo2_test::scalar_combiners(words,textures,v0,v1);
        const auto ambient=pt::surface_detail::argb(pt::source_default_ambient_rgb(words));
        const auto diffuse=pt::surface_color(surface->family,{},textures);
        for(unsigned c=0;c<3;++c)
            ambient_error=std::max(ambient_error,std::abs(ambient_only[c]-std::min(2.f*ambient[c]*diffuse[c],1.f)));
        alpha_dependency=std::max(alpha_dependency,std::abs(full[3]-ambient_only[3]));
    }
    bool controls=true;
    for(unsigned word=0;word<material.forward_program.words.size();++word) {
        auto changed=material;changed.forward_program.words[word]^=1u;
        const bool color=(word>=10 && word<=25) || word==43 || word==44;
        controls&=bool(pt::qualified_surface(changed))==color;
    }
    auto changed=material;changed.forward_raster.alpha_test=true;
    controls&=!pt::qualified_surface(changed);
    changed=material;changed.environment_pass=std::make_shared<Material>();
    changed.environment_pass->forward_blend=8;
    controls&=!pt::qualified_surface(changed);

    std::array<Vertex,3> triangle{};
    triangle[1].position={1,0,0};triangle[2].position={0,1,0};
    const auto materials=std::span(&material,1);
    auto scene=pt::build_scene(triangle,materials);
    const std::array<std::uint32_t,4> expected{
        pt::source_default_ambient_kind,0u,material.forward_program.words[14]&0xffffffu,0u};
    WorldMaterialUniform a{},b{};
    a.program=b.program=material.forward_program.words;
    a.forward_transforms=b.forward_transforms=material.forward_transforms;
    a.program[14]=0xffa02048u;b.program[14]=0x0011952bu;
    const std::array owners{pt::MaterialAliasInput{{1,0,0},&a},pt::MaterialAliasInput{{2,0,0},&b}};
    const auto aliases=pt::refresh_material_aliases(scene,materials,owners);
    const bool owner_colors=aliases.size()==2 && scene.materials[0].color_guide==expected &&
        scene.materials[aliases[0].material].color_guide==std::array<std::uint32_t,4>{expected[0],0u,0xa02048u,0u} &&
        scene.materials[aliases[1].material].color_guide==std::array<std::uint32_t,4>{expected[0],0u,0x11952bu,0u} &&
        scene.material_alias_images.empty();
    return {{"passed",ambient_error<2.e-6f && alpha_dependency>.2f && controls && owner_colors},
        {"scalar_cases",samples},{"maximum_ambient_error",ambient_error},{"maximum_alpha_dependency",alpha_dependency},
        {"control_mutations_passed",controls},{"owner_colors_passed",owner_colors}};
}

nlohmann::json check_vehicle_reflection(const Cache& cache,std::mt19937& random) {
    constexpr std::string_view name="objects\\vehicles\\ghost\\shaders\\ghost_shell";
    const auto tag=std::ranges::find_if(cache.tags(),[&](const auto& entry) {
        return entry.group=="shad" && entry.address && entry.name==name;
    });
    if(tag==cache.tags().end())throw FormatError("Prepared vehicle map lost the Ghost shell");
    auto material=cache.load_material(tag->datum);material.selected_shader_lod=0;
    if(!prepare_dynamic_material(cache,material,{},true,0.f))
        throw FormatError("Ghost shell dynamic pass did not prepare");
    const auto& program=material.forward_program;
    const auto& images=material.animated_textures[1];
    const auto& cube=images.empty()?material.detail:images.at(material.texture_frames[1]);
    const auto maps=pt::find_environment_maps(std::span(&material,1));
    const auto response=pt::material_response(material);
    bool admitted=pt::reflection_pass(material)==&material && maps[0]==&cube &&
        !pstencil::supports_combined_visor_environment(material) &&
        response.response[1]==float(program.words[17]>>24)/255.f &&
        response.response[2]==float(program.words[25]>>24)/255.f &&
        !(response.texture[3]&pt::environment_mask_flag);
    auto changed=material;changed.forward_program.words[38]^=1u;
    admitted&=!pt::reflection_pass(changed);

    constexpr unsigned samples=4096;
    std::uniform_real_distribution<float> channel(0,1);
    unsigned visible_cube{};
    for(unsigned sample=0;sample<samples;++sample) {
        auto words=program.words;
        for(unsigned n=10;n<=25;++n)words[n]=random();
        std::array<ShaderVector,4> texels;ShaderVector v0,v1;
        for(auto& texel:texels)for(auto& value:texel)value=channel(random);
        for(auto& value:v0)value=channel(random);
        for(auto& value:v1)value=channel(random);
        const auto full=halo2_test::scalar_combiners(words,texels,v0,v1);
        const auto unreflected=halo2_test::scalar_combiners(words,
            {texels[0],{},texels[2],texels[3]},v0,v1);
        words[17]&=0x00ffffffu;
        words[25]&=0x00ffffffu;
        const auto suppressed=halo2_test::scalar_combiners(words,texels,v0,v1);
        admitted&=suppressed==unreflected && suppressed[3]==full[3];
        visible_cube+=full!=suppressed;
    }
    return {{"passed",admitted && visible_cube>0},{"name",material.name},{"scalar_cases",samples},
        {"visible_cube_cases",visible_cube},{"source_cube_face_size",cube.width}};
}

nlohmann::json check_vehicle_finishes(const Cache& cache) {
    using Class=SourceSurfaceMaterial;
    struct FinishCase {
        std::string_view shader,bitmap;
        Class finish;
        float roughness;
        std::uint32_t retention;
    };
    constexpr std::array fixtures{
        FinishCase{"objects\\vehicles\\ghost\\shaders\\ghost_shell","objects\\bitmaps\\reflection_maps\\carapace_shell",
            Class::CoatedMetal,.51f,0xfffafffau},
        FinishCase{"objects\\vehicles\\ghost\\shaders\\ghost_metal","objects\\bitmaps\\detail_maps\\metal_dirty",
            Class::Metal,.53f,0},
        FinishCase{"objects\\vehicles\\warthog\\shaders\\chassis","objects\\vehicles\\warthog\\bitmaps\\chassis",
            Class::CoatedMetal,.51f,0xfffafffau},
        FinishCase{"objects\\vehicles\\warthog\\shaders\\metal","objects\\bitmaps\\detail_maps\\metal_dirty",
            Class::Metal,.53f,0},
        FinishCase{"objects\\vehicles\\warthog\\shaders\\tires_hubless","objects\\vehicles\\warthog\\bitmaps\\hub",
            Class::Metal,.53f,0},
        FinishCase{"objects\\vehicles\\warthog\\shaders\\tires","objects\\bitmaps\\detail_maps\\rubber",
            Class::Rubber,.90f,0xd6d6d6d6u},
        FinishCase{"objects\\vehicles\\warthog\\shaders\\vinyl","objects\\bitmaps\\detail_maps\\vinyl",
            Class::Polymer,.76f,0xeae0eae0u}};
    for(const auto name:{"objects\\vehicles\\wraith\\shaders\\chassis", "objects\\vehicles\\ghost\\shaders\\ghost_dash",
        "objects\\vehicles\\ghost\\shaders\\ghost_torn", "objects\\vehicles\\warthog\\shaders\\glass"})
        if(pt::model_surface_material(name)!=Class::Unknown)
            throw FormatError("Vehicle finish lookup included an unrelated shader: "+std::string(name));
    auto rows=nlohmann::json::array();
    for(const auto& fixture:fixtures) {
        const auto tag=std::ranges::find_if(cache.tags(),[&](const auto& entry) {
            return entry.group=="shad" && entry.address && entry.name==fixture.shader;
        });
        if(tag==cache.tags().end())throw FormatError("Prepared vehicle map lost finish shader: "+std::string(fixture.shader));
        const auto authored=cache.load_material(tag->datum);
        const bool bitmap_present=std::ranges::any_of(authored.bitmaps,[&](const auto& binding) {
            const auto* bitmap=cache.find_tag({binding.datum},tag_groups::bitmap);
            return bitmap && bitmap->name==fixture.bitmap;
        });
        if(!bitmap_present)throw FormatError("Prepared vehicle finish lost its authored bitmap: "+authored.name);
        for(bool dynamic:{true,false}) {
            auto material=authored;material.selected_shader_lod=0;
            const bool prepared=dynamic?prepare_dynamic_material(cache,material,{},true,0.f):
                prepare_default_object_material(cache,material,0.f);
            if(!prepared)throw FormatError("Prepared vehicle finish lost its source pass: "+authored.name);
            const auto response=pt::material_response(material);
            auto neutral=material;
            neutral.name="unmapped";
            neutral.source_surface_material=Class::Unknown;
            const auto original=pt::material_response(neutral);
            if(material.source_surface_material!=fixture.finish || response.response[0]!=fixture.roughness ||
                response.detail[2]!=fixture.retention || std::bit_cast<float>(response.detail[3])!=fixture.roughness)
                throw FormatError("Vehicle finish lost its authored roughness or diffuse response: "+material.name);
            if(response.base!=original.base || response.glancing_tint!=original.glancing_tint ||
                response.emission!=original.emission || response.texture!=original.texture ||
                !reflection_finish_matches(original,response,pt::surface_material_response(fixture.finish)))
                throw FormatError("Vehicle finish lost authored reflection balance, tint, or coverage: "+material.name);
            rows.push_back({{"name",material.name},{"bitmap",fixture.bitmap},{"dynamic",dynamic},
                {"roughness",response.response[0]},{"diffuse_retention",response.detail[2]},
                {"facing",response.response[1]},{"glancing",response.response[2]}});
        }
    }
    return rows;
}

nlohmann::json check_weapon_finishes(const Cache& cache) {
    using Class=SourceSurfaceMaterial;
    struct FinishCase {std::string_view shader;Class finish;};
    constexpr std::array fixtures{
        FinishCase{"objects\\weapons\\rifle\\smg\\shaders\\smg_painted_metal",Class::CoatedMetal},
        FinishCase{"objects\\weapons\\rifle\\smg\\shaders\\smg_bare_metal",Class::Metal},
        FinishCase{"objects\\weapons\\rifle\\smg\\shaders\\smg_rubber_grip",Class::Rubber},
        FinishCase{"objects\\weapons\\rifle\\smg\\shaders\\smg_lod",Class::CoatedMetal},
        FinishCase{"objects\\weapons\\rifle\\battle_rifle\\shaders\\battle_rifle_metal",Class::Metal},
        FinishCase{"objects\\weapons\\rifle\\battle_rifle\\shaders\\battle_rifle_plastic",Class::Polymer},
        FinishCase{"objects\\weapons\\rifle\\battle_rifle\\shaders\\battle_rifle_scope",Class::Rubber},
        FinishCase{"objects\\weapons\\rifle\\battle_rifle\\shaders\\battle_rifle_lens",Class::Lens},
        FinishCase{"objects\\weapons\\rifle\\battle_rifle\\shaders\\battle_rifle_lod",Class::CoatedMetal}};
    auto rows=nlohmann::json::array();
    for(const auto& fixture:fixtures) {
        const auto tag=std::ranges::find_if(cache.tags(),[&](const auto& entry) {
            return entry.group=="shad" && entry.address && entry.name==fixture.shader;
        });
        if(tag==cache.tags().end())throw FormatError("Armory fixture lost weapon finish: "+std::string(fixture.shader));
        auto material=cache.load_material(tag->datum);material.selected_shader_lod=0;
        if(!prepare_dynamic_material(cache,material,{},true,0.f))
            throw FormatError("Armory weapon finish lost its source pass: "+material.name);
        const auto response=pt::material_response(material);
        const auto profile=pt::surface_material_response(fixture.finish);
        const auto finish=material.source_surface_material;
        material.source_surface_material=Class::Unknown;
        const auto original=pt::material_response(material);
        if(finish!=fixture.finish || response.response[0]!=profile.roughness ||
           response.detail[2]!=pt::pack_surface_material_response(profile,profile) ||
           std::bit_cast<float>(response.detail[3])!=profile.roughness ||
           response.base!=original.base || response.glancing_tint!=original.glancing_tint ||
           response.emission!=original.emission || response.texture!=original.texture ||
           !reflection_finish_matches(original,response,profile))
            throw FormatError("Armory weapon finish changed its source color, reflection or coverage: "+material.name);
        rows.push_back({{"name",material.name},{"roughness",response.response[0]},
            {"diffuse_retention",response.detail[2]}});
    }
    return rows;
}

nlohmann::json check_weapon_reflections(const Cache& cache,std::mt19937& random) {
    using Json=nlohmann::json;
    Json rows=Json::array();
    std::uniform_real_distribution<float> channel(0,1);
    for(const std::string_view name:{
        "objects\\weapons\\rifle\\smg\\shaders\\smg_bare_metal",
        "objects\\weapons\\rifle\\battle_rifle\\shaders\\battle_rifle_lens"}) {
        const auto tag=std::ranges::find_if(cache.tags(),[&](const auto& candidate) {
            return candidate.group=="shad" && candidate.address && candidate.name==name;
        });
        if(tag==cache.tags().end())throw FormatError("Armory reflection fixture lost shader "+std::string(name));
        auto material=cache.load_material(tag->datum);material.selected_shader_lod=0;
        if(!prepare_dynamic_material(cache,material,{},true,0.f) || !material.environment_pass ||
           pt::reflection_pass(material)!=material.environment_pass.get())
            throw FormatError("Armory reflection fixture lost its combined lighting child");
        const auto& child=*material.environment_pass;
        const auto& images=child.animated_textures[1];
        const auto& cube=images.empty()?child.detail:images.at(child.texture_frames[1]);
        const auto maps=pt::find_environment_maps(std::span(&material,1));
        if(maps[0]!=&cube || cube.layers!=6)
            throw FormatError("Armory reflection fixture selected the wrong cube stage");
        const auto response=pt::material_response(material);
        const auto& current=child.forward_program.words;
        const auto finish=pt::surface_material_response(material.source_surface_material);
        auto authored=response;
        authored.response[1]=float(current[17]>>24)/255.f;
        authored.response[2]=float(current[25]>>24)/255.f;
        if(!reflection_finish_matches(authored,response,finish))
            throw FormatError("Armory reflection fixture lost its authored facing and glancing strengths");

        auto reference=child;
        reference.forward_program.words[5]=0x1d1d0000u;
        reference.forward_program.words[31]=0x000000d0u;
        if(!pstencil::supports_combined_environment_strength(reference))
            throw FormatError("Armory reflection changed controls beyond the independent highlight operand");
        float error{},cube_signal{};
        constexpr unsigned samples=8192;
        for(unsigned n=0;n<samples;++n) {
            auto actual=current;
            for(unsigned word=10;word<=25;++word)actual[word]=random();
            auto standard=actual;
            standard[5]=reference.forward_program.words[5];standard[31]=reference.forward_program.words[31];
            // Isolate the v0*T1 cube operand from the separate v1*r13.a highlight.
            actual[41]&=0xffff0000u;standard[41]&=0xffff0000u;
            std::array<ShaderVector,4> textures;ShaderVector v0,v1;
            for(auto& texture:textures)for(auto& c:texture)c=channel(random);
            for(auto& c:v0)c=channel(random);
            for(auto& c:v1)c=channel(random);
            const auto reflected=halo2_test::scalar_combiners(actual,textures,v0,v1);
            const auto expected=halo2_test::scalar_combiners(standard,textures,v0,v1);
            textures[1][0]=textures[1][1]=textures[1][2]=0.f;
            const auto black_cube=halo2_test::scalar_combiners(actual,textures,v0,v1);
            for(unsigned c=0;c<3;++c) {
                error=std::max(error,std::abs(reflected[c]-expected[c]));
                cube_signal=std::max(cube_signal,std::abs(reflected[c]-black_cube[c]));
            }
        }
        auto changed=material;
        changed.environment_pass=std::make_shared<Material>(child);
        changed.environment_pass->forward_program.words[5]^=1u;
        rows.push_back({{"name",name},{"passed",error<2.e-6f && cube_signal>.2f && !pt::reflection_pass(changed)},
            {"cube_stage",1},{"scalar_cases",samples},{"maximum_cube_error",error},{"maximum_cube_signal",cube_signal}});
    }
    return rows;
}
}
int main(int argc,char** argv)try {
    const std::string_view fixture=argc==4?argv[3]:"";
    const bool armory=fixture=="--armory",weapons=fixture=="--weapons",vehicles=fixture=="--vehicles";
    const bool npcs=fixture=="--npcs";
    if(argc!=3 && !armory && !weapons && !vehicles && !npcs)throw FormatError("Supply prepared map cache and small report path");
    if(!std::filesystem::is_regular_file(argv[1]))return 77;
    Cache cache(argv[1]);
    if(npcs) {
        const auto result=npc_source_fixture(cache);
        std::ofstream report(argv[2]);
        if(!(report<<result.dump(2)<<'\n'))throw FormatError("Could not write NPC source shading report");
        std::cout<<result.dump()<<'\n';
        return 0;
    }
    std::mt19937 random(0x24d37a1u);
    std::uniform_real_distribution<float> channel(0,1),coordinate(-3,3);
    constexpr std::array<std::string_view,5> armory_materials{
        "\\equipment_cart","\\head_smith","\\marine_standard","\\odst_armor","\\military_equipment"};
    constexpr std::array<std::string_view,2> weapon_materials{
        "objects\\weapons\\rifle\\smg\\shaders\\smg_lod",
        "objects\\weapons\\rifle\\battle_rifle\\shaders\\battle_rifle_lod"};
    constexpr std::array<std::string_view,2> vehicle_materials{
        "objects\\vehicles\\warthog\\shaders\\chassis",
        "objects\\vehicles\\ghost\\shaders\\ghost_metal"};
    nlohmann::json rows=nlohmann::json::array();bool admitted=true,controls=true;
    unsigned cases{},uv_cases{};float rgb_error{},alpha_error{},ambient_error{},uv_error{},fallback_error{};
    for(const auto& tag:cache.tags()) {
        if(tag.group!="shad")continue;
        const bool selected=armory?std::ranges::any_of(armory_materials,
            [&](std::string_view name){return tag.name.ends_with(name);}):
            weapons?std::ranges::find(weapon_materials,tag.name)!=weapon_materials.end():
            vehicles?std::ranges::find(vehicle_materials,tag.name)!=vehicle_materials.end():
            tag.name.ends_with("\\capital_ship_a") || tag.name.ends_with("\\capital_ship_b");
        if(!selected)continue;
        auto material=cache.load_material(tag.datum);material.selected_shader_lod=0;
        if(!prepare_dynamic_material(cache,material,{},true,0.f))throw FormatError("Actual dynamic source did not prepare");
        const auto qualified=pt::qualified_surface(material);
        admitted&=qualified && qualified->family==pt::SurfaceFamily::DynamicDetail;
        admitted&=pt::qualified_unlit_white_tint(material);
        admitted&=pt::source_ambient_owner(material)==&material;
        const auto program=decode_vertex_program(material.vertex_program);
        for(unsigned sample=0;sample<8192;++sample) {
            auto words=material.forward_program.words;
            for(unsigned n=10;n<=25;++n)words[n]=random();words[43]=random();words[44]=random();
            std::array<ShaderVector,4> textures;ShaderVector v0,v1;
            for(auto& texture:textures)for(auto& c:texture)c=channel(random);
            for(auto& c:v0)c=channel(random);for(auto& c:v1)c=channel(random);
            const auto full=halo2_test::scalar_combiners(words,textures,v0,v1);
            // Native stage1 writes T0.a*T1.a; stage6 writes twice C1.a
            // (word24) times that product to v0.a; final G selects v0.a.
            const float alpha=std::clamp(2.f*float(words[24]>>24)/255.f*textures[0][3]*textures[1][3],0.f,1.f);
            alpha_error=std::max(alpha_error,std::abs(full[3]-alpha));
            // Isolate diffuse while preserving every native combiner stage:
            // signed bump.T3 dot=.5, diffuse C0=white/C1=black; suppress
            // only the independent specular and ambient RGB constants.
            auto isolated=words;isolated[12]|=0xffffffu;
            for(unsigned word:{14u,15u,20u})isolated[word]&=0xff000000u;
            auto t=textures;t[2]={.5f,.5f,1.f,textures[2][3]};t[3]={.5f,.5f,.75f,textures[3][3]};
            const auto unlit=halo2_test::scalar_combiners(isolated,t,v0,v1);
            // Leave only the native ambient contribution active, with live
            // C1 RGB and unrestricted bump Z.
            for(unsigned word:{12u,14u,20u})words[word]&=0xff000000u;
            const auto ambient=halo2_test::scalar_combiners(words,textures,v0,v1);
            const auto irradiance=pt::source_ambient_irradiance(words);
            const float bump_z=std::max(2.f*textures[2][2]-1.f,0.f);
            ShaderVector expected{};
            for(unsigned c=0;c<3;++c) {
                expected[c]=std::clamp(2.f*textures[0][c]*textures[1][c],0.f,1.f);
                rgb_error=std::max(rgb_error,std::abs(unlit[c]-expected[c]));
                ambient_error=std::max(ambient_error,
                    std::abs(ambient[c]-std::clamp(expected[c]*irradiance[c]*bump_z,0.f,1.f)));
                fallback_error=std::max(fallback_error,std::abs(textures[0][c]-expected[c]));
            }
            expected[3]=alpha;
            if(qualified) {
                auto changed=material;changed.forward_program.words=words;
                const auto fixed=pt::surface_color(qualified->family,pt::surface_constants(changed,qualified->family),textures);
                for(unsigned c=0;c<3;++c)rgb_error=std::max(rgb_error,std::abs(fixed[c]-expected[c]));
                alpha_error=std::max(alpha_error,std::abs(fixed[3]-expected[3]));
            }
            ++cases;
        }
        for(unsigned sample=0;sample<256;++sample) {
            VertexProgramState state;
            for(auto& attr:state.attributes)for(auto& c:attr)c=coordinate(random);
            for(auto& constant:state.constants)for(auto& c:constant)c=coordinate(random);
            state.constants[172]={1,1,0,0};
            const Vec2 raw{coordinate(random),coordinate(random)};state.attributes[1]={raw.x,raw.y,0,1};
            auto changed=material;
            for(unsigned n=0;n<3;++n)for(unsigned c=0;c<4;++c)
                changed.forward_transforms[n][c]=state.constants[18+n][c];
            execute_vertex_program(program,state);
            const auto& base=changed.forward_transforms[0];const Vec2 uv{raw.x*base[0]+base[2],raw.y*base[1]+base[3]};
            for(unsigned stage=0;stage<2;++stage) {
                const auto transform=stage?changed.forward_transforms[stage]:ShaderVector{1,1,0,0};
                const Vec2 expected{uv.x*transform[0]+transform[2],uv.y*transform[1]+transform[3]};
                const auto& actual=state.outputs[9+stage];
                uv_error=std::max({uv_error,std::abs(expected.x-actual[0]),std::abs(expected.y-actual[1]),std::abs(actual[3]-1.f)});
                if(const auto model=pt::qualified_unlit_uv(changed)) {
                    const auto fixed=pt::unlit_uv(*model,raw)[stage];
                    uv_error=std::max({uv_error,std::abs(fixed.x-actual[0]),std::abs(fixed.y-actual[1])});
                }
            }
            ++uv_cases;
        }
        if(qualified)for(unsigned word=0;word<60;++word) {
            auto changed=material;changed.forward_program.words[word]^=1u;
            const bool color=(word>=10 && word<=25)||word==43||word==44;
            controls&=bool(pt::qualified_surface(changed))==color;
        }
        auto changed=material;changed.vertex_program.back()^=std::byte{1};controls&=!pt::qualified_surface(changed);
        if(armory || weapons || vehicles) {
            changed=material;changed.forward_raster.alpha_test=true;controls&=!pt::qualified_surface(changed);
        }
        rows.push_back({{"name",tag.name},{"admitted",bool(qualified)},{"white_tint",pt::qualified_unlit_white_tint(material)},
            {"source_ambient",pt::source_ambient_owner(material)==&material},
            {"vertex_shader",material.passes.at(material.forward_pass_index).vertex_shader},
            {"source_program",material.forward_program.words},{"source_reflection_mask_color",material.forward_program.words[24]}});
        if(armory || weapons || vehicles) {
            auto fallback=check_default_lighting(cache,tag.datum,random);
            admitted&=fallback["passed"].get<bool>();
            rows.back()["default_lighting"]=std::move(fallback);
        }
    }
    auto reflections=weapons?check_weapon_reflections(cache,random):nlohmann::json::array();
    if(vehicles)reflections.push_back(check_vehicle_reflection(cache,random));
    for(const auto& reflection:reflections)admitted&=reflection["passed"].get<bool>();
    const auto material_count=armory?armory_materials.size():2;
    const bool passed=rows.size()==material_count && admitted && controls && rgb_error<2.e-6f &&
        alpha_error<2.e-6f && ambient_error<2.e-6f && uv_error==0 && fallback_error>.2f;
    nlohmann::json report{{"passed",passed},{"materials",rows},{"scalar_cases",cases},{"vertex_cases",uv_cases},
        {"maximum_rgb_error",rgb_error},{"maximum_alpha_error",alpha_error},{"maximum_ambient_error",ambient_error},
        {"maximum_uv_error",uv_error},
        {"maximum_T0_fallback_error",fallback_error},{"control_mutations_passed",controls}};
    if(weapons || vehicles)report["reflections"]=reflections;
    if(vehicles)report["finishes"]=check_vehicle_finishes(cache);
    if(weapons)report["weapon_finishes"]=check_weapon_finishes(cache);
    if(fixture.empty() && cache.name()=="00a_introduction") {
        report["ship_finishes"]=heretic_ship_source_fixture(cache);
        report["carapace_reflections"]=check_carapace_reflections(cache);
        report["elite_skin"]=check_heretic_elite_skin(cache);
        report["heretic_finishes"]=check_heretic_finishes(cache);
        report["chamber_reflections"]=chamber_reflection_source_fixture(cache);
    }
    std::ofstream(argv[2])<<report.dump(2)<<'\n';std::cout<<report.dump()<<'\n';return passed?0:1;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
