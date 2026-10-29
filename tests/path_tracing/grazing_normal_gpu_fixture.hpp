#pragma once

// A smooth normal can point just past the view horizon while its actual face
// still faces the camera. Lambertian direct lighting depends on the authored
// normal and sun, not on which side of that shading-normal horizon is viewed.
int grazing_normal_gpu(const std::filesystem::path& shaders) {
    Fixture gpu;gpu.initialize(shaders);gpu.renderer->set_quality(1,1);
    const std::array materials{white_material()};
    const Vec3 shading{.1f,0,std::sqrt(.99f)},geometric{0,0,1};
    const Vec3 direction{.8f,0,.6f};pt::Light sun;sun.direction=direction;sun.color={1,1,1};
    auto scene=pt::build_scene({},materials,std::span(&sun,1));
    std::vector<Vertex> primary;quad(primary,-40,-.8f,40,.8f,.5f,shading,0);
    // This is the orthographic projection of eye(-1000,0,20.5) toward
    // center(0,0,.5): right=(0,1,0), up=(v,0,u), forward=(u,0,-v).
    const float u=1000.f/std::sqrt(1000.f*1000.f+20.f*20.f),v=u*.02f;
    Camera grazing;grazing.matrix={0,v,u*.001f,0,1,0,0,0,0,u,-v*.001f,0,
        0,-u*.5f,.5f+v*.0005f,1};
    grazing.eye={-1000,0,20.5f,1};grazing.forward={u,0,-v,0};
    require(-v<0 && shading.x*u-shading.z*v>0,
        "Grazing-normal control no longer separates geometric and shading view hemispheres");
    const float expected=(shading.x*direction.x+shading.z*direction.z)*255.f;
    require(expected>170 && expected<175,"Grazing-normal authored sun control changed");
    const auto candidate=gpu.draw(scene,primary,{},grazing);
    auto geometric_primary=primary;for(auto& vertex:geometric_primary)vertex.normal=geometric;
    const auto flat=gpu.draw(scene,geometric_primary,{},grazing);
    Camera ordinary;ordinary.matrix={.02f,0,0,0,0,1,0,0,0,0,-.001f,0,0,0,.5005f,1};
    ordinary.eye={0,0,20.5f,1};ordinary.forward={0,0,-1,0};
    const auto frontal=gpu.draw(scene,primary,{},ordinary);
    auto blocked_geometry=primary;
    quad(blocked_geometry,-41,-.85f,41,.85f,.55f,geometric,0);
    const auto blocked=gpu.draw(pt::build_scene(blocked_geometry,materials,std::span(&sun,1)),primary,{},grazing);
    const float candidate_mean=mean(candidate,0),flat_mean=mean(flat,0),frontal_mean=mean(frontal,0),blocked_mean=mean(blocked,0);
    std::cout<<"Grazing source normal: {\"expected_authored_sun\":"<<expected
        <<",\"grazing_authored_normal\":"<<candidate_mean<<",\"ordinary_authored_normal\":"<<frontal_mean
        <<",\"grazing_geometric_normal\":"<<flat_mean<<",\"real_blocker\":"<<blocked_mean
        <<",\"geometric_dot_incoming\":"<<-v
        <<",\"shading_dot_incoming\":"<<shading.x*u-shading.z*v
        <<",\"validation_errors\":"<<gpu.validation_errors.load()<<'}'<<std::endl;
    require(std::abs(flat_mean-153.f)<2 && std::abs(frontal_mean-expected)<2,
        "Grazing-normal geometric or ordinary-view sun reference changed");
    // The former independent Ns/view flip makes this exactly black despite
    // Ng facing the eye and the authored Ns/sun cosine being about0.677.
    require(std::abs(candidate_mean-expected)<2 && std::abs(candidate_mean-frontal_mean)<2,
        "Grazing view reversed an authored smooth normal on a geometrically front-facing face");
    require(blocked_mean<2,"Grazing-normal orientation leaked an actual opaque sun blocker");
    require(gpu.validation_errors.load()==0,"Grazing-normal fixture reported Vulkan errors");
    return 0;
}
