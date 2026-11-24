#pragma once
#include "halo2/path_tracing/color_guide.hpp"
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
    void set(unsigned owner,std::uint32_t sourceAmbient) {
        WorldMaterialUniform primary{},lighting{};lighting.program[15]=sourceAmbient;
        lighting.forward_transforms[0]=lighting.forward_transforms[2]={1,1,0,0};
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
    require(gpu.validation_errors.load()==0,"Primary source baseline fixture reported Vulkan errors");
    std::cout<<"Primary source baseline: maximum_byte_error="<<error<<'\n';return 0;
}
