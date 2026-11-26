#pragma once
#include "halo2/path_tracing/color_guide.hpp"
#include "halo2/path_tracing/material_alias.hpp"
#include "halo2/path_tracing/source_ambient.hpp"

namespace {
struct PrimaryAmbientOwners {
    Fixture& gpu;VkDescriptorPool pool{};std::array<VkDescriptorSet,2> sets{};std::array<Buffer,2> buffers;
    explicit PrimaryAmbientOwners(Fixture& owner):gpu(owner) {
        const VkDescriptorPoolSize sizes[]{{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,8},{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,2}};
        VkDescriptorPoolCreateInfo create{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};create.maxSets=2;
        create.poolSizeCount=2;create.pPoolSizes=sizes;check(vkCreateDescriptorPool(gpu.device,&create,nullptr,&pool),"create ambient owner pool");
        const std::array layouts{gpu.material_layout,gpu.material_layout};
        VkDescriptorSetAllocateInfo allocate{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};allocate.descriptorPool=pool;
        allocate.descriptorSetCount=2;allocate.pSetLayouts=layouts.data();
        check(vkAllocateDescriptorSets(gpu.device,&allocate,sets.data()),"allocate ambient owner descriptors");
        for(unsigned i=0;i<2;++i) {
            gpu.buffer(buffers[i],sizeof(WorldMaterialUniform),VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
            std::array<VkCopyDescriptorSet,4> copies{};
            for(unsigned stage=0;stage<4;++stage) {
                auto& copy=copies[stage];copy={VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET};copy.srcSet=gpu.material;
                copy.srcBinding=stage;copy.dstSet=sets[i];copy.dstBinding=stage;copy.descriptorCount=1;
            }
            VkDescriptorBufferInfo bank{buffers[i].handle,0,sizeof(WorldMaterialUniform)};
            VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};write.dstSet=sets[i];write.dstBinding=4;
            write.descriptorCount=1;write.descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;write.pBufferInfo=&bank;
            vkUpdateDescriptorSets(gpu.device,1,&write,copies.size(),copies.data());
        }
    }
    void set(unsigned owner,std::uint32_t sourceAmbient,
             ShaderVector base={1,1,0,0},ShaderVector bump={1,1,0,0}) {
        WorldMaterialUniform primary{},lighting{};lighting.program[15]=sourceAmbient;
        lighting.forward_transforms[0]=base;lighting.forward_transforms[2]=bump;
        const auto copy=pt::source_ambient_uniform(primary,lighting);
        require(primary.specular==ShaderVector{},"PT ambient conduit mutated its retained native primary uniform");
        std::memcpy(buffers[owner].mapped,&copy,sizeof(copy));
    }
    ~PrimaryAmbientOwners() {
        vkDestroyDescriptorPool(gpu.device,pool,nullptr);
        for(auto& buffer:buffers) {
            if(buffer.mapped)vkUnmapMemory(gpu.device,buffer.memory);
            if(buffer.handle)vkDestroyBuffer(gpu.device,buffer.handle,nullptr);
            if(buffer.memory)vkFreeMemory(gpu.device,buffer.memory,nullptr);
        }
    }
};
}

// Independent transport check for the authenticated source diffuse tag. Its
// complete native-control admission and 2*RGB transfer are checked by the
// source scalar fixtures; this test supplies analytic lightmap samples.
int source_primary_gpu(const std::filesystem::path& shaders) {
    Fixture gpu;gpu.initialize(shaders);gpu.renderer->set_quality(1,1);
    gpu.primary_color({.25f,.5f,.75f,1});
    std::vector<Vertex> primary;quad(primary,-1,-1,1,1,.5f,{0,0,-1},0);
    for(auto& vertex:primary){vertex.lightmap=1;vertex.lightmap_uv={.5f,.5f};}
    auto material=white_material();material.diffuse=solid_texture({64,128,191,255});
    const std::array<Vec3,6> black{};
    constexpr unsigned sampler=3u|(3u<<4)|(2u<<8);
    const auto make=[&](std::span<const pt::Light> lights={},bool qualified=true) {
        auto scene=pt::build_scene(primary,std::span(&material,1),lights);
        scene.materials[0].response={1,0,0,0};
        scene.materials[0].color_guide={1,sampler,0,qualified?2u:0u};
        const std::array images{solid_texture({32,64,96,255})};pt::refresh_color_guides(scene,images);
        return scene;
    };
    const auto draw=[&](const pt::Scene& scene,bool filtered=false) {
        return gpu.draw(scene,primary,{},Camera{},~0u,false,filtered,&black);
    };
    auto scene=make();auto raw=draw(scene);
    const std::array<float,3> expected{16,64,144};
    float error{};
    for(unsigned c=0;c<3;++c) {
        error=std::max(error,std::abs(mean(raw,c)-expected[c]));
        require(std::abs(mean(raw,c)-expected[c])<1.1f,
            "Qualified primary interior lost its authored diffuse baseline in a black environment");
    }
    std::vector<std::uint8_t> filtered;
    for(unsigned frame=0;frame<16;++frame)filtered=draw(scene,true);
    for(unsigned c=0;c<3;++c)require(std::abs(mean(filtered,c)-expected[c])<1.1f,
        "Denoising changed the full-resolution authored primary diffuse baseline");
    // Every baked guide (lightmap image or static radiance) carries its 2*RGB
    // energy; the authenticated tag no longer gates it (source_indirect.glsl),
    // so interiors lit only by baked fixtures are not left black.
    auto unqualified=make({},false);const auto untagged=draw(unqualified);
    for(unsigned c=0;c<3;++c)require(std::abs(mean(untagged,c)-expected[c])<1.1f,
        "An untagged baked guide lost its authored diffuse baseline");
    // An authored distant source retains the original traced outdoor route,
    // including a true zero for a sun behind this receiver's normal.
    pt::Light sun;sun.direction=Vec3{0,0,1};sun.color={1,1,1};
    auto outdoors=make(std::span(&sun,1));require(mean(draw(outdoors),0)<1,
        "Primary cached brightness bypassed the authored distant-light shadow route");
    // Current local lighting remains additive and obeys its actual blocker.
    const pt::Light local{{0,0,-2},10,{1,1,1},.25f};
    auto local_scene=make(std::span(&local,1));const auto illuminated=draw(local_scene);
    require(mean(illuminated,0)>mean(raw,0)+6,
        "Source baseline suppressed current traced local diffuse lighting");
    // A strong baked mesh emitter must never randomize a live punctual lamp.
    // A second, live mesh emitter must contribute even when its share of the
    // original all-emitter power would be tiny.
    auto baked_geometry=primary;
    quad(baked_geometry,-2,-2,2,2,-1,{0,0,1},1);
    for(auto& vertex:baked_geometry)vertex.lightmap=1;
    auto live_geometry=primary;
    quad(live_geometry,-.6f,-.6f,.6f,.6f,-1,{0,0,1},2);
    live_geometry.resize(primary.size()+3); // Keep identical source samples in both scenes.
    auto mixed_geometry=baked_geometry;
    mixed_geometry.insert(mixed_geometry.end(),live_geometry.begin()+primary.size(),live_geometry.end());
    const auto receive=[&](std::span<const Vertex> vertices,std::span<const pt::Light> lights) {
        const std::array material_bank{material,white_material(),white_material()};
        auto fixture=pt::build_scene(vertices,material_bank,lights);
        fixture.materials[0]=scene.materials[0];fixture.color_guide_pixels=scene.color_guide_pixels;
        auto& bake=fixture.materials[1];
        bake.texture[3]|=pt::baked_emission_flag|pt::light_only_flag|pt::uniform_emission_flag;
        bake.color_guide=scene.materials[0].color_guide;
        bake.emission={100,100,100,0};
        auto& live=fixture.materials[2];
        live.texture[3]|=pt::light_only_flag|pt::uniform_emission_flag;
        live.emission={3,3,3,0};
        pt::update_geometry(fixture,vertices,lights);
        return fixture;
    };
    const auto baked_mesh=receive(baked_geometry,std::span(&local,1));
    require(baked_mesh.emissive_count==2 && draw(baked_mesh)==illuminated,
        "All-baked emitter bank introduced noise into a deterministic live light");
    const auto live_mesh=receive(live_geometry,{});
    auto mixed_mesh=receive(mixed_geometry,{});
    const auto live_pixels=draw(live_mesh);
    require(live_mesh.emissive_count==1 && mixed_mesh.emissive_count==3 &&
        mean(live_pixels,0)>mean(raw,0)+1 && draw(mixed_mesh)==live_pixels,
        "Baked emitter power changed the sampling or energy of the live mesh light");
    const auto frame_delta=[](const auto& first,const auto& second) {
        float sum{};
        for(unsigned y=8;y<height-8;++y)for(unsigned x=8;x<width-8;++x) {
            const auto at=(y*width+x)*4+1;
            const float delta=float(first[at])-float(second[at]);sum+=delta*delta;
        }
        return std::sqrt(sum/float((width-16)*(height-16)));
    };
    const auto mesh_first=draw(mixed_mesh,true),mesh_second=draw(mixed_mesh,true);
    for(unsigned frame=0;frame<14;++frame)draw(mixed_mesh,true);
    const auto mesh_converged=draw(mixed_mesh,true),mesh_next=draw(mixed_mesh,true);
    const float initial_flicker=frame_delta(mesh_first,mesh_second),converged_flicker=frame_delta(mesh_converged,mesh_next);
    std::cout<<"Cached receiver area-light RMS frame difference: "<<initial_flicker<<" -> "<<converged_flicker<<'\n';
    require(initial_flicker>.1f && converged_flicker<initial_flicker*.65f &&
        std::abs(mean(mesh_converged,1)-mean(live_pixels,1))<2.f,
        "Area light did not converge or changed its authored direct light energy");
    auto mesh_shadow_geometry=mixed_geometry;
    quad(mesh_shadow_geometry,-3,-3,3,3,0,{0,0,-1},0);
    pt::update_geometry(mixed_mesh,mesh_shadow_geometry);
    const auto mesh_shadow=draw(mixed_mesh,true);
    require(std::abs(mean(mesh_shadow,1)-expected[1])<1.1f,
        "An area-light history retained obsolete direct light after a caster moved into its path");
    pt::update_geometry(mixed_mesh,mixed_geometry);
    for(unsigned frame=0;frame<8;++frame)draw(mixed_mesh,true);
    mixed_mesh.materials[2].emission={.01f,.01f,.01f,0};
    pt::update_geometry(mixed_mesh,mixed_geometry);
    const auto dimmed_mesh=draw(mixed_mesh,true);
    require(std::abs(mean(dimmed_mesh,1)-expected[1])<1.1f,
        "Area-direct history retained stale radiance from a stationary animated emitter");
    // Match sample sequences across two separate histories. The moving
    // triangle stays in the receiving hemisphere, beyond both ends of
    // every wall-to-emitter segment. It cannot change area visibility.
    {
        auto wall_geometry=primary;
        quad(wall_geometry,-.9f,-.15f,-.6f,.15f,-1,{0,0,1},2);
        for(unsigned n=0;n<23;++n) {
            const float offset=float(n)*.01f;
            quad(wall_geometry,.6f+offset,-.15f,.9f+offset,.15f,-1,{0,0,1},2);
        }
        quad(wall_geometry,.1f,-2,2,2,-.94f,{0,0,1},0);
        const auto moving_first=wall_geometry.size();
        quad(wall_geometry,3,-.15f,3.3f,.15f,-.3f,{0,0,1},0);
        auto control_scene=receive(wall_geometry,{});
        control_scene.materials[2].emission={2.2f,.07f,4,0};
        pt::update_geometry(control_scene,wall_geometry,{},primary.size());
        auto moving_scene=control_scene;
        Fixture control,animated;
        control.initialize(shaders);animated.initialize(shaders);
        const auto draw_wall=[&](Fixture& fixture,const pt::Scene& state) {
            return fixture.draw(state,primary,{},Camera{},~0u,false,true,&black);
        };
        for(auto* fixture:{&control,&animated}) {
            fixture->primary_color({.3f,.27f,.34f,1});
            fixture->renderer->set_quality(1,1);
        }
        for(unsigned frame=0;frame<16;++frame) {
            draw_wall(control,control_scene);
            draw_wall(animated,moving_scene);
        }
        for(std::size_t n=moving_first;n<wall_geometry.size();++n)wall_geometry[n].position.x+=.25f;
        pt::update_geometry(moving_scene,wall_geometry,{},primary.size());
        const auto expected_wall=draw_wall(control,control_scene);
        const auto actual_wall=draw_wall(animated,moving_scene);
        const auto blue_delta=std::abs(mean(expected_wall,2)-mean(actual_wall,2));
        std::cout<<"Cached wall side caster mean blue difference: "<<blue_delta<<'\n';
        require(actual_wall==expected_wall,
            "A moving caster outside every wall-to-area-light segment reset the cached receiver history");

        // With the diffuse path already integrated in the bake, weak gloss
        // must carry its real weight on every remaining path. Rare amplified
        // samples otherwise look grainy or lose their energy on a new shot.
        auto glossy_geometry=primary;
        quad(glossy_geometry,-3,-2,3,2,-1.2f,{0,0,1},1);
        auto glossy=receive(glossy_geometry,{});
        glossy.materials[0].response={.63f,.035f,.035f,0};
        glossy.materials[1].color_guide={2,0,0,0};
        glossy.materials[1].emission={.25f,.015f,.51f,0};
        const std::array dark_guide{solid_texture({18,13,23,255})};
        pt::refresh_color_guides(glossy,dark_guide);
        pt::update_geometry(glossy,glossy_geometry,{},primary.size());
        const float initial_blue=mean(draw_wall(control,glossy),2,8,56,8,56);
        std::vector<std::uint8_t> settled;
        for(unsigned frame=0;frame<16;++frame)settled=draw_wall(control,glossy);
        const float settled_blue=mean(settled,2,8,56,8,56);
        std::cout<<"Cached wall reflection blue, first and settled: "<<initial_blue<<", "<<settled_blue<<'\n';
        require(settled_blue>17.f && std::abs(initial_blue-settled_blue)<2.5f,
            "A weak reflection on lightmapped geometry lost its energy before accumulation settled");
    }
    auto geometry=primary;quad(geometry,-2,-2,2,2,-.5f,{0,0,-1},0);
    auto blocked=pt::build_scene(geometry,std::span(&material,1),std::span(&local,1));
    blocked.materials[0]=scene.materials[0];blocked.color_guide_pixels=scene.color_guide_pixels;
    const auto shadow=draw(blocked);
    for(unsigned c=0;c<3;++c)require(std::abs(mean(shadow,c)-expected[c])<1.1f,
        "Current local shadow erased the source baseline or retained blocked local light");
    // A second cached surface may not contribute the already integrated
    // static diffuse field a second time at a qualified interior primary.
    auto cached_wall=blocked;cached_wall.lights.clear();
    for(auto& triangle:cached_wall.triangles) {
        Vertex vertex;vertex.lightmap=1;vertex.lightmap_uv={.5f,.5f};
        triangle.color_guide=pt::color_guide_triangle(vertex,vertex,vertex,cached_wall.materials[0]);
    }
    const auto once=draw(cached_wall);gpu.renderer->set_quality(1,2);const auto twice=draw(cached_wall);
    for(unsigned c=0;c<3;++c)require(std::abs(mean(once,c)-expected[c])<1.1f && once==twice,
        "Primary authored diffuse baseline double-counted secondary static transport");
    // Native source ambient is additive, independent of the new traced
    // diffuse/reflection energy split (including the source reflection mask).
    gpu.renderer->set_quality(1,1);
    for(float reflection:{0.f,.4f,1.f}) {
        auto reflective=scene;reflective.materials[0].response={.3f,reflection,reflection,0};
        reflective.materials[0].texture[3]|=2048u;
        for(float mask:{0.f,.5f,1.f})for(bool filter:{false,true}) {
            gpu.primary_color({.25f,.5f,.75f,mask});
            const auto frame=draw(reflective,filter);
            for(unsigned c=0;c<3;++c)require(std::abs(mean(frame,c)-expected[c])<1.1f,
                "Traced reflection strength depleted authored primary diffuse energy");
        }
    }
    gpu.primary_color({.25f,.5f,.75f,1});
    // Same material and geometry, two actual descriptor owners in one frame.
    // Their current ambient changes after history converges; full-resolution
    // resolve must consume the matching owner rather than a grid/history tint.
    auto model_scene=make({},false);model_scene.materials[0].color_guide={};
    model_scene.materials[0].texture[3]|=pt::source_ambient_flag;
    std::vector<Vertex> owners;quad(owners,-1,-1,0,1,.5f,{0,0,-1},0);quad(owners,0,-1,1,1,.5f,{0,0,-1},0);
    gpu.primary_color({.5f,.5f,1,1},2);PrimaryAmbientOwners bindings(gpu);
    const std::array owner_descriptors{bindings.sets[0],bindings.sets[0],bindings.sets[1],bindings.sets[1]};
    for(unsigned frame=0;frame<18;++frame) {
        const std::uint32_t first=frame<16?0x00334d66u:0u,second=frame<16?0x00661933u:0x001a334du;
        bindings.set(0,first);bindings.set(1,second);
        const auto image=gpu.draw(model_scene,owners,{},Camera{},~0u,false,true,&black,true,false,true,nullptr,owner_descriptors);
        for(unsigned owner=0;owner<2;++owner)for(unsigned c=0;c<3;++c) {
            const unsigned color=owner?second:first,shift=(2-c)*8;
            const float expectedAmbient=2.f*float((color>>shift)&255u)/255.f;
            const float sourceAlbedo=float((c+1)*64)/255.f;
            const float expectedByte=sourceAlbedo*expectedAmbient*255.f;
            require(std::abs(mean(image,c,owner?40:8,owner?56:24)-expectedByte)<1.1f,
                "Current primary owner ambient was shared, delayed by history, or multiplied into albedo twice");
        }
    }
    // Native signed bump Z controls this ambient term at its actual texture
    // sample, independent of source albedo and converged traced lighting.
    gpu.primary_color({.5f,.5f,.5f,1},2);bindings.set(0,0x00334d66u);bindings.set(1,0x00661933u);
    const auto flat=gpu.draw(model_scene,owners,{},Camera{},~0u,false,true,&black,true,false,true,nullptr,owner_descriptors);
    require(mean(flat,0,8,24)<1 && mean(flat,2,40,56)<1,
        "Primary ambient dropped the native signed bump-map Z factor");
    model_scene.materials[0].texture[3]|=pt::source_flat_ambient_flag;
    const auto regalia=gpu.draw(model_scene,owners,{},Camera{},~0u,false,true,&black,true,false,true,nullptr,owner_descriptors);
    require(std::abs(mean(regalia,0,8,24)-64.f*2.f*51.f/255.f)<1.1f &&
        std::abs(mean(regalia,2,40,56)-192.f*2.f*51.f/255.f)<1.1f,
        "A combined-lighting actor incorrectly lost its authored ambient on a flat bump sample");
    model_scene.materials[0].texture[3]&=~pt::source_ambient_flag;
    model_scene.materials[0].color_guide={pt::source_default_ambient_kind,0,0x00334d66u,0};
    const auto fixed_regalia=gpu.draw(model_scene,owners,{},Camera{},~0u,false,true,&black,true,false,true,nullptr,owner_descriptors);
    require(std::abs(mean(fixed_regalia,0)-64.f*2.f*51.f/255.f)<1.1f &&
        std::abs(mean(fixed_regalia,2)-192.f*2.f*102.f/255.f)<1.1f,
        "Fixed accumulated actor ambient was overridden by live owner data");

    // The two halves of the bound lighting image slope toward and away from
    // the sun, with equal bump Z. Independent c18/c20 must control reflected
    // and diffuse relief as well as owner ambient. The neutral atlas path
    // would give both receivers the same smooth-normal shading.
    gpu.primary_color({1,1,1,1});
    std::vector<std::uint8_t> bumpPixels(width*height*4);
    for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x) {
        const auto pixel=(y*width+x)*4;
        bumpPixels[pixel]=x<width/2?204:51;
        bumpPixels[pixel+1]=128;bumpPixels[pixel+2]=230;bumpPixels[pixel+3]=255;
    }
    gpu.primary_texture(bumpPixels,2);
    for(auto& vertex:owners){vertex.tangent={1,0,0};vertex.binormal={0,1,0};}
    sun.direction=Vec3{0,.8f,-.6f};
    auto reliefScene=pt::build_scene(owners,std::span(&material,1),std::span(&sun,1));
    reliefScene.materials[0].response={1,0,0,0};
    reliefScene.materials[0].texture[3]|=pt::source_ambient_flag;
    const ShaderVector base{.1f,1,.05f,0},away{-.5f,1,.6f,0},toward{.5f,1,.15f,0};
    for(const float viewRole:{1.f,2.f}) {
        Camera camera;camera.eye[3]=viewRole;
        bindings.set(0,0,base,away);bindings.set(1,0,base,toward);
        const auto shade=[&] {
            return gpu.draw(reliefScene,owners,{},camera,~0u,false,true,&black,true,false,true,nullptr,owner_descriptors);
        };
        const auto originalRelief=shade();
        require(mean(originalRelief,0,8,24)<3 && mean(originalRelief,0,40,56)>225,
            "Object relief ignored its completed native c18/c20 and bump image");
        bindings.set(0,0,base,toward);
        const auto updatedRelief=shade();
        require(mean(updatedRelief,0,8,24)>225 && mean(updatedRelief,0,40,56)==mean(originalRelief,0,40,56),
            "Moving one object's bump coordinates left stale relief or affected its neighbor");
    }
    reliefScene.materials[0].texture[3]&=~pt::source_ambient_flag;
    reliefScene.materials[0].color_guide={pt::source_default_ambient_kind,0,0,0};
    WorldMaterialUniform defaultLighting;
    defaultLighting.forward_transforms[0]=base;
    defaultLighting.forward_transforms[2]=away;
    std::memcpy(bindings.buffers[0].mapped,&defaultLighting,sizeof(defaultLighting));
    defaultLighting.forward_transforms[2]=toward;
    std::memcpy(bindings.buffers[1].mapped,&defaultLighting,sizeof(defaultLighting));
    const auto defaultRelief=gpu.draw(reliefScene,owners,{},Camera{},~0u,false,true,&black,true,false,true,nullptr,owner_descriptors);
    require(mean(defaultRelief,0,8,24)<3 && mean(defaultRelief,0,40,56)>225,
        "Fixed default lighting lost its authored bump coordinates when its ambient ignored bump Z");

    // Keep the authored color mask and the current owner's paint separate
    // from irradiance. Qualify the same default program used by NPC armor.
    auto painted=white_material();
    painted.forward_pass=true;painted.forward_vertex=6;painted.forward_pass_index=0;
    auto& paintPass=painted.passes.emplace_back();
    paintPass.name=pt::surface_detail::one_change_color_pass;
    paintPass.vertex_shader="rasterizer\\vertex_shaders_xbox\\lightmap_dynamic_tex_detail";
    paintPass.implementation=3;
    paintPass.pixel_externs={{0x00c0,4,3}};
    painted.vertex_program=vertex_bytes(pt::unlit_uv_detail::dynamic_detail_words);
    paintPass.vertex_code=painted.vertex_program;
    painted.forward_program.words=pt::surface_detail::default_one_change_color;
    painted.forward_program.words[15]=0x007f7f7fu;
    painted.forward_transforms[0]=painted.forward_transforms[1]=painted.forward_transforms[2]={1,1,0,0};
    painted.detail=solid_texture({255,0,128,255});
    gpu.primary_color({1,0,128.f/255.f,1},1);
    gpu.primary_color({.5f,.5f,1,1},2);
    auto paintScene=pt::build_scene(primary,std::span(&painted,1));
    auto paintUniform=WorldMaterialUniform{};
    paintUniform.program=painted.forward_program.words;
    paintUniform.program[13]=0x00402080u;
    paintUniform.forward_transforms=painted.forward_transforms;
    const std::array paintInputs{pt::MaterialAliasInput{{1,0,0},&paintUniform}};
    const auto paintBindings=pt::refresh_material_aliases(paintScene,std::span(&painted,1),paintInputs);
    native_uniform(gpu,painted);
    const auto tinted=gpu.draw(paintScene,primary,{},Camera{},paintBindings.at(0).material,false,true,&black);
    const float irradiance=254.f/255.f;
    require(std::abs(mean(tinted,0)-64.f*irradiance)<1.1f && std::abs(mean(tinted,1)-255.f*irradiance)<1.1f &&
        std::abs(mean(tinted,2)-(255.f+(128.f-255.f)*(128.f/255.f))*irradiance)<1.1f,
        "NPC surface resolve lost its per-channel paint mask or completed owner tint");
    require(gpu.validation_errors.load()==0,"Primary source baseline fixture reported Vulkan errors");
    std::cout<<"Primary source baseline: maximum_byte_error="<<error<<'\n';return 0;
}
