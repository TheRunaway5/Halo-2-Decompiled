#pragma once
#include "halo2/path_tracing/chamber_reflection.hpp"
#include "halo2/path_tracing/color_guide.hpp"
#include "halo2/path_tracing/scene.hpp"
#include "halo2/path_tracing/source_lights.hpp"
#include "halo2/path_tracing/surface_material.hpp"
#include "halo2/world_material.hpp"
#include "../scalar_combiners.hpp"
#include <bit>
#include <cmath>
#include <nlohmann/json.hpp>

inline nlohmann::json chamber_reflection_source_fixture(const halo2::Cache& cache) {
    using namespace halo2;
    namespace pt=halo2::path_tracing;
    using Json=nlohmann::json;
    const auto atmosphere=cache.load_bsp_atmosphere(0);
    // Camera position captured from the authored council shot at tick 1229.
    const Vec3 eye{std::bit_cast<float>(3276866035u),std::bit_cast<float>(3272548598u),
        std::bit_cast<float>(1104100589u)};
    if(atmosphere.cluster_at(eye)!=45)
        throw FormatError("The Heretic council camera left its authored cluster");
    Json interior_clusters=Json::array(),outdoor_clusters=Json::array();
    for(int cluster:{4,9,10,45}) {
        const auto& source=atmosphere.clusters.at(cluster);
        if(source.visible_sky!=15 || pt::source_bsp_lighting_sky(atmosphere,cluster))
            throw FormatError("Council chamber lost its visible sky or acquired outdoor lighting");
        interior_clusters.push_back(cluster);
    }
    for(const auto& [cluster,sky]:{std::pair{1,15u},std::pair{27,10u}}) {
        if(pt::source_bsp_lighting_sky(atmosphere,cluster)!=sky)
            throw FormatError("Exterior cluster lost its authored lighting sky");
        outdoor_clusters.push_back({{"cluster",cluster},{"lighting_sky",sky}});
    }
    const auto mesh=cache.load_bsp(0);
    unsigned visible_beam_materials{};
    for(const auto& material:mesh.materials) {
        if(material.name!="scenarios\\shaders\\solo\\highcharity\\long_volume")continue;
        if(!material.forward_pass || material.forward_blend!=3 || material.diffuse.pixels.empty() ||
           pt::ray_material(material))
            throw FormatError("The authored Council light beam lost its raster pass or entered ray lighting");
        ++visible_beam_materials;
    }
    if(visible_beam_materials==0)throw FormatError("Missing the original Council light beam material");
    pt::Scene scene;
    pt::refresh_color_guides(scene,mesh.lightmaps);
    for(const auto& material:mesh.materials)scene.materials.push_back(pt::material_response(material));
    unsigned guide_samples{};
    Vec3 guide_sum{};
    for(std::size_t i=0;i<mesh.vertices.size();i+=3) {
        const auto& first=mesh.vertices[i];
        if(first.bsp_cluster!=45)continue;
        pt::GpuTriangle triangle;
        triangle.metadata[0]=first.material;
        triangle.color_guide=pt::color_guide_triangle(first,mesh.vertices[i+1],mesh.vertices[i+2],
            scene.materials.at(first.material));
        if(const auto color=pt::sample_color_guide(scene,triangle,{1.f/3,1.f/3,1.f/3})) {
            guide_sum={guide_sum.x+color->x,guide_sum.y+color->y,guide_sum.z+color->z};
            ++guide_samples;
        }
    }
    if(guide_samples<1000 || std::min({guide_sum.x,guide_sum.y,guide_sum.z})<.1f*guide_samples)
        throw FormatError("The Heretic council camera cluster lost its authored baked irradiance");
    const auto guide_mean=std::array{guide_sum.x/guide_samples,guide_sum.y/guide_samples,guide_sum.z/guide_samples};
    struct Source {
        std::string_view leaf;
        unsigned facing,glancing;
    };
    constexpr std::array sources{
        Source{"cov_dark_blue",64,51},Source{"panel_f_blue",128,255},
        Source{"strip_a_grey",128,89},Source{"strip_a_blue_lit",128,89}};
    Json materials=Json::array();
    for(const auto& source:sources) {
        const auto name=std::string(pt::chamber_reflection_detail::owner)+std::string(source.leaf);
        const auto tag=std::ranges::find_if(cache.tags(),[&](const auto& item) {
            return item.group=="shad" && item.address && item.name==name;
        });
        if(tag==cache.tags().end())throw FormatError("Missing Heretic chamber source shader: "+name);
        const auto root=cache.load_material(tag->datum);
        const auto& bitmap=root.bitmaps.at(4);
        const auto& bitmap_tag=cache.tag_at_index(bitmap.datum&65535u);
        if(bitmap_tag.datum!=bitmap.datum || bitmap_tag.name!="objects\\bitmaps\\reflection_maps\\carapace_shell")
            throw FormatError("The Heretic chamber's authored reflection bitmap changed: "+name);
        const auto cube=cache.load_bitmap(bitmap.datum,bitmap.index);
        Json cases=Json::array();
        for(unsigned lod:{0u,1u})for(bool per_vertex:{false,true}) {
            auto material=root;
            material.selected_shader_lod=lod;
            if(!prepare_world_material(cache,material,per_vertex))
                throw FormatError("Could not prepare Heretic chamber shader: "+name);
            const auto* pass=pt::chamber_reflection_pass(material);
            if(!pass || (pass!=&material)!=(lod==0) || pass->detail.layers!=6 ||
               pass->detail.pixels.fingerprint()!=cube.pixels.fingerprint())
                throw FormatError("Heretic chamber reflection did not select the authored cube: "+name);

            const auto& words=pass->forward_program.words;
            const auto parameters=pt::chamber_reflection_parameters(words);
            if(parameters.facing!=ShaderVector{148/255.f,148/255.f,148/255.f,float(source.facing)/255.f} ||
               parameters.glancing!=ShaderVector{107/255.f,107/255.f,107/255.f,float(source.glancing)/255.f} ||
               parameters.baked_weight!=192/255.f)
                throw FormatError("Heretic chamber authored reflection response changed: "+name);
            const auto gpu=pt::material_response(material);
            const auto maps=pt::find_environment_maps(std::span(&material,1));
            const auto finish=source.leaf=="strip_a_grey"?SourceSurfaceMaterial::Metal:SourceSurfaceMaterial::CoatedMetal;
            const auto profile=pt::surface_material_response(finish);
            auto authored=gpu;
            authored.response[1]=parameters.facing[3];authored.response[2]=parameters.glancing[3];
            if(pt::reflection_pass(material)!=pass || maps.front()!=&pass->detail ||
               !(gpu.texture[3]&pt::chamber_baked_reflection_flag) || !(gpu.texture[3]&2048u) ||
               material.source_surface_material!=finish || gpu.response[0]!=profile.roughness ||
               !reflection_finish_matches(authored,gpu,profile) ||
               gpu.glancing_tint[3]!=parameters.baked_weight || gpu.color_guide[0]!=(per_vertex?2u:1u))
                throw FormatError("Path Tracing did not carry the chamber's authored cube or bake guide: "+name);

            float maximum_error{},maximum_signal{};
            for(float facing:{0.f,.4f,1.f})for(float alpha:{0.f,.23f,1.f})for(float light:{0.f,.17f,.64f}) {
                // An outward normal and direction with known N.V isolate
                // the cube through both actual source combiner programs.
                std::array<ShaderVector,4> stages{{{0,0,0,alpha},{.17f,.28f,.39f,0},
                    {.5f,.5f,1,1},per_vertex?ShaderVector{.5f,.5f,1,1}:ShaderVector{light,light,light,1}}};
                const ShaderVector v0{.5f,.5f,.5f+.5f*facing,1};
                const ShaderVector v1{light,light,light,.5f};
                const auto reflected=halo2_test::scalar_combiners(words,stages,v0,v1);
                const auto sample=stages[1];stages[1]={};
                const auto black=halo2_test::scalar_combiners(words,stages,v0,v1);
                // N and incident vector are chosen so the source direction
                // correction is zero. The real pass still consumes bake / v1
                // and the texture accumulation alpha independently.
                const float mask=std::min(2.f*alpha*std::min(3.f*light*parameters.baked_weight,1.f),1.f);
                const float strength=std::lerp(parameters.glancing[3],parameters.facing[3],facing);
                for(unsigned c=0;c<3;++c) {
                    const float tint=std::lerp(parameters.glancing[c],parameters.facing[c],facing);
                    const float signal=reflected[c]-black[c];
                    const float expected=std::clamp(black[c]+sample[c]*tint*strength*mask,0.f,1.f)-black[c];
                    maximum_signal=std::max(maximum_signal,signal);
                    maximum_error=std::max(maximum_error,std::abs(expected-signal));
                }
            }
            if(maximum_error>2.e-6f || maximum_signal<.03f)
                throw FormatError("Heretic chamber cube scalar mismatch: "+name+" "+
                    std::to_string(maximum_error)+" / "+std::to_string(maximum_signal));

            auto changed=material;
            if(changed.environment_pass)changed.environment_pass=std::make_shared<Material>(*changed.environment_pass);
            auto& changed_pass=changed.environment_pass?*changed.environment_pass:changed;
            changed_pass.forward_program.words[54]^=1u<<5;
            if(pt::chamber_reflection_pass(changed))
                throw FormatError("Heretic chamber qualifier accepted a different cube sample mode");
            changed_pass.forward_program.words[54]=words[54];
            changed_pass.passes[changed_pass.forward_pass_index].implementation=2;
            if(pt::chamber_reflection_pass(changed))
                throw FormatError("Heretic chamber qualifier accepted the dynamic implementation");
            changed_pass.passes[changed_pass.forward_pass_index].implementation=per_vertex?1u:0u;
            changed_pass.passes[changed_pass.forward_pass_index].textures[1].stage=0;
            if(pt::chamber_reflection_pass(changed))
                throw FormatError("Heretic chamber qualifier accepted a duplicate texture stage");
            cases.push_back({{"lod",lod},{"implementation",per_vertex?1:0},{"bake_guide",gpu.color_guide[0]},
                {"maximum_error",maximum_error},{"maximum_cube_signal",maximum_signal}});
        }
        materials.push_back({{"name",name},{"cases",cases}});
    }
    return {{"passed",true},{"materials",materials},{"council_camera_cluster",45},
        {"council_baked_guide_samples",guide_samples},{"council_mean_baked_rgb",guide_mean},
        {"raster_only_beam_materials",visible_beam_materials},
        {"interior_clusters",interior_clusters},
        {"outdoor_clusters",outdoor_clusters}};
}
