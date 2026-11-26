#pragma once
#include "halo2/path_tracing/material_residency.hpp"
#include "halo2/path_tracing/surface_material.hpp"

namespace {
void check_npc(bool condition,const char* description) {
    if(!condition)throw FormatError(description);
}

void check_npc_finish(const Material& material,SourceSurfaceMaterial expected,bool nearby_reflection=false) {
    if(material.source_surface_material!=expected)
        throw FormatError("NPC material did not retain its assigned finish: "+material.name);
    auto legacy=material;legacy.source_surface_material=SourceSurfaceMaterial::Unknown;
    std::array<Vertex,3> triangle{};
    triangle[1].position={1,0,0};triangle[2].position={0,1,0};
    const auto source=pt::build_scene(triangle,std::span(&legacy,1));
    const auto finished=pt::build_scene(triangle,std::span(&material,1));
    const auto& before=source.materials.front();
    const auto& after=finished.materials.front();
    const auto profile=pt::surface_material_response(expected);
    if(after.response[0]!=profile.roughness ||
       after.detail[2]!=pt::pack_surface_material_response(profile,profile) ||
       std::bit_cast<float>(after.detail[3])!=profile.roughness)
        throw FormatError("NPC finish did not reach the ray material: "+material.name);
    auto texture=after.texture;
    if(nearby_reflection)texture[3]&=~(pt::nearby_scene_reflection_flag|pt::visor_scene_reflection_flag);
    if(source.pixels!=finished.pixels || before.texture!=texture || before.base!=after.base ||
       before.transform!=after.transform || before.emission!=after.emission ||
       before.glancing_tint!=after.glancing_tint || before.color_guide!=after.color_guide ||
       !reflection_finish_matches(before,after,profile))
        throw FormatError("NPC finish lost authored textures, colors, reflection balance, or coverage: "+material.name);
}

nlohmann::json check_heretic_elite_skin(const Cache& cache) {
    constexpr std::string_view name="objects\\characters\\elite\\shaders\\head";
    const auto tag=std::ranges::find_if(cache.tags(),[](const auto& item) {
        return item.group=="shad" && item.address && item.name==name;
    });
    check_npc(tag!=cache.tags().end(),"The Heretic lost the Elite head shader");
    auto rows=nlohmann::json::array();
    for(const bool dynamic:{true,false}) {
        auto skin=cache.load_material(tag->datum);skin.selected_shader_lod=0;
        const bool prepared=dynamic?prepare_dynamic_material(cache,skin,{},true,0.f):
            prepare_default_object_material(cache,skin,0.f);
        check_npc(prepared && skin.source_surface_material==SourceSurfaceMaterial::Skin,
            "Elite head did not prepare as skin");
        const auto surface=pt::qualified_surface(skin);
        const auto* pass=pt::reflection_pass(skin);
        check_npc(surface && surface->family==pt::SurfaceFamily::TextureFourChangeColor &&
            pass && pass->forward_vertex==4 && pass->forward_blend==8,"Elite head lost its qualified albedo or reflection pass");
        auto legacy=skin;legacy.source_surface_material=SourceSurfaceMaterial::Unknown;
        std::array<Vertex,3> triangle{};
        triangle[1].position={1,0,0};triangle[2].position={0,1,0};
        const auto prepare_scene=[&](const Material& material) {
            const auto roots=std::span(&material,1);
            auto scene=pt::build_scene({},roots);
            scene.material_environment_maps=pt::find_environment_maps(roots);
            pt::refresh_materials_for_geometry(scene,roots,triangle);
            return scene;
        };
        const auto old=prepare_scene(legacy);
        auto scene=prepare_scene(skin);
        const auto before=old.materials.front(),after=scene.materials.front();
        const auto profile=pt::surface_material_response(SourceSurfaceMaterial::Skin);
        check_npc(before.response[1]==.75f && before.response[2]==.75f && before.response[0]>=.60f,
            "Elite head control lost the source carapace reflection");
        check_npc(std::abs(after.response[1]-.055f)<1.e-6f && after.response[1]==after.response[2] &&
            after.response[0]==profile.roughness &&
            std::bit_cast<float>(after.detail[3])==profile.roughness &&
            after.detail[2]==pt::pack_surface_material_response(profile,profile),"Elite skin retained an armor-like lobe");
        check_npc(scene.pixels==old.pixels && after.detail[1]!=0 && after.detail[1]==before.detail[1] &&
            after.texture==before.texture && after.base==before.base && after.glancing_tint==before.glancing_tint &&
            after.transform==before.transform && after.color_guide==before.color_guide,
            "Elite skin lost its authored textures, reflection mask, tints or lighting inputs");

        WorldMaterialUniform primary{},environment{};
        primary.program=skin.forward_program.words;primary.forward_transforms=skin.forward_transforms;
        environment.program=pass->forward_program.words;
        environment.diffuse=pass->forward_diffuse;environment.diffuse[3]=.25f;
        environment.specular=pass->forward_specular;environment.specular[3]=.95f;
        const std::array owners{pt::MaterialAliasInput{{1,0,0},&primary,&environment,pass}};
        const auto aliases=pt::refresh_material_aliases(scene,std::span(&skin,1),owners);
        check_npc(aliases.size()==1,"Elite skin lost its live material alias");
        const auto& owner=scene.materials.at(aliases.front().material);
        check_npc(std::abs(owner.response[1]-.25f/.95f*.10f)<1.e-6f &&
            std::abs(owner.response[2]-.10f)<1.e-6f && owner.response[0]==after.response[0] &&
            owner.detail[3]==after.detail[3],
            "Live Elite skin constants restored the armor-like lobe");
        rows.push_back({{"name",name},{"dynamic",dynamic},{"source_facing",before.response[1]},
            {"source_glancing",before.response[2]},{"facing",after.response[1]},{"glancing",after.response[2]},
            {"roughness",after.response[0]}});
    }
    return rows;
}

nlohmann::json check_npc_finishes(const Cache& cache) {
    using Finish=SourceSurfaceMaterial;
    constexpr std::array expected{
        std::pair{"objects\\characters\\elite\\shaders\\torso",Finish::CoatedMetal},
        std::pair{"objects\\characters\\grunt\\shaders\\grunt_backpack",Finish::Polymer},
        std::pair{"objects\\characters\\jackal\\shaders\\jackal_head",Finish::Skin},
        std::pair{"objects\\characters\\marine\\shaders\\eyes_blue",Finish::Lens},
        std::pair{"objects\\characters\\marine\\shaders\\head_smith",Finish::Skin},
        std::pair{"objects\\characters\\marine\\shaders\\marine_standard",Finish::Fabric},
        std::pair{"objects\\characters\\marine\\shaders\\helmet_standard",Finish::Metal}};
    nlohmann::json finishes=nlohmann::json::array();
    for(const auto& [name,finish]:expected) {
        const auto tag=std::ranges::find_if(cache.tags(),[&](const auto& item) {
            return item.group=="shad" && item.address && item.name==name;
        });
        check_npc(tag!=cache.tags().end(),"Missing actual Old Mombasa finish material");
        auto material=cache.load_material(tag->datum);material.selected_shader_lod=0;
        check_npc(prepare_dynamic_material(cache,material,{},true,0.f),"NPC finish source did not prepare");
        check_npc_finish(material,finish,finish==Finish::CoatedMetal);
        finishes.push_back({{"name",material.name},{"roughness",pt::surface_material_response(finish).roughness}});
    }
    return finishes;
}

float check_honor_guard_surface(const Material& material,unsigned expected_weight,std::mt19937& random) {
    const auto surface=pt::qualified_surface(material);
    const auto family=pt::SurfaceFamily::TextureHonorGuard;
    const auto& native=material.forward_program.words;
    check_npc(surface && surface->family==family && surface->stages==15 && native[11]>>24==expected_weight,
        "Honor guard lost its exact native darkening or texture bindings");
    for(unsigned word=0;word<native.size();++word) {
        auto changed=material;changed.forward_program.words[word]^=1u;
        const bool color=(word>=10 && word<=25) || word==43 || word==44;
        check_npc(bool(pt::qualified_surface(changed))==color,"Honor guard accepted altered combiner controls");
    }
    auto changed=material;changed.forward_raster.alpha_test=true;
    check_npc(!pt::qualified_surface(changed),"Honor guard accepted a different alpha policy");
    changed=material;changed.passes[changed.forward_pass_index].textures[3].external=1;
    check_npc(!pt::qualified_surface(changed),"Honor guard accepted an external detail binding");

    float error{};
    std::uniform_real_distribution<float> unit(0.f,1.f);
    for(unsigned sample=0;sample<8192;++sample) {
        auto words=native;
        for(unsigned word=10;word<=25;++word)words[word]=random();
        words[43]=random();words[44]=random();
        std::array<ShaderVector,4> textures;
        for(auto& texture:textures)for(auto& c:texture)c=unit(random);
        ShaderVector v0,v1;
        for(auto& c:v0)c=unit(random);
        for(auto& c:v1)c=unit(random);
        const auto reference=halo2_test::scalar_combiners(words,textures,v0,v1);
        const auto traced=pt::surface_color(family,pt::surface_constants(words,family),textures);
        for(unsigned c=0;c<4;++c)error=std::max(error,std::abs(reference[c]-traced[c]));
    }
    check_npc(error<2.e-6f,"Honor guard surface disagreed with the native RGBA combiner");

    std::array<Vertex,3> triangle{};
    triangle[1].position={1,0,0};triangle[2].position={0,1,0};
    const auto roots=std::span(&material,1);
    auto scene=pt::build_scene(triangle,roots);
    WorldMaterialUniform uniform{};
    uniform.program=native;
    uniform.forward_transforms=material.forward_transforms;
    uniform.program[11]=0x51aabbccu;
    const std::array owners{pt::MaterialAliasInput{{1,0,0},&uniform}};
    const auto aliases=pt::refresh_material_aliases(scene,roots,owners);
    check_npc(aliases.size()==1,"Honor guard lost its live material alias");
    const auto check_record=[&](const pt::GpuMaterial& target,std::uint32_t weight) {
        check_npc(target.texture[3]&pt::surface_flag,"Honor guard did not reach the surface atlas");
        const auto record=std::bit_cast<std::uint32_t>(target.base[3])-1;
        check_npc(scene.pixels.at(record+3)==weight && scene.pixels.at(record+7)==15,
            "Honor guard atlas lost its authored weight or a texture stage");
    };
    check_record(scene.materials.front(),expected_weight<<24);
    check_record(scene.materials.at(aliases.front().material),0x51000000u);
    return error;
}

nlohmann::json check_heretic_finishes(const Cache& cache) {
    using Finish=SourceSurfaceMaterial;
    struct FinishCase {
        std::string_view shader,bitmap;
        Finish finish;
        bool nearby_reflection=false;
        int honor_guard_weight=-1;
    };
    constexpr std::array cases{
        FinishCase{"objects\\characters\\prophet\\shaders\\regret_head",
            "objects\\bitmaps\\detail_maps\\skin_human",Finish::Skin},
        FinishCase{"objects\\characters\\prophet\\shaders\\truth_body",
            "objects\\bitmaps\\detail_maps\\fabric",Finish::Fabric},
        FinishCase{"objects\\characters\\prophet\\shaders\\mercy_eyes",
            "objects\\bitmaps\\reflection_maps\\glassy_sharp_reflections",Finish::Lens},
        FinishCase{"objects\\characters\\prophet\\shaders\\truth_helmet",
            "objects\\bitmaps\\reflection_maps\\mirror_surface",Finish::Polished,true},
        FinishCase{"objects\\characters\\elite\\shaders\\council_helmet",
            "objects\\bitmaps\\reflection_maps\\carapace_shell",Finish::CoatedMetal,true},
        FinishCase{"objects\\characters\\elite\\shaders\\elite_honor_armor",
            "objects\\characters\\elite\\elite_honor_guard\\bitmaps\\elite_honor_armor_detail",Finish::CoatedMetal,true,128},
        FinishCase{"objects\\characters\\elite\\shaders\\pike",
            "objects\\weapons\\melee\\pike\\bitmaps\\pike_illum",Finish::CoatedMetal,true,0},
        FinishCase{"objects\\characters\\brute\\shaders\\brute_honor_armor",
            "objects\\bitmaps\\reflection_maps\\carapace_shell",Finish::CoatedMetal,true,128},
        FinishCase{"objects\\characters\\brute\\shaders\\tartarus_shoulder_armor",
            "objects\\bitmaps\\detail_maps\\metal_dirty",Finish::Metal},
        FinishCase{"objects\\characters\\masterchief\\shaders\\masterchief",
            "objects\\bitmaps\\reflection_maps\\masterchief_armor",Finish::CoatedMetal},
        FinishCase{"objects\\characters\\masterchief\\shaders\\masterchief_visor",
            "objects\\bitmaps\\reflection_maps\\mirror_surface",Finish::Lens,true},
        FinishCase{"objects\\vehicles\\gravity_throne\\shaders\\throne_shiny",
            "objects\\vehicles\\gravity_throne\\bitmaps\\gravity_throne_illum",Finish::CoatedMetal,true},
        FinishCase{"objects\\vehicles\\gravity_throne\\shaders\\throne_dull",
            "objects\\bitmaps\\reflection_maps\\dark_gray",Finish::Metal}};
    nlohmann::json finishes=nlohmann::json::array();
    std::mt19937 random(77123);
    for(const auto& fixture:cases) {
        const auto tag=std::ranges::find_if(cache.tags(),[&](const auto& item) {
            return item.group=="shad" && item.address && item.name==fixture.shader;
        });
        if(tag==cache.tags().end())
            throw FormatError("The Heretic lost its authored finish material: "+std::string(fixture.shader));
        const auto authored=cache.load_material(tag->datum);
        const bool bitmap_bound=std::ranges::any_of(authored.bitmaps,[&](const auto& binding) {
            const auto* bitmap=cache.find_tag({binding.datum},tag_groups::bitmap);
            return bitmap && bitmap->name==fixture.bitmap;
        });
        if(!bitmap_bound)
            throw FormatError("The Heretic lost its authored finish bitmap: "+authored.name);
        float honor_guard_error{};
        for(const bool fixed:{false,true}) {
            auto material=authored;material.selected_shader_lod=0;
            const bool prepared=fixed?prepare_default_object_material(cache,material,0.f):
                prepare_dynamic_material(cache,material,{},true,0.f);
            if(!prepared)
                throw FormatError("The Heretic finish material did not prepare: "+authored.name);
            check_npc_finish(material,fixture.finish,fixture.nearby_reflection);
            const bool throne=fixture.shader=="objects\\vehicles\\gravity_throne\\shaders\\throne_shiny";
            const bool visor=fixture.shader=="objects\\characters\\masterchief\\shaders\\masterchief_visor";
            const bool combined=!fixed && (fixture.finish==Finish::Polished || visor);
            const auto response=pt::material_response(material);
            const auto* reflection=pt::reflection_pass(material);
            const bool combined_visor=visor && reflection && pstencil::supports_combined_visor_environment(*reflection);
            if(bool(response.texture[3]&pt::visor_scene_reflection_flag)!=combined_visor)
                throw FormatError("Master Chief's combined visor lost its separate scene reflection policy: "+authored.name);
            if(throne || combined) {
                const bool source=reflection && (throne?reflection->forward_vertex==4 &&
                    reflection->forward_blend==8:pstencil::supports_combined_environment_program(*reflection));
                if(!source || !(response.texture[3]&pt::nearby_scene_reflection_flag) ||
                   !pt::find_environment_maps(std::span(&material,1)).front())
                    throw FormatError("The Heretic lost its authored nearby reflection: "+authored.name);
            }
            if(fixture.honor_guard_weight>=0)
                honor_guard_error=std::max(honor_guard_error,
                    check_honor_guard_surface(material,unsigned(fixture.honor_guard_weight),random));
        }
        nlohmann::json finish{{"name",authored.name},{"bitmap",fixture.bitmap},
            {"roughness",pt::surface_material_response(fixture.finish).roughness}};
        if(fixture.honor_guard_weight>=0) {
            finish["honor_guard_scalar_cases"]=16384;
            finish["honor_guard_max_error"]=honor_guard_error;
        }
        finishes.push_back(std::move(finish));
    }
    return finishes;
}

nlohmann::json npc_source_fixture(const Cache& cache) {
    constexpr std::array names{
        "objects\\characters\\grunt\\shaders\\grunt_torso",
        "objects\\characters\\grunt\\shaders\\grunt_head",
        "objects\\characters\\jackal\\shaders\\jackal_torso",
        "objects\\characters\\jackal\\shaders\\jackal_head"};
    constexpr unsigned sample_count=4096;
    std::mt19937 random(0x74aaa553u);
    std::uniform_real_distribution<float> unit(0.f,1.f),coordinates(-3.f,3.f);
    nlohmann::json materials=nlohmann::json::array();
    for(unsigned index=0;index<names.size();++index) {
        const auto tag=std::ranges::find_if(cache.tags(),[&](const auto& item) {
            return item.group=="shad" && item.address && item.name==names[index];
        });
        check_npc(tag!=cache.tags().end(),"Missing actual Old Mombasa armor material");
        const bool masked_tint=index%2==0;
        for(const bool fixed:{false,true}) {
            auto material=cache.load_material(tag->datum);material.selected_shader_lod=0;
            check_npc(fixed?prepare_default_object_material(cache,material,0.f):
                prepare_dynamic_material(cache,material,{},true,0.f),"NPC source material did not prepare");
            const auto surface=pt::qualified_surface(material);
            const auto expected_family=masked_tint?pt::SurfaceFamily::DynamicOneChangeColor:
                (fixed?pt::SurfaceFamily::OpaqueDetail:pt::SurfaceFamily::DynamicDetail);
            check_npc(surface && surface->family==expected_family && surface->default_object_ambient==fixed &&
                pt::qualified_unlit_white_tint(material),"NPC source material lost its qualified surface or native tint");
            check_npc(pt::source_ambient_owner(material)==(fixed?nullptr:&material),
                "NPC selected the wrong live ambient source owner");
            const unsigned ambient_word=fixed?(masked_tint?15u:14u):(masked_tint?23u:15u);
            const auto color_before=material.forward_program.words[ambient_word];
            ObjectLightingState lighting{};lighting.ambient={.7f,.4f,.3f};
            apply_object_lighting(material,lighting);
            const auto& native=material.forward_program.words;
            check_npc(fixed?native[ambient_word]==color_before:
                (pt::source_ambient_color_word(material)==ambient_word && (native[ambient_word]&0xffffffu)==0xb2664du),
                "NPC ambient color did not follow its authored live or default binding");

            float error{},discarded_color{};
            for(unsigned sample=0;sample<sample_count;++sample) {
                auto program=native;
                for(unsigned word=10;word<=25;++word)program[word]=random();
                program[43]=random();program[44]=random();
                std::array<ShaderVector,4> textures;
                for(auto& texture:textures)for(auto& c:texture)c=unit(random);
                ShaderVector v0,v1;
                for(auto& c:v0)c=unit(random);
                for(auto& c:v1)c=unit(random);
                // Suppress direction and specular contributions to compare
                // the ambient/albedo transfer of the complete native body.
                if(fixed)for(unsigned word:{11u,19u})program[word]&=0xff000000u;
                else if(masked_tint)for(unsigned word:{12u,20u,43u})program[word]&=0xff000000u;
                else for(unsigned word:{12u,14u,20u})program[word]&=0xff000000u;
                const auto actual=halo2_test::scalar_combiners(program,textures,v0,v1);
                const auto color=pt::surface_detail::argb(program[ambient_word]);
                const auto albedo=pt::surface_color(surface->family,pt::surface_constants(program,surface->family),textures);
                const float bump=fixed?1.f:std::max(0.f,2.f*textures[2][2]-1.f);
                for(unsigned c=0;c<3;++c) {
                    const float expected=std::min(1.f,2.f*albedo[c]*color[c]*bump);
                    error=std::max(error,std::abs(expected-actual[c]));
                    discarded_color=std::max(discarded_color,std::abs(albedo[c]-textures[0][c]));
                }
            }
            check_npc(error<2.e-6f && discarded_color>.2f,
                "NPC material did not reproduce native ambient or distinguish the old base-texture fallback");
            auto rejected=material;
            rejected.forward_program.words[45]^=1u;
            check_npc(!pt::qualified_surface(rejected),"NPC qualification accepted altered source controls");
            rejected=material;
            rejected.forward_raster.alpha_test=true;
            check_npc(!pt::qualified_surface(rejected),"An alpha-tested NPC escaped opaque surface qualification");
            if(masked_tint) {
                rejected=material;rejected.environment_pass=std::make_shared<Material>();
                rejected.environment_pass->forward_blend=8;
                check_npc(!pt::qualified_surface(rejected),"Unmodeled destination alpha leaked out of NPC tint qualification");
            }

            const auto vertex=decode_vertex_program(material.vertex_program);
            float uv_error{};
            for(unsigned sample=0;sample<128;++sample) {
                VertexProgramState state;
                for(auto& a:state.attributes)for(auto& v:a)v=coordinates(random);
                for(auto& c:state.constants)for(auto& v:c)v=coordinates(random);
                state.constants[172]={1,1,0,0};
                const Vec2 raw{coordinates(random),coordinates(random)};
                state.attributes[1]={raw.x,raw.y,0,1};
                execute_vertex_program(vertex,state);
                const auto& base=state.constants[18];
                const Vec2 uv{raw.x*base[0]+base[2],raw.y*base[1]+base[3]};
                for(unsigned stage:{1u,2u}) {
                    const auto& transform=state.constants[18+stage];
                    const auto& actual=state.outputs[9+stage];
                    uv_error=std::max({uv_error,std::abs(actual[0]-(uv.x*transform[0]+transform[2])),
                        std::abs(actual[1]-(uv.y*transform[1]+transform[3]))});
                }
            }
            check_npc(uv_error<2.e-6f,"NPC tint or bump moved away from the source c18/c19/c20 calculation");

            std::array<Vertex,3> triangle{};
            triangle[1].position={1,0,0};triangle[2].position={0,1,0};
            const auto roots=std::span(&material,1);
            auto scene=pt::build_scene(triangle,roots);
            WorldMaterialUniform uniforms[2]{};
            for(auto& uniform:uniforms) {
                uniform.program=native;
                uniform.forward_transforms=material.forward_transforms;
            }
            uniforms[0].program[ambient_word]=0x11aa44u;
            uniforms[1].program[ambient_word]=0x441177u;
            if(masked_tint) {
                const auto tint_word=fixed?13u:14u;
                uniforms[0].program[tint_word]=0x553333u;uniforms[1].program[tint_word]=0x33bb99u;
            }
            const std::array owners{pt::MaterialAliasInput{{1,0,0},&uniforms[0],nullptr,nullptr,fixed?nullptr:&uniforms[0]},
                pt::MaterialAliasInput{{2,0,0},&uniforms[1],nullptr,nullptr,fixed?nullptr:&uniforms[1]}};
            const auto aliases=pt::refresh_material_aliases(scene,roots,owners);
            check_npc(aliases.size()==owners.size(),"NPC instance shading lost its owner alias");
            for(unsigned i=0;i<aliases.size();++i) {
                const auto& bound=scene.materials.at(aliases[i].material);
                check_npc(bound.color_guide[0]==(fixed?pt::source_default_ambient_kind:3u) &&
                    bound.color_guide[2]==(uniforms[i].program[ambient_word]&0xffffffu),
                    "NPC secondary rays lost their selected ambient register");
                if(masked_tint) {
                    const auto record=std::bit_cast<std::uint32_t>(bound.base[3])-1;
                    check_npc(scene.pixels.at(record+3)==uniforms[i].program[fixed?13u:14u],
                        "NPC resident tint did not follow its individual rendered owner");
                }
            }
            const auto draw=pt::source_ambient_uniform(uniforms[0],uniforms[0],ambient_word);
            const auto static_color=pt::surface_detail::argb(uniforms[0].program[ambient_word]);
            check_npc(draw.specular==ShaderVector{2.f*static_color[0],2.f*static_color[1],2.f*static_color[2],1.f} &&
                draw.transforms[0]==material.forward_transforms[0] && draw.transforms[1]==material.forward_transforms[2],
                "NPC primary descriptor did not retain the correct ambient color and bump UVs");
            materials.push_back({{"name",tag->name},{"default_lighting",fixed},{"scalar_cases",sample_count},
                {"maximum_error",error},{"maximum_uv_error",uv_error},{"ambient_word",ambient_word}});
        }
    }
    return {{"passed",true},{"materials",materials},{"finishes",check_npc_finishes(cache)}};
}
}
