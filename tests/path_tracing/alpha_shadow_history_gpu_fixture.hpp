#pragma once

int alpha_shadow_history_gpu(const std::filesystem::path& shaders) {
    Fixture gpu;gpu.initialize(shaders,{128,128},true);gpu.fence_submissions=true;
    gpu.renderer->set_quality(1,1);
    std::vector<Vertex> primary;quad(primary,-1,-1,1,1,.5f,{0,0,-1},0);
    auto geometry=primary;quad(geometry,-1,-1,1,1,-3,{0,0,1},1);
    pt::Light light;light.direction=Vec3{0,0,-1};light.color={.3f,.3f,.3f};
    const std::array lights{light,light};
    const auto center=[](const std::vector<std::uint8_t>& image) {
        double sum{};for(unsigned y=32;y<96;++y)for(unsigned x=32;x<96;++x)
            sum+=image[(y*128+x)*4];
        return sum/4096;
    };
    for(unsigned variant=0;variant<3;++variant) {
        std::array materials{white_material(),white_material()};
        auto& caster=materials[1];caster.forward_raster.alpha_test=true;
        caster.forward_raster.alpha_function=0x204;caster.forward_raster.alpha_reference=64;
        caster.diffuse=solid_texture({255,255,255,128});
        if(variant==2) {
            caster.diffuse.width=2;caster.diffuse.pixels.resize(8);
            for(unsigned n=0;n<8;++n)caster.diffuse.pixels[n]=std::byte{255};
            caster.diffuse.pixels[3]=std::byte{0};
            caster.texture_states[0].address_u=3;
        }
        auto vertices=geometry;
        for(std::size_t n=primary.size();n<vertices.size();++n)vertices[n].uv={.75f,.5f};
        auto scene=pt::build_scene(vertices,materials,lights);
        const auto draw=[&](bool capture) {
            return gpu.draw(scene,primary,{},Camera{},~0u,false,true,nullptr,true,capture,capture);
        };
        for(unsigned frame=0;frame<15;++frame)draw(false);
        const auto before=center(draw(true));
        if(variant==0)caster.forward_raster.alpha_reference=192;
        else if(variant==1)caster.forward_raster.alpha_function=0x201;
        else for(std::size_t n=primary.size();n<vertices.size();++n)vertices[n].uv={.25f,.5f};
        pt::refresh_materials(scene,materials);pt::update_geometry(scene,vertices,lights);
        const auto changed=center(draw(true));
        std::cout<<"Path Tracing animated alpha shadow: {\"case\":"<<variant
            <<",\"before\":"<<before<<",\"changed\":"<<changed<<'}'<<std::endl;
        require(before<1,"Alpha-shadow control did not establish an opaque shadow");
        require(changed>150,"Denoiser retained stale alpha-caster cutoff, comparison or vertex-UV shadow history");
    }
    require(gpu.validation_errors.load()==0,"Animated alpha-shadow fixture reported Vulkan errors");
    return 0;
}
