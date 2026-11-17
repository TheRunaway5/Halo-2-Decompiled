#pragma once

int moving_light_sampling_gpu(const std::filesystem::path& shaders) {
    constexpr VkExtent2D extent{256,256};
    Fixture gpu;gpu.initialize(shaders,extent,true);gpu.fence_submissions=true;
    gpu.primary_color({.1f,.1f,.1f,1});gpu.renderer->set_quality(1,1);
    const std::array materials{white_material()};
    std::vector<Vertex> primary;quad(primary,-1,-1,1,1,.5f,{0,0,-1},0);
    std::array<pt::Light,64> lights;
    for(unsigned n=0;n<lights.size();++n) {
        lights[n].position={12+float(n),12,-3};lights[n].radius=1;lights[n].color={1,1,1};
    }
    lights[0].position=lights[1].position={0,0,-3};lights[0].radius=lights[1].radius=100;
    lights[0].color={1.8f,1.8f,1.8f};lights[1].color={.2f,.2f,.2f};
    auto scene=pt::build_scene(primary,materials,lights);
    for(unsigned frame=0;frame<16;++frame) {
        lights[0].position.x=lights[1].position.x=float(frame)*.01f;
        pt::update_geometry(scene,primary,lights);
        if(frame!=15)gpu.draw(scene,primary,{},Camera{},~0u,false,true,nullptr,true,false,false);
    }
    const auto changed=gpu.draw(scene,primary,{},Camera{},~0u,false,true,nullptr,true,true);
    const auto history=gpu.renderer->temporal_stats();
    // Two co-located gray lights have a known analytic sum. Splitting that
    // source into unequal pieces and adding62 noncontributing source lights
    // cannot change its expected energy or introduce one-sample bank noise.
    auto summed=lights[0];summed.color={2,2,2};
    auto reference_scene=pt::build_scene(primary,materials,std::span(&summed,1));
    const auto reference=gpu.draw(reference_scene,primary,{},Camera{},~0u,false,true);
    const auto raw=gpu.draw(scene,primary,{},Camera{},~0u,false,false);
    const auto measure=[&](const std::vector<std::uint8_t>& frame) {
        double error{},mean{};unsigned count{};
        for(unsigned y=32;y<224;++y)for(unsigned x=32;x<224;++x) {
            const auto at=(std::size_t(y)*extent.width+x)*4;
            error+=std::abs(int(frame[at])-int(reference[at]));mean+=frame[at];++count;
        }
        return std::array{mean/count,error/count};
    };
    const auto filtered=measure(changed),unfiltered=measure(raw),control=measure(reference);
    std::cout<<"Path Tracing moving light sampling: {\"sources\":64,\"contributing\":2,\"frames\":16"
        <<",\"reference_mean\":"<<control[0]<<",\"filtered_mean\":"<<filtered[0]
        <<",\"filtered_error\":"<<filtered[1]<<",\"raw_error\":"<<unfiltered[1]
        <<",\"indirect_history_mean\":"<<history.banks[0].mean_count
        <<",\"validation_errors\":"<<gpu.validation_errors.load()<<'}'<<std::endl;
    require(control[0]>40 && control[0]<55,"Moving-light control lost its known unoccluded analytic illumination");
    require(unfiltered[1]<1.0 && filtered[1]<1.0,
        "Moving unequal lights or inactive bank entries introduced avoidable direct-light sampling grain");
    require(history.banks[0].mean_count>15.5,"Moving local lights reset unrelated indirect lighting history");
    require(gpu.validation_errors.load()==0,"Moving-light sampling fixture reported Vulkan errors");
    return 0;
}
