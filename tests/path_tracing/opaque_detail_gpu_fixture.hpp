#pragma once
#include "halo2/path_tracing/material_surface.hpp"

// Actual native control/vertex identities, with controlled image operands.
// The CPU source test independently checks these expressions against the
// complete authored programs and images in original/prepared Lockout caches.
int opaque_detail_gpu(const std::filesystem::path& shaders) {
    Fixture gpu;gpu.initialize(shaders);gpu.renderer->set_quality(1,1);
    auto material=native_surface(false);
    material.passes[0].name="shaders\\shader_passes\\lightmap\\lightmap_tex_detail";
    material.forward_program.words=pt::surface_detail::opaque_detail;
    material.forward_raster.alpha_test=0;
    material.forward_transforms[0]={.75f,.5f,.125f,.25f};
    material.forward_transforms[1]={2,1,.25f,.125f};
    for(unsigned stage:{0u,1u})material.texture_states[stage].mag_filter=material.texture_states[stage].min_filter=1;
    const ShaderVector base{80.f/255,120.f/255,160.f/255,1};
    material.diffuse=solid_texture({80,120,160,255});gpu.primary_color(base);
    std::vector<std::uint8_t> detail(width*height*4);
    for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x) {
        const std::array<std::uint8_t,4> color=x%16<2?std::array<std::uint8_t,4>{24,32,40,255}:
            std::array<std::uint8_t,4>{160,128,96,255};
        std::copy(color.begin(),color.end(),detail.begin()+(y*width+x)*4);
    }
    material.detail={width,height,11,{}};material.detail.pixels.resize(detail.size());
    std::memcpy(material.detail.pixels.data(),detail.data(),detail.size());gpu.primary_texture(detail,1);
    native_uniform(gpu,material);
    std::vector<Vertex> primary;quad(primary,-1,-1,1,1,.5f,{0,0,-1},0);
    for(auto& vertex:primary)vertex.color={0,0,0};
    auto scene=pt::build_scene(primary,std::span(&material,1));
    const auto qualified=pt::qualified_surface(material);
    require(qualified && qualified->family==pt::SurfaceFamily::OpaqueDetail,
        "Opaque detail GPU fixture failed complete native identity admission");
    require((scene.materials[0].texture[3]&pt::surface_flag)!=0,
        "Opaque detail GPU fixture did not activate its source material residency");
    const auto check_pixels=[&](const std::vector<std::uint8_t>& frame,const char* message,bool holes=false) {
        double squared{};float maximum{};unsigned count{};
        for(unsigned y=4;y<height-4;++y)for(unsigned x=4;x<width-4;++x) {
            const Vec2 raw{(float(x)+.5f)/width,(float(y)+.5f)/height};
            const auto source=pt::material_surface(scene,0,raw);
            require(source.has_value() || holes,"Opaque primary unexpectedly rejected native diffuse texels");
            for(unsigned c=0;c<3;++c) {
                const float expected=source?(*source)[c]*63.75f:0;
                const float error=std::abs(float(frame[(y*width+x)*4+c])-expected);
                maximum=std::max(maximum,error);squared+=error*error;++count;
            }
        }
        require(maximum<1.2f,message);
        return std::sqrt(squared/count);
    };
    const auto raw=gpu.draw(scene,primary,{.25f,.25f,.25f});
    const auto primary_error=check_pixels(raw,"Primary opaque material lost source T1 panel detail or ordered UVs");
    check_pixels(gpu.draw(scene,primary,{.25f,.25f,.25f},Camera{},~0u,true),
        "Opaque source-W prepass changed qualified native material coverage");
    const auto filtered_error=check_pixels(gpu.draw(scene,primary,{.25f,.25f,.25f},Camera{},~0u,false,true),
        "Filtered opaque resolve lost full-resolution native panel detail");

    // Raw T1 alpha-kill remains independent of final opaque alpha. Check both
    // raster shading and its source-W prepass through actual GPU draws.
    for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width/2;++x)detail[(y*width+x)*4+3]=0;
    std::memcpy(material.detail.pixels.data(),detail.data(),detail.size());material.texture_states[1].alpha_kill=4;
    gpu.primary_texture(detail,1);pt::refresh_materials(scene,std::span(&material,1));
    check_pixels(gpu.draw(scene,primary,{.25f,.25f,.25f},Camera{},~0u,true),
        "Opaque detail source-W role ignored its authored raw T1 alpha kill",true);
    check_pixels(gpu.draw(scene,primary,{.25f,.25f,.25f},Camera{},~0u,false,true),
        "Filtered opaque role filled authored raw T1 alpha-kill holes",true);

    // A secondary diffuse hit must use the same two source textures. Match
    // the exact geometry/seed with neutral T1 so any common visibility is
    // independent of this material-color assertion. The old T0-only fallback
    // produces equal frames and fails the non-unit red/blue ratios below.
    material.diffuse=solid_texture({80,120,160,255});material.detail=solid_texture({160,128,96,255});
    material.texture_states[1].alpha_kill=0;
    std::array materials{white_material(),material};std::vector<Vertex> wall;
    quad(wall,-16384,-16384,16384,16384,-1,{0,0,1},1);
    const auto secondary_scene=pt::build_scene(wall,materials);
    gpu.primary_color({1,1,1,1});std::memset(gpu.uniform.mapped,0,sizeof(WorldMaterialUniform));
    for(auto& vertex:primary)vertex.color={1,1,1};
    const auto secondary=gpu.draw(secondary_scene,primary,{.25f,.25f,.25f});
    auto neutral_materials=materials;neutral_materials[1].detail=solid_texture({128,128,128,255});
    const auto neutral_scene=pt::build_scene(wall,neutral_materials);
    const auto neutral=gpu.draw(neutral_scene,primary,{.25f,.25f,.25f});
    require(mean(neutral,0)>1,"Secondary opaque albedo control received no measurable lighting");
    unsigned black_secondary{},black_neutral{};float secondary_error{};
    for(unsigned y=24;y<40;++y)for(unsigned x=24;x<40;++x) {
        const auto at=(y*width+x)*4;
        black_secondary+=secondary[at]<=1 && secondary[at+1]<=1 && secondary[at+2]<=1;
        black_neutral+=neutral[at]<=1 && neutral[at+1]<=1 && neutral[at+2]<=1;
        for(unsigned c=0;c<3;++c) {
            const float ratio=float(std::array<unsigned,3>{160,128,96}[c])/128;
            secondary_error=std::max(secondary_error,std::abs(float(secondary[at+c])-neutral[at+c]*ratio));
        }
    }
    require(secondary_error<1.2f,"Secondary opaque bounce dropped authored T1 detail from its source albedo");
    // Report a moderate-coordinate visibility control separately. It has no
    // material-acceptance threshold and does not replace source visibility
    // regressions; it constrains interpretation of the oversized plane above.
    std::vector<Vertex> moderate_wall;quad(moderate_wall,-256,-256,256,256,-1,{0,0,1},1);
    const auto moderate_scene=pt::build_scene(moderate_wall,materials);
    const auto moderate=gpu.draw(moderate_scene,primary,{.25f,.25f,.25f});unsigned black_moderate{};
    for(unsigned y=24;y<40;++y)for(unsigned x=24;x<40;++x) {
        const auto at=(y*width+x)*4;
        black_moderate+=moderate[at]<=1 && moderate[at+1]<=1 && moderate[at+2]<=1;
    }
    require(gpu.validation_errors.load()==0,"Vulkan validation reported errors in opaque native surface draws");
    std::cout<<"PASS native opaque GPU: primary_RMS="<<primary_error<<" filtered_RMS="<<filtered_error
        <<" secondary_color_error="<<secondary_error<<" secondary_RGB="<<mean(secondary,0)<<','<<mean(secondary,1)<<','<<mean(secondary,2)
        <<" neutral_RGB="<<mean(neutral,0)<<','<<mean(neutral,1)<<','<<mean(neutral,2)
        <<" black_large="<<black_secondary<<" black_neutral="<<black_neutral<<" black_moderate="<<black_moderate
        <<" moderate_RGB="<<mean(moderate,0)<<','<<mean(moderate,1)<<','<<mean(moderate,2)<<'\n';
    return 0;
}
