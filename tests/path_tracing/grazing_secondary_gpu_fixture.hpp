#pragma once

// The primary is a perfect planar mirror whose normal already faces the eye.
// Only the secondary roof's authored smooth normal crosses the view horizon;
// this isolates traceNormal from the primary raster-normal correction.
int grazing_secondary_gpu(const std::filesystem::path& shaders) {
    Fixture gpu;gpu.initialize(shaders);gpu.renderer->set_quality(1,1);
    const Vec3 geometric{0,0,1},roof_geometric{0,0,-1};
    const Vec3 shading{.1f,0,-std::sqrt(.99f)},direction{.8f,0,-.6f};
    auto black=white_material();black.diffuse=solid_texture({0,0,0,255});
    const std::array materials{white_material(),white_material(),black};
    pt::Light sun;sun.direction=direction;sun.color={1,1,1};
    std::vector<Vertex> primary;quad(primary,-40,-.8f,40,.8f,.5f,geometric,0);
    auto geometry=primary;quad(geometry,50,-2,150,2,2.5f,shading,1);
    const auto make_scene=[&](std::span<const Vertex> vertices) {
        auto scene=pt::build_scene(vertices,materials,std::span(&sun,1));
        // Zero roughness makes GGX's sampled half-vector exactly Ng, so the
        // continuation and its unit weight are deterministic. This is an
        // explicit fixture BRDF, not a change to material adaptation policy.
        scene.materials[0].base={1,1,1,scene.materials[0].base[3]};
        scene.materials[0].response={0,1,1,scene.materials[0].response[3]};
        return scene;
    };
    const float u=1000.f/std::sqrt(1000.f*1000.f+20.f*20.f),v=u*.02f;
    Camera camera;camera.matrix={0,v,u*.001f,0,1,0,0,0,0,u,-v*.001f,0,
        0,-u*.5f,.5f+v*.0005f,1};
    camera.eye={-1000,0,20.5f,1};camera.forward={u,0,-v,0};
    const float expected=(shading.x*direction.x+shading.z*direction.z)*255.f;
    require(-v<0 && shading.x*u+shading.z*v>0 && expected>170 && expected<175,
        "Secondary grazing control no longer separates geometric and shading view hemispheres");
    const auto candidate=gpu.draw(make_scene(geometry),primary,{},camera);
    auto flat_geometry=geometry;for(std::size_t n=primary.size();n<flat_geometry.size();++n)
        flat_geometry[n].normal=roof_geometric;
    const auto flat=gpu.draw(make_scene(flat_geometry),primary,{},camera);
    auto blocked_geometry=geometry;quad(blocked_geometry,50,-2,160,2,1.5f,geometric,2);
    const auto blocked=gpu.draw(make_scene(blocked_geometry),primary,{},camera);
    const float candidate_mean=mean(candidate,0),flat_mean=mean(flat,0),blocked_mean=mean(blocked,0);
    std::cout<<"Grazing secondary normal: {\"expected_authored_sun\":"<<expected
        <<",\"secondary_authored_normal\":"<<candidate_mean<<",\"secondary_geometric_normal\":"<<flat_mean
        <<",\"real_blocker\":"<<blocked_mean<<",\"primary_mirror_roughness\":0"
        <<",\"roof_geometric_dot_incoming\":"<<-v
        <<",\"roof_shading_dot_incoming\":"<<shading.x*u+shading.z*v
        <<",\"validation_errors\":"<<gpu.validation_errors.load()<<'}'<<std::endl;
    require(std::abs(flat_mean-153.f)<2,"Secondary grazing mirror/sun reference changed");
    require(std::abs(candidate_mean-expected)<2,
        "Secondary hit reversed an authored smooth normal on a geometrically front-facing face");
    require(blocked_mean<2,"Secondary grazing orientation leaked an actual opaque blocker");
    require(gpu.validation_errors.load()==0,"Secondary grazing fixture reported Vulkan errors");
    return 0;
}
