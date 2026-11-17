#pragma once
#include "halo2/path_tracing/material_alias.hpp"

int material_alias_history_gpu(const std::filesystem::path& shaders,const std::filesystem::path& map) {
    if(!std::filesystem::is_regular_file(map))return 77;
    Cache cache(map);Material root;bool found=false;
    for(const auto& tag:cache.tags())if(tag.group=="shad" && tag.name.ends_with("\\capital_ship_a")) {
        root=cache.load_material(tag.datum);root.selected_shader_lod=0;
        require(prepare_dynamic_material(cache,root,{},true,0.f),"Alias history source material failed preparation");found=true;break;
    }
    require(found && pt::source_ambient_owner(root)==&root,"Alias history requires actual native capital ambient controls");
    root.diffuse=solid_texture({255,255,255,255});root.detail=solid_texture({128,128,128,255});
    root.bump=solid_texture({128,128,255,255});
    for(auto& bank:root.animated_textures)bank.clear();
    auto mirror=white_material();std::array materials{root,mirror};
    std::array<WorldMaterialUniform,2> primary_uniforms,ambient;
    for(unsigned n=0;n<2;++n) {
        primary_uniforms[n].program=root.forward_program.words;primary_uniforms[n].forward_transforms=root.forward_transforms;
        primary_uniforms[n].program[24]=0x80000000;
        ambient[n].program=root.forward_program.words;ambient[n].program[15]=0x007f7f7f;
        ambient[n].forward_transforms[0]=ambient[n].forward_transforms[2]={1,1,0,0};
    }
    auto red=solid_texture({255,0,0,255}),green=solid_texture({0,255,0,255}),blue=solid_texture({0,0,255,255});
    std::array inputs{pt::MaterialAliasInput{{0xe2000001u,0,~0u},&primary_uniforms[0]},
        pt::MaterialAliasInput{{0xe3000001u,0,~0u},&primary_uniforms[1]}};
    for(unsigned n=0;n<2;++n)inputs[n].ambient=&ambient[n];
    inputs[0].surface_images[0]=&red;inputs[1].surface_images[0]=&green;
    Fixture gpu;gpu.initialize(shaders,{64,64},true);gpu.fence_submissions=true;gpu.renderer->set_quality(1,1);
    gpu.primary_color({1,1,1,1});
    std::vector<Vertex> receiver;quad(receiver,-1,-1,1,1,.5f,{0,0,-1},1);
    std::vector<Vertex> walls;quad(walls,-4,-4,0,4,-3,{0,0,1},0);quad(walls,0,-4,4,4,-3,{0,0,1},0);
    auto scene=pt::build_scene(walls,materials);
    const auto refresh=[&](bool diffuse=false) {
        pt::clear_material_aliases(scene,materials.size());pt::refresh_materials(scene,materials);
        const auto aliases=pt::refresh_material_aliases(scene,materials,inputs);
        for(unsigned n=0;n<walls.size();++n)walls[n].material=aliases[n/6].material;
        pt::update_geometry(scene,walls);
        scene.materials[1].response={0,diffuse?0.f:1.f,diffuse?0.f:1.f,0};
        scene.materials[1].base=scene.materials[1].glancing_tint={1,1,1,1};
    };
    Camera camera;camera.eye={0,0,-10000,1};
    const auto draw=[&](bool capture=true) {
        return gpu.draw(scene,receiver,{},camera,~0u,false,true,nullptr,true,capture,capture);
    };
    const auto warm=[&] {for(unsigned n=0;n<15;++n)draw(false);return draw();};
    refresh();const auto before=warm();inputs[0].surface_images[0]=&blue;refresh();const auto changed=draw();
    std::cout<<"Alias image history: prior_red="<<mean(before,0,8,24)<<" changed_red="<<mean(changed,0,8,24)
        <<" changed_blue="<<mean(changed,2,8,24)<<" other_green="<<mean(changed,1,40,56)<<std::endl;
    require(mean(before,0,8,24)>245 && mean(before,1,40,56)>245,"Alias history controls lost distinct reflected owners");
    const bool image_current=mean(changed,0,8,24)<3 && mean(changed,2,8,24)>245 && mean(changed,1,40,56)>245;
    const auto before_ambient=warm();ambient[0].program[15]=0x003f3f3f;refresh();const auto changed_ambient=draw();
    const auto retained=gpu.renderer->temporal_stats();
    std::cout<<"Alias ambient history: before_blue="<<mean(before_ambient,2,8,24)<<" changed_blue="
        <<mean(changed_ambient,2,8,24)<<" diffuse_history="<<retained.banks[0].mean_count<<std::endl;
    const bool ambient_current=mean(changed_ambient,2,8,24)>123 && mean(changed_ambient,2,8,24)<129;
    // A real diffuse continuation must reject the same old secondary radiance.
    walls.clear();quad(walls,-16384,-16384,16384,16384,-3,{0,0,1},0);
    ambient[0].program[15]=0x007f7f7f;inputs[0].surface_images[0]=&red;refresh(true);const auto diffuse_before=warm();
    inputs[0].surface_images[0]=&blue;refresh(true);const auto diffuse_after=draw();
    std::cout<<"Alias diffuse history: before_red="<<mean(diffuse_before,0)<<" after_red="<<mean(diffuse_after,0)
        <<" after_blue="<<mean(diffuse_after,2)<<std::endl;
    require(image_current,"Reflected stationary owner retained its previous material image");
    require(ambient_current,"Reflected stationary owner retained its previous ambient lighting");
    require(retained.banks[0].mean_count>15.5,"Owner ambient animation globally reset unrelated lighting history");
    require(mean(diffuse_before,0)>240 && mean(diffuse_after,0)<3 && mean(diffuse_after,2)>240,
        "Diffuse bounce retained a stationary owner's previous material image");
    // Smooth normals can animate without moving the actual triangle plane.
    ambient[0].program[15]=0;inputs[0].surface_images[0]=&red;
    pt::Light normal_light;normal_light.direction=Vec3{0,0,1};normal_light.color={1,1,1};
    refresh();pt::update_geometry(scene,walls,std::span(&normal_light,1));const auto normal_before=warm();
    for(auto& vertex:walls)vertex.normal={1,0,0};
    refresh();pt::update_geometry(scene,walls,std::span(&normal_light,1));const auto normal_after=draw();
    std::cout<<"Alias stationary normal: before_red="<<mean(normal_before,0)<<" changed_red="<<mean(normal_after,0)<<std::endl;
    require(mean(normal_before,0)>200 && mean(normal_after,0)<3,
        "Stationary secondary normal retained its previous reflected lighting");
    // A changing owner outside the reflection lobe must not turn a converged
    // stochastic sky reflection back into a fresh noisy sample every frame.
    walls.clear();quad(walls,10,10,11,11,-3,{0,0,1},0);
    for(auto& vertex:receiver)vertex.normal={std::sin(.3926990817f),0,-std::cos(.3926990817f)};
    const std::array<Vec3,6> sky{{{0,1,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{1,0,0}}};
    gpu.environment_side=32;refresh();scene.materials[1].response={.25f,1,1,0};
    const auto sky_draw=[&](bool capture) {
        return gpu.draw(scene,receiver,{},camera,~0u,false,true,&sky,true,capture,capture);
    };
    for(unsigned n=0;n<31;++n)sky_draw(false);
    const auto converged=noise_metrics(sky_draw(true),{64,64});
    for(unsigned n=0;n<32;++n) {
        ambient[0].program[15]=n%2?0x007f0000:0x00007f00;refresh();scene.materials[1].response={.25f,1,1,0};
        if(n!=31)sky_draw(false);
    }
    const auto unrelated=noise_metrics(sky_draw(true),{64,64});
    std::cout<<"Alias offaxis convergence: reference_deviation="<<converged.deviation<<" changed_deviation="<<unrelated.deviation
        <<" reference_high_frequency="<<converged.high_frequency<<" changed_high_frequency="<<unrelated.high_frequency<<std::endl;
    require(converged.mean>30 && converged.mean<220 && converged.deviation>1,
        "Alias unrelated-reflection control lost its stochastic signal");
    require(unrelated.deviation<converged.deviation*1.5 && unrelated.high_frequency<converged.high_frequency*1.6,
        "Offaxis owner ambient animation discarded converged reflection history");
    // The same retention requirement applies to a genuine diffuse receiver
    // when the changed owner is entirely behind its outgoing hemisphere.
    walls.clear();quad(walls,-4,-4,4,4,3,{0,0,-1},0);
    for(auto& vertex:receiver)vertex.normal={0,0,-1};refresh(true);
    for(unsigned n=0;n<16;++n)gpu.draw(scene,receiver,{.25f,.25f,.25f},camera,~0u,false,true);
    ambient[0].program[15]=0x0000007f;refresh(true);
    const auto retained_diffuse=gpu.draw(scene,receiver,{.25f,.25f,.25f},camera,~0u,false,true,nullptr,true,true);
    require(mean(retained_diffuse,0)>62 && mean(retained_diffuse,0)<65 && gpu.renderer->temporal_stats().banks[0].mean_count>15.5,
        "Owner behind a diffuse hemisphere discarded unrelated lighting history");
    require(gpu.validation_errors.load()==0,"Alias material history reported Vulkan errors");
    return 0;
}
