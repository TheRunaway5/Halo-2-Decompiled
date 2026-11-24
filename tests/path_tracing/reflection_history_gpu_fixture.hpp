#pragma once

int foreground_reflection_turn_gpu(const std::filesystem::path& shaders) {
    constexpr VkExtent2D extent{128,128};
    Fixture gpu;gpu.initialize(shaders,extent,true);gpu.fence_submissions=true;
    gpu.environment_side=32;gpu.renderer->set_quality(1,1);
    const std::array materials{white_material()};auto scene=pt::build_scene({},materials);
    auto& mirror=scene.materials[0];mirror.base=mirror.glancing_tint={1,1,1,1};make_mirror(mirror,.12f);
    const std::array<Vec3,6> sky{{{0,1,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{1,0,0}}};
    const auto draw=[&](const pt::Scene& owner,float yaw,bool capture) {
        const float s=std::sin(yaw),c=std::cos(yaw);
        const auto rotate=[&](Vec3 value){return Vec3{c*value.x+s*value.z,value.y,-s*value.x+c*value.z};};
        std::vector<Vertex> primary;
        quad(primary,-1,-1,1,1,.5f,{std::sin(.3926990817f),0,-std::cos(.3926990817f)},0);
        for(auto& vertex:primary){vertex.position=rotate(vertex.position);vertex.normal=rotate(vertex.normal);}
        Camera camera;camera.matrix={c,0,s,0,0,1,0,0,-s,0,c,0,0,0,0,1};
        const auto eye=rotate({0,0,-10000});camera.eye={eye.x,eye.y,eye.z,2};camera.forward={s,0,c,0};
        // A rigidly camera-attached surface has unchanged camera-space guides,
        // but turning the camera rotates its reflection through the WORLD sky.
        return gpu.draw(owner,primary,{},camera,~0u,false,true,&sky,true,capture,capture);
    };
    auto reference_scene=scene;
    for(unsigned frame=0;frame<15;++frame)draw(reference_scene,-.07f,false);
    const auto reference=draw(reference_scene,-.07f,true);
    for(unsigned frame=0;frame<15;++frame)draw(scene,.07f,false);
    const auto before=draw(scene,.07f,true),changed=draw(scene,-.07f,true);
    const auto history=gpu.renderer->temporal_stats();
    std::cout<<"Path Tracing foreground reflection turn: {\"prior_red\":"<<mean(before,0)
        <<",\"reference_red\":"<<mean(reference,0)<<",\"reference_green\":"<<mean(reference,1)
        <<",\"changed_red\":"<<mean(changed,0)<<",\"changed_green\":"<<mean(changed,1)
        <<",\"diffuse_history_mean\":"<<history.banks[1].mean_count
        <<",\"validation_errors\":"<<gpu.validation_errors.load()<<'}'<<std::endl;
    require(mean(before,0)>220 && mean(reference,0)<20 && mean(reference,1)>220,
        "Foreground reflection-turn control lost its distinct WORLD sky endpoints");
    require(mean(changed,0)-mean(reference,0)<20 && mean(reference,1)-mean(changed,1)<20,
        "Denoiser retained a foreground reflection trail after a rigid camera turn");
    require(history.banks[1].mean_count>15.5,
        "Foreground reflection turn discarded valid camera-attached diffuse history");
    require(gpu.validation_errors.load()==0,"Foreground reflection-turn fixture reported Vulkan errors");
    return 0;
}

int reflected_object_history_gpu(const std::filesystem::path& shaders) {
    constexpr VkExtent2D extent{128,128};
    Fixture gpu;gpu.initialize(shaders,extent,true);gpu.fence_submissions=true;
    gpu.renderer->set_quality(1,1);
    const std::array materials{white_material(),white_material()};
    std::vector<Vertex> primary;quad(primary,-1,-1,1,1,.5f,{0,0,-1},0);
    auto before=primary,after=primary;
    quad(before,-1.5f,-1.5f,1.5f,1.5f,-3,{0,0,1},1);
    quad(after,4,-1.5f,7,1.5f,-3,{0,0,1},1);
    auto scene=pt::build_scene(before,materials);
    scene.materials[0].base=scene.materials[0].glancing_tint={1,1,1,1};
    make_mirror(scene.materials[0],.12f);
    scene.materials[1].emission={1,0,0,0};
    pt::update_geometry(scene,before);
    Camera camera;camera.eye={0,0,-10000,1};
    const auto draw=[&](const pt::Scene& owner,bool capture) {
        return gpu.draw(owner,primary,{0,1,0},camera,~0u,false,true,nullptr,true,capture,capture);
    };
    auto reference_scene=scene;pt::update_geometry(reference_scene,after);
    for(unsigned frame=0;frame<15;++frame)draw(reference_scene,false);
    const auto reference=draw(reference_scene,true);
    for(unsigned frame=0;frame<15;++frame)draw(scene,false);
    const auto before_frame=draw(scene,true);
    pt::update_geometry(scene,after);
    const auto changed=draw(scene,true);
    const auto history=gpu.renderer->temporal_stats();
    std::cout<<"Path Tracing reflected object history: {\"before_red\":"<<mean(before_frame,0)
        <<",\"reference_red\":"<<mean(reference,0)<<",\"reference_green\":"<<mean(reference,1)
        <<",\"changed_red\":"<<mean(changed,0)<<",\"changed_green\":"<<mean(changed,1)
        <<",\"diffuse_history_mean\":"<<history.banks[0].mean_count
        <<",\"validation_errors\":"<<gpu.validation_errors.load()<<'}'<<std::endl;
    require(mean(before_frame,0)>220 && mean(reference,0)<3 && mean(reference,1)>220,
        "Moving reflected-object control did not produce distinct radiance endpoints");
    require(mean(changed,0)<8 && mean(changed,1)>220,
        "Denoiser retained a trail from moving reflected geometry on a stationary receiver");
    require(history.banks[0].mean_count>15.5,"Reflected-object motion reset independent diffuse history");
    require(gpu.validation_errors.load()==0,"Reflected-object fixture reported Vulkan errors");
    return 0;
}

int unrelated_reflection_motion_gpu(const std::filesystem::path& shaders) {
    constexpr VkExtent2D extent{128,128};
    Fixture gpu;gpu.initialize(shaders,extent,true);gpu.fence_submissions=true;
    gpu.renderer->set_quality(1,1);gpu.environment_side=32;
    const std::array materials{white_material(),white_material()};
    constexpr float angle=.3926990817f;
    std::vector<Vertex> primary;quad(primary,-1,-1,1,1,.5f,{std::sin(angle),0,-std::cos(angle)},0);
    const auto geometry=[&](unsigned frame) {
        auto vertices=primary;const float x=10.f+.01f*float(frame%2);
        quad(vertices,x,10,x+1,11,-3,{0,0,1},1);return vertices;
    };
    auto scene=pt::build_scene(geometry(0),materials);
    scene.materials[0].base=scene.materials[0].glancing_tint={1,1,1,1};
    make_mirror(scene.materials[0],.25f);
    Camera camera;camera.eye={0,0,-10000,1};
    const std::array<Vec3,6> sky{{{0,1,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{1,0,0}}};
    const auto draw=[&](const pt::Scene& owner,bool capture) {
        return gpu.draw(owner,primary,{},camera,~0u,false,true,&sky,true,capture,capture);
    };
    auto reference_scene=scene;
    for(unsigned frame=0;frame<31;++frame)draw(reference_scene,false);
    const auto reference=noise_metrics(draw(reference_scene,true),extent);
    for(unsigned frame=0;frame<31;++frame) {
        pt::update_geometry(scene,geometry(frame));draw(scene,false);
    }
    pt::update_geometry(scene,geometry(31));
    const auto moving=noise_metrics(draw(scene,true),extent);
    std::cout<<"Path Tracing unrelated reflection motion: {\"reference_deviation\":"<<reference.deviation
        <<",\"moving_deviation\":"<<moving.deviation<<",\"reference_high_frequency\":"<<reference.high_frequency
        <<",\"moving_high_frequency\":"<<moving.high_frequency<<'}'<<std::endl;
    require(reference.deviation>1 && reference.mean>30 && reference.mean<220,
        "Unrelated reflection-motion control did not exercise stochastic specular lighting");
    require(moving.deviation<reference.deviation*1.5 && moving.high_frequency<reference.high_frequency*1.6,
        "Unrelated off-axis geometry animation discarded converged specular history");
    require(gpu.validation_errors.load()==0,"Unrelated reflection motion fixture reported Vulkan errors");
    return 0;
}

int reflection_history_gpu(const std::filesystem::path& shaders) {
    constexpr VkExtent2D extent{128,128};
    Fixture gpu;gpu.initialize(shaders,extent,true);gpu.fence_submissions=true;
    gpu.environment_side=32;
    gpu.renderer->set_quality(1,1);
    const std::array materials{white_material()};
    auto scene=pt::build_scene({},materials);
    scene.materials[0].base={1,1,1,1};make_mirror(scene.materials[0],.12f);
    const auto surface=[](float angle) {
        std::vector<Vertex> vertices;
        quad(vertices,-1,-1,1,1,.5f,{std::sin(angle),0,-std::cos(angle)},0);
        return vertices;
    };
    constexpr float quarter_pi=.7853981634f;
    const auto before=surface(quarter_pi*.5f-.035f),after=surface(quarter_pi*.5f+.035f);
    Camera camera;camera.eye={0,0,-10000,1};
    const std::array<Vec3,6> sky{{{0,1,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{1,0,0}}};
    const auto draw=[&](const pt::Scene& owner,const auto& vertices,bool capture) {
        return gpu.draw(owner,vertices,{},camera,~0u,false,true,&sky,true,capture,capture);
    };
    // Separate Scene ownership establishes independent fully converged
    // endpoints. The animation below changes only the receiver's smooth
    // normal; geometry, material and camera all remain on the same surface.
    auto reference_scene=scene;
    for(unsigned frame=0;frame<15;++frame)draw(reference_scene,after,false);
    const auto reference=draw(reference_scene,after,true);
    for(unsigned frame=0;frame<15;++frame)draw(scene,before,false);
    const auto warm=draw(scene,before,true);
    const auto changed=draw(scene,after,true);
    const auto channel=[&](const std::vector<std::uint8_t>& frame,unsigned c) {
        double sum{};unsigned count{};
        for(unsigned y=32;y<96;++y)for(unsigned x=32;x<96;++x) {
            sum+=frame[(std::size_t(y)*extent.width+x)*4+c];++count;
        }
        return sum/count;
    };
    const auto prior_red=channel(warm,0),reference_red=channel(reference,0),current_red=channel(changed,0);
    const auto reference_green=channel(reference,1),current_green=channel(changed,1);
    std::cout<<"Path Tracing reflection history: {\"prior_red\":"<<prior_red
        <<",\"reference_red\":"<<reference_red<<",\"changed_red\":"<<current_red
        <<",\"reference_green\":"<<reference_green<<",\"changed_green\":"<<current_green
        <<",\"validation_errors\":"<<gpu.validation_errors.load()<<'}'<<std::endl;
    require(prior_red>220 && reference_green>220 && reference_red<25,
        "Reflection-history control did not produce distinct authored sky highlights");
    require(current_red-reference_red<20 && reference_green-current_green<20,
        "Denoiser smeared a sharp reflected highlight across an animated smooth normal");
    require(gpu.validation_errors.load()==0,"Reflection-history fixture reported Vulkan errors");
    return 0;
}

int reflection_tint_history_gpu(const std::filesystem::path& shaders) {
    Fixture gpu;gpu.initialize(shaders,{64,64},true);gpu.fence_submissions=true;
    gpu.renderer->set_quality(1,1);
    const std::array materials{white_material()};auto scene=pt::build_scene({},materials);
    auto& mirror=scene.materials[0];make_mirror(mirror,.12f);
    mirror.base=mirror.glancing_tint={1,0,0,1};
    std::vector<Vertex> primary;quad(primary,-1,-1,1,1,.5f,{0,0,-1},0);
    const auto draw=[&](bool capture) {
        return gpu.draw(scene,primary,{1,1,1},Camera{},~0u,false,true,nullptr,true,capture,capture);
    };
    for(unsigned frame=0;frame<15;++frame)draw(false);
    const auto before=draw(true);
    mirror.base=mirror.glancing_tint={0,1,0,1};
    const auto changed=draw(true);const auto history=gpu.renderer->temporal_stats();
    std::cout<<"Path Tracing reflection tint history: {\"prior_red\":"<<mean(before,0)
        <<",\"changed_red\":"<<mean(changed,0)<<",\"changed_green\":"<<mean(changed,1)
        <<",\"diffuse_history_mean\":"<<history.banks[0].mean_count<<'}'<<std::endl;
    require(mean(before,0)>240 && mean(before,1)<3,"Reflection tint control lost its authored red response");
    require(mean(changed,0)<3 && mean(changed,1)>240,
        "Denoiser retained stale authored reflection tint at unchanged roughness");
    require(history.banks[0].mean_count>15.5,"Reflection tint animation reset independent diffuse history");
    require(gpu.validation_errors.load()==0,"Reflection tint fixture reported Vulkan errors");
    return 0;
}
