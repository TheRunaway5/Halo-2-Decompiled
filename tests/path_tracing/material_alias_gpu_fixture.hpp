#pragma once
#include "halo2/path_tracing/material_alias.hpp"
#include "halo2/path_tracing/material_surface.hpp"

int material_alias_gpu(const std::filesystem::path& shaders) {
    Fixture gpu;gpu.initialize(shaders);gpu.renderer->set_quality(1,1);
    Material root;root.forward_pass=true;root.forward_vertex=3;root.forward_pass_index=0;
    root.passes.resize(1);root.passes[0].name="shaders\\shader_passes\\texture_mapping\\default_texture_two_change_color_zwrite";
    root.passes[0].vertex_shader="rasterizer\\vertex_shaders_xbox\\stencil_texture";
    root.vertex_program=vertex_bytes(pt::unlit_uv_detail::texture_words);root.passes[0].vertex_code=root.vertex_program;
    root.forward_program.words=pt::surface_detail::texture_controls(4);
    root.forward_program.words[10]=0xff0000;root.forward_program.words[18]=0xff00;
    root.forward_transforms[0]=root.forward_transforms[1]={1,1,0,0};root.forward_transforms[2]={1,1,1,1};
    root.diffuse=solid_texture({255,255,255,255});root.detail=solid_texture({128,128,128,255});
    root.auxiliary=solid_texture({255,0,0,255});
    gpu.primary_color({1,1,1,1},0);gpu.primary_color({128.f/255,128.f/255,128.f/255,1},1);gpu.primary_color({1,0,0,1},3);
    std::array<WorldMaterialUniform,2> uniforms;
    for(auto& u:uniforms){u.program=root.forward_program.words;u.forward_transforms=root.forward_transforms;}
    uniforms[0].program[11]=0x00ff0000;uniforms[1].program[11]=0x0000ff00;
    std::array inputs{pt::MaterialAliasInput{{0xe2000001u,0,~0u},&uniforms[0]},
        pt::MaterialAliasInput{{0xe3000001u,0,~0u},&uniforms[1]}};
    std::vector<Vertex> primary;quad(primary,-1,-1,0,1,.5f,{0,0,-1},0);quad(primary,0,-1,1,1,.5f,{0,0,-1},0);
    auto scene=pt::build_scene(primary,std::span(&root,1));
    auto aliases=pt::refresh_material_aliases(scene,std::span(&root,1),inputs);
    PrimaryAmbientOwners descriptors(gpu);
    const std::array sets{descriptors.sets[0],descriptors.sets[0],descriptors.sets[1],descriptors.sets[1]};
    const auto submit=[&](bool filtered) {
        for(unsigned owner=0;owner<2;++owner) {
            std::memcpy(descriptors.buffers[owner].mapped,&uniforms[owner],sizeof(WorldMaterialUniform));
            for(unsigned v=owner*6;v<(owner+1)*6;++v)primary[v].material=aliases[owner].material;
        }
        pt::update_geometry(scene,primary);
        return gpu.draw(scene,primary,{.25f,.25f,.25f},Camera{},~0u,false,filtered,nullptr,true,false,true,nullptr,sets);
    };
    for(bool filtered:{false,true}) {
        const auto frame=submit(filtered);
        require(mean(frame,0,8,24)>60 && mean(frame,1,8,24)<1 && mean(frame,1,40,56)>60 && mean(frame,0,40,56)<1,
            "Shared-root owners lost distinct current source colors in PT primary roles");
    }
    const auto stable=aliases;
    uniforms[0].program[11]=0x000000ff;
    pt::clear_material_aliases(scene,1);pt::refresh_materials(scene,std::span(&root,1));
    std::swap(inputs[0],inputs[1]);aliases=pt::refresh_material_aliases(scene,std::span(&root,1),inputs);
    require(aliases[0].material==stable[0].material && aliases[1].material==stable[1].material,
        "Owner submission reorder changed PT material IDs");
    const auto changed=submit(true);
    require(mean(changed,2,8,24)>60 && mean(changed,0,8,24)<1 && mean(changed,1,40,56)>60,
        "One owner's next-frame source color stayed stale or changed its neighbor");

    // Two real secondary intersections use the same root, independent native
    // ambient children and shared bump image. The perfect front mirrors isolate
    // the returned owner response from primary diffuse/sky/lighting estimates.
    root.passes[0].name="shaders\\shader_passes\\texture_mapping\\default_texture_zwrite";
    root.forward_program.words=pt::surface_detail::texture_controls(1);root.forward_program.words[43]=0xffffff;
    root.environment_pass=std::make_shared<Material>();auto& child=*root.environment_pass;
    child.forward_pass=true;child.forward_vertex=6;child.accumulation_texture=true;child.forward_pass_index=0;
    child.passes.resize(1);child.passes[0].name="shaders\\shader_passes\\lightmap\\old_lightmap_tangentspace";
    child.passes[0].vertex_shader="rasterizer\\vertex_shaders_xbox\\lightmap_dynamic";
    child.vertex_program=vertex_bytes(pt::source_ambient_detail::dynamic_vertex);child.passes[0].vertex_code=child.vertex_program;
    child.forward_program.words=pt::source_ambient_detail::dynamic_accumulation;child.bump=solid_texture({128,128,255,255});
    require(pt::source_ambient_owner(root)==&child,"Secondary owner fixture lost exact source ambient qualification");
    std::array<WorldMaterialUniform,2> ambient;
    for(unsigned n=0;n<2;++n) {
        uniforms[n].program=root.forward_program.words;uniforms[n].forward_transforms=root.forward_transforms;
        ambient[n].program=child.forward_program.words;ambient[n].forward_transforms[0]=ambient[n].forward_transforms[2]={1,1,0,0};
    }
    ambient[0].program[15]=0x007f0000;ambient[1].program[15]=0x00007f00;
    auto mirror=white_material();std::array materials{root,mirror};
    std::vector<Vertex> geometry;quad(geometry,-16384,-16384,0,16384,-3,{0,0,1},0);
    quad(geometry,0,-16384,16384,16384,-3,{0,0,1},0);
    auto secondary=pt::build_scene(geometry,materials);
    inputs={pt::MaterialAliasInput{{0xe2000001u,0,~0u},&uniforms[0]},pt::MaterialAliasInput{{0xe3000001u,0,~0u},&uniforms[1]}};
    for(unsigned n=0;n<2;++n)inputs[n].ambient=&ambient[n];
    auto wall_aliases=pt::refresh_material_aliases(secondary,materials,inputs);
    const auto image_count=secondary.material_alias_images.size();require(image_count==1,"Owners duplicated their shared source bump image");
    const std::array<Vec3,6> black{};
    auto reflect=[&] {
        for(unsigned n=0;n<12;++n)geometry[n].material=wall_aliases[n/6].material;
        pt::update_geometry(secondary,geometry);
        secondary.materials[1].response={0,1,1,0};secondary.materials[1].base={1,1,1,1};
        for(auto& v:primary)v.material=1;
        std::memset(gpu.uniform.mapped,0,sizeof(WorldMaterialUniform));gpu.primary_color({0,0,0,1});
        return gpu.draw(secondary,primary,{},Camera{},~0u,false,false,&black);
    };
    const auto reflected=reflect();
    require(mean(reflected,0,8,24)>245 && mean(reflected,1,8,24)<1 && mean(reflected,1,40,56)>245 && mean(reflected,0,40,56)<1,
        "Secondary rays lost separate native owner ambient/color state on shared material roots");
    ambient[0].program[15]=0x0000007f;
    pt::clear_material_aliases(secondary,materials.size());pt::refresh_materials(secondary,materials);
    std::swap(inputs[0],inputs[1]);wall_aliases=pt::refresh_material_aliases(secondary,materials,inputs);
    const auto reflected_changed=reflect();
    require(mean(reflected_changed,2,8,24)>245 && mean(reflected_changed,0,8,24)<1 && mean(reflected_changed,1,40,56)>245,
        "Reflected owner lighting stayed stale or changed the other shared-root owner");
    require(secondary.material_alias_images.size()==image_count,"Next frame duplicated shared bump images");
    require(gpu.validation_errors.load()==0,"Vulkan validation reported owner alias errors");
    std::cout<<"PASS shared-root source aliases: two primary owners, secondary ambient owners, reorder and selective next-frame change\n";
    return 0;
}
