#pragma once
#include "halo2/path_tracing/color_guide.hpp"

// The raster receiver sees a constant, source-qualified lightmap only after a
// real closest-hit segment reaches the rear wall. An enabled black sky isolates
// this cached static field. A missing sky must preserve the same irradiance;
// explicit removal of the energy tag supplies emission-only physical controls.
// CPU tests qualify the actual native program. This fixture controls its GPU tag
// directly so it independently checks transport rather than copying admission.
int source_indirect_gpu(const std::filesystem::path& shaders) {
    Fixture gpu;gpu.initialize(shaders);gpu.renderer->set_quality(1,1);
    gpu.primary_color({64.f/255,64.f/255,64.f/255,1});
    std::vector<Vertex> primary;quad(primary,-1,-1,1,1,.5f,{0,0,-1},0);
    for(auto& vertex:primary){vertex.lightmap=2;vertex.lightmap_uv={.5f,.5f};}
    const std::array<Vec3,6> black_sky{};
    std::array<Vec3,6> colored_sky;colored_sky.fill({.2f,.4f,.6f});
    constexpr unsigned sampler=3u|(3u<<4)|(2u<<8);
    const auto make_scene=[&](unsigned primary_guide,unsigned wall_albedo=128,bool qualified=true,
                              bool wall=true,bool blocker=false,std::span<const pt::Light> lights={}) {
        std::array materials{white_material(),white_material(),white_material()};
        materials[0].diffuse=solid_texture({64,64,64,255});
        materials[1].diffuse=solid_texture({std::uint8_t(wall_albedo),std::uint8_t(wall_albedo),std::uint8_t(wall_albedo),255});
        materials[2].diffuse=solid_texture({0,0,0,255});
        std::vector<Vertex> geometry;
        // Every sampled hemisphere direction reaches this plane. Its huge
        // constant-color extent removes Monte Carlo direction variance from
        // the analytic irradiance/albedo controls below.
        if(wall)quad(geometry,-16384,-16384,16384,16384,-1,{0,0,1},1);
        if(blocker)quad(geometry,-16384,-16384,16384,16384,0,{0,0,1},2);
        auto scene=pt::build_scene(geometry,materials,lights);
        for(auto& material:scene.materials)material.response={1,0,0,material.response[3]};
        scene.materials[0].color_guide={1,sampler,0,2};
        scene.materials[1].color_guide={1,sampler,0,qualified?2u:0u};
        Vertex a;a.lightmap=1;a.lightmap_uv={.5f,.5f};
        for(auto& triangle:scene.triangles)if(triangle.metadata[0]==1)
            triangle.color_guide=pt::color_guide_triangle(a,a,a,scene.materials[1]);
        const std::array images{solid_texture({64,128,192,255}),
            solid_texture({std::uint8_t(primary_guide),std::uint8_t(primary_guide),std::uint8_t(primary_guide),255})};
        pt::refresh_color_guides(scene,images);return scene;
    };
    const auto draw=[&](const pt::Scene& scene,bool native_bank=true,const std::array<Vec3,6>* sky=nullptr) {
        return gpu.draw(scene,primary,{},Camera{},~0u,false,false,native_bank?(sky?sky:&black_sky):nullptr);
    };
    const auto expected=[&](unsigned channel,unsigned wall_albedo=128) {
        return 2.f*(64.f/255)*(float(wall_albedo)/255)*float((channel+1)*64);
    };
    auto scene=make_scene(32);
    // Exercise a fresh renderer before any sky/environment cube was enabled,
    // as well as the enabled-to-disabled transition checked below.
    const auto first_no_sky=draw(scene,false),cached=draw(scene);
    require(first_no_sky==cached,
        "No-sky context discarded authenticated secondary irradiance");
    for(unsigned channel=0;channel<3;++channel)
        require(std::abs(mean(cached,channel)-expected(channel))<1.1f,
            "Secondary source irradiance lost its authored2x RGB or current surface albedo");
    auto brighter_primary=make_scene(224);const auto primary_changed=draw(brighter_primary);
    require(primary_changed==cached,"Primary baked guide brightness became direct RT lighting");
    require(draw(scene,true,&colored_sky)==cached,
        "Cached static field double-counted secondary sky NEE");
    auto no_hit=make_scene(224,128,true,false);const auto absent=draw(no_hit);
    require(mean(absent,0)<1 && mean(absent,1)<1 && mean(absent,2)<1,
        "Source field lit a receiver without an actual secondary intersection");
    auto blocked_scene=make_scene(32,128,true,true,true);const auto blocked=draw(blocked_scene);
    require(mean(blocked,0)<1 && mean(blocked,1)<1 && mean(blocked,2)<1,
        "Secondary source field leaked through the current closest opaque blocker");
    auto darker_wall=make_scene(32,64);const auto albedo_changed=draw(darker_wall);
    for(unsigned channel=0;channel<3;++channel)
        require(std::abs(mean(albedo_changed,channel)-expected(channel,64))<1.1f,
            "Cached secondary lighting bypassed the hit surface's current albedo");
    const auto allocated=gpu.renderer->allocated_bytes();
    auto unqualified=make_scene(32,128,false);const auto hue_only=draw(unqualified);
    require(mean(hue_only,0)<1 && mean(hue_only,1)<1 && mean(hue_only,2)<1,
        "An unqualified hue-only guide supplied cached lighting energy");
    require(gpu.renderer->allocated_bytes()==allocated,
        "Source-energy qualification allocated another GPU resource");
    const auto sky_off=draw(scene,false);
    require(sky_off==cached,
        "No-sky context discarded authenticated secondary irradiance");
    gpu.renderer->set_quality(1,2);const auto two_bounces=draw(scene);
    require(two_bounces==cached,"A second bounce absorbed or duplicated the first compatible cached field");
    gpu.renderer->set_quality(1,1);

    // The authored distant source contributes no primary cosine here and must
    // not be added again at a secondary surface whose static field contains it.
    pt::Light sun;sun.direction=Vec3{0,0,1};sun.color={1,1,1};
    auto sun_scene=make_scene(32,128,true,true,false,std::span(&sun,1));
    const auto no_double_sun=draw(sun_scene);
    require(no_double_sun==cached,"Cached static field double-counted secondary distant-light NEE");

    // A distant local point has nearly constant cosine/range over this small
    // receiver. Its .25 radial attenuation adds about8 output levels while
    // its direction is behind the primary shading normal, isolating the hit.
    pt::Light local;local.position={0,0,1.e6f};local.radius=2.e6f;local.color={1,1,1};
    auto local_scene=make_scene(32,128,true,true,false,std::span(&local,1));
    const auto with_local=draw(local_scene);
    require(mean(with_local,0)>mean(cached,0)+5,
        "Cached static field suppressed an actual local light at its secondary hit");
    // Fallback emission retains its authored texture multiplier: .25 strength
    // times128/255 at the hit times64/255 at the primary is about8 levels.
    auto emitting_scene=scene;emitting_scene.materials[1].emission={.25f,.25f,.25f,0};
    const auto with_emission=draw(emitting_scene);
    require(draw(emitting_scene,false)==with_emission,
        "Missing sky changed an authenticated field plus its current emission");
    auto physical_emission_scene=emitting_scene;
    for(auto& material:physical_emission_scene.materials)material.color_guide[3]=0;
    const auto emission_only=draw(physical_emission_scene,false),physical_emission=draw(physical_emission_scene);
    require(emission_only==physical_emission && mean(emission_only,0)>7,
        "Explicit cache-disabled emission-only control changed with sky availability");
    for(unsigned channel=0;channel<3;++channel)
        require(std::abs(mean(with_emission,channel)-mean(cached,channel)-8.f)<1.1f,
            "Cached secondary lighting replaced directly visible authored emission");

    // Qualification affects only the diffuse lobe. A perfect GGX mirror still
    // continues to the actual sky at depth2, with no cached diffuse addition.
    auto mirror_scene=scene;mirror_scene.materials[1].response={0,1,1,0};
    mirror_scene.materials[1].base={1,1,1,mirror_scene.materials[1].base[3]};
    const auto mirror_one=draw(mirror_scene,true,&colored_sky);
    gpu.renderer->set_quality(1,2);const auto mirror_two=draw(mirror_scene,true,&colored_sky);
    require(mean(mirror_one,0)<1 && mean(mirror_two,0)>10 && mean(mirror_two,2)>mean(mirror_two,0)*2.5f,
        "Diffuse source-cache termination stopped a compatible surface's GGX continuation");
    // A weak emitter behind the cached wall cannot change its constant field.
    // It can still attract a proposal without an extra visibility query. The
    // primary's qualified irradiance is a sampling hint only: clearing that
    // one tag leaves exactly the same physical secondary field and blocker.
    gpu.renderer->set_quality(1,1);
    std::vector<Vertex> background_geometry;
    quad(background_geometry,-16384,-16384,16384,16384,-1,{0,0,1},1);
    for(auto& vertex:background_geometry){vertex.lightmap=1;vertex.lightmap_uv={.5f,.5f};}
    quad(background_geometry,-.25f,-.25f,.25f,.25f,-2,{0,0,1},2);
    std::array background_materials{white_material(),white_material(),white_material()};
    background_materials[1].diffuse=solid_texture({128,128,128,255});
    auto background_scene=pt::build_scene(background_geometry,background_materials);
    for(auto& material:background_scene.materials)material.response={1,0,0,material.response[3]};
    for(unsigned slot:{0u,1u})background_scene.materials[slot].color_guide={1,sampler,0,2};
    const std::array background_images{solid_texture({64,128,192,255}),solid_texture({128,128,128,255})};
    pt::refresh_color_guides(background_scene,background_images);
    background_scene.materials[2].emission={.001f,.001f,.001f,0};
    pt::update_geometry(background_scene,background_geometry);
    require(background_scene.emissive_count==2,"Background sampling control lacks its actual hidden emitter");
    const auto background=draw(background_scene,false);
    auto no_hint_scene=background_scene;no_hint_scene.materials[0].color_guide[3]=0;
    const auto no_hint=draw(no_hint_scene,false);
    const float background_noise=deviation(background,2),no_hint_noise=deviation(no_hint,2);
    require(no_hint_noise>10,"Background control did not establish nonzero proposal variance");
    require(background_noise<no_hint_noise*.2f,
        "Authored static-field sampling hint failed to reduce background noise");
    require(std::abs(mean(background,2,8,56,8,56)-expected(2))<1.1f &&
        std::abs(mean(no_hint,2,8,56,8,56)-expected(2))<4.f*no_hint_noise/48.f+1.f,
        "Proposal-only irradiance hint changed constant-field lighting energy");
    require(gpu.validation_errors.load()==0,"Secondary source irradiance fixture reported Vulkan errors");
    std::cout<<"Secondary source irradiance: {\"cached_rgb\":["<<mean(cached,0)<<','<<mean(cached,1)<<','<<mean(cached,2)
        <<"],\"no_sky_rgb\":["<<mean(sky_off,0)<<','<<mean(sky_off,1)<<','<<mean(sky_off,2)
        <<"],\"first_no_sky\":"<<mean(first_no_sky,0)
        <<",\"blocked\":"<<mean(blocked,0)<<",\"two_bounces\":"<<mean(two_bounces,0)
        <<",\"local\":"<<mean(with_local,0)<<",\"emission\":"<<mean(with_emission,0)
        <<",\"GGX_depth2\":"<<mean(mirror_two,0)<<",\"background_noise\":"<<background_noise
        <<",\"no_hint_noise\":"<<no_hint_noise<<",\"validation_errors\":0}"<<std::endl;
    return 0;
}
