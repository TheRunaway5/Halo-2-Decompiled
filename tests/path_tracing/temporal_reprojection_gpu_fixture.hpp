#pragma once
#include <limits>

namespace temporal_reprojection_detail {
Camera perspective_camera(Vec3 origin,VkExtent2D extent,unsigned frame,bool moving) {
    const float yaw=moving?float(frame)*.0006f:0.f;
    const float s=std::sin(yaw),c=std::cos(yaw),sx=float(extent.height)/extent.width;
    const Vec3 eye{origin.x+(moving?float(frame)*.2f:0.f),origin.y,origin.z-96.f};
    constexpr float near_plane=16.f,far_plane=1024.f;
    constexpr float a=far_plane/(far_plane-near_plane),b=near_plane*a;
    Camera camera;camera.matrix={sx*c,0,a*s,s,0,1,0,0,-sx*s,0,a*c,c,
        sx*(-c*eye.x+s*eye.z),-eye.y,-a*(s*eye.x+c*eye.z)-b,-(s*eye.x+c*eye.z)};
    camera.eye={eye.x,eye.y,eye.z,1};camera.forward={s,0,c,0};return camera;
}
VkViewport viewport(VkExtent2D extent,unsigned frame,bool jitter) {
    // Neither offset changes the camera or scene. Alternating this pair
    // reproduces a stationary temporal upscaler's subpixel raster motion.
    const float x=jitter?(frame%2?.375f:-.375f):0.f;
    const float y=jitter?(frame%2?-.25f:.25f):0.f;
    return {x,y,float(extent.width),float(extent.height),0,1};
}
std::vector<std::uint8_t> textured_border() {
    std::vector<std::uint8_t> pixels(width*height*4);
    for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x) {
        // The measurement interior has known constant albedo26/255, so its
        // variance measures lighting noise rather than authored texture.
        const bool border=x<8 || x>=width-8 || y<8 || y>=height-8;
        const auto value=std::uint8_t(border?(((x/4+y/4)%2)?18:34):26);
        const auto at=(std::size_t(y)*width+x)*4;
        pixels[at]=pixels[at+1]=pixels[at+2]=value;pixels[at+3]=255;
    }
    return pixels;
}
Camera foreground_camera(Vec3 origin,VkExtent2D extent,unsigned frame) {
    auto camera=perspective_camera(origin,extent,frame,true);
    for(unsigned column=0;column<4;++column) {
        camera.matrix[column*4]+=.35f*camera.matrix[column*4+3];
        camera.matrix[column*4+1]-=.22f*camera.matrix[column*4+3];
    }
    // A common title FP matrix scale preserves perspective division.
    for(auto& value:camera.matrix)value*=.15f;
    camera.eye[3]=2;return camera;
}
std::vector<Vertex> foreground_vertices(Vec3 origin,unsigned frame,float depth=32.f) {
    const float yaw=float(frame)*.0006f,s=std::sin(yaw),c=std::cos(yaw);
    const Vec3 eye{origin.x+float(frame)*.2f,origin.y,origin.z-96.f};
    std::vector<Vertex> vertices;quad(vertices,-96,-48,96,48,depth,{.6f,0,-.8f},2);
    for(auto& vertex:vertices) {
        const auto p=vertex.position,n=vertex.normal;
        vertex.position={eye.x+c*p.x+s*p.z,eye.y+p.y,eye.z-s*p.x+c*p.z};
        vertex.normal={c*n.x+s*n.z,n.y,-s*n.x+c*n.z};
    }
    return vertices;
}
}

int temporal_reprojection_gpu(const std::filesystem::path& shaders,bool moving,VkExtent2D extent={512,288}) {
    using namespace temporal_reprojection_detail;
    Fixture gpu;gpu.initialize(shaders,extent,true);gpu.fence_submissions=true;
    gpu.renderer->set_quality(1,1);gpu.primary_texture(textured_border());
    auto black_emitter=white_material();black_emitter.diffuse=solid_texture({0,0,0,255});
    black_emitter.forward_vertex=2;black_emitter.forward_diffuse={1,1,1,1};
    const std::array materials{white_material(),black_emitter};
    const Vec3 origin{74.2382f,-117.899f,.21363f};
    std::vector<Vertex> primary;quad(primary,-256,-144,256,144,32,{0,0,-1},0);
    for(auto& vertex:primary)vertex.position={origin.x+vertex.position.x,origin.y+vertex.position.y,origin.z+vertex.position.z};
    auto geometry=primary;quad(geometry,origin.x+700,origin.y+700,origin.x+764,origin.y+764,origin.z-300,{0,0,1},1);
    pt::Light sun;sun.direction=Vec3{0,0,-1};sun.color={2,2,2};
    std::array<Vec3,6> sky;sky.fill({.08f,.08f,.08f});
    sky[0]={.9f,.9f,.9f};sky[3]={.7f,.7f,.7f};sky[5]={.2f,.2f,.2f};
    auto scene=pt::build_scene(geometry,materials,std::span(&sun,1));
    const auto base_camera=perspective_camera(origin,extent,0,moving);
    const auto raw=noise_metrics(gpu.draw(scene,primary,{},base_camera,~0u,false,false,&sky),extent);
    const auto render=[&](unsigned frame,bool capture) {
        const auto camera=perspective_camera(origin,extent,frame,moving);
        const auto actual=viewport(extent,frame,!moving);
        return gpu.draw(scene,primary,{},camera,~0u,false,true,&sky,true,capture,capture,&actual);
    };
    const auto first=noise_metrics(render(0,true),extent);
    const auto bytes=gpu.renderer->allocated_bytes();
    for(unsigned frame=1;frame<15;++frame)render(frame,false);
    const auto final=noise_metrics(render(15,true),extent);
    const auto stable_history=gpu.renderer->temporal_stats();
    // Keep this same Scene owner/material/normal, and move the actual surface
    // through depth. Reprojection must reject its former plane, rather than
    // accumulating an unrelated stationary receiver at the previous pixel.
    for(auto& vertex:primary)vertex.position.z+=16.f;
    for(std::size_t n=0;n<primary.size();++n)geometry[n]=primary[n];
    pt::update_geometry(scene,geometry,std::span(&sun,1));
    const auto moved=noise_metrics(render(15,true),extent);
    const auto moved_history=gpu.renderer->temporal_stats();
    std::cout<<"Path Tracing temporal reprojection: {\"mode\":\""<<(moving?"camera":"jitter")
        <<"\",\"width\":"<<extent.width<<",\"height\":"<<extent.height
        <<",\"frames\":16,\"upscaler\":\"Off\",\"perspective\":true,\"textured_border\":true"
        <<",\"jitter_x_pixels\":"<<(moving?0:.375)<<",\"jitter_y_pixels\":"<<(moving?0:.25)
        <<",\"camera_translation_per_frame\":"<<(moving?.2:0)<<",\"camera_yaw_per_frame\":"<<(moving?.0006:0)<<',';
    print_noise_metrics("raw",raw);std::cout<<',';print_noise_metrics("first",first);std::cout<<',';
    print_noise_metrics("sixteenth",final);std::cout<<',';print_noise_metrics("moved_receiver_first",moved);
    std::cout<<",\"stable_history_mean\":"<<stable_history.banks[0].mean_count
        <<",\"stable_history_max\":"<<stable_history.banks[0].maximum_count
        <<",\"moved_receiver_history_mean\":"<<moved_history.banks[0].mean_count
        <<",\"moved_receiver_history_max\":"<<moved_history.banks[0].maximum_count
        <<",\"validation_errors\":"<<gpu.validation_errors.load()<<'}'<<std::endl;
    require(raw.mean>52 && raw.deviation>2 && raw.high_frequency>2,
        "Reprojection control did not establish actual stochastic sky lighting above the deterministic sun");
    require(std::abs(final.mean-raw.mean)<3 && std::abs(moved.mean-raw.mean)<3,
        "Reprojection changed the known static receiver lighting energy");
    require(final.deviation<raw.deviation*.2 && final.high_frequency<raw.high_frequency*.2 &&
            final.deviation<first.deviation*.65+.25,
        "Jitter or camera motion prevented actual temporal lighting-noise convergence");
    require(stable_history.banks[0].mean_count>(moving?8.:15.5) && stable_history.banks[0].maximum_count==16,
        "Jitter or camera motion discarded valid static WORLD lighting history");
    require(moved_history.banks[0].sampled_valid_count>16 && moved_history.banks[0].mean_count<1.1 &&
            moved_history.banks[0].maximum_count==1,
        "Reprojection reused history belonging to the moved receiver's old plane");
    require(gpu.renderer->allocated_bytes()==bytes,"Reprojection history allocation grew across fixed-size frames");
    require(gpu.validation_errors.load()==0,"Reprojection fixture reported Vulkan errors");
    return 0;
}

int foreground_reprojection_gpu(const std::filesystem::path& shaders,VkExtent2D extent={512,288},bool trace=false) {
    using namespace temporal_reprojection_detail;
    Fixture gpu;gpu.initialize(shaders,extent,true);gpu.fence_submissions=true;
    gpu.renderer->set_quality(1,1);gpu.primary_texture(textured_border());
    auto black_emitter=white_material();black_emitter.diffuse=solid_texture({0,0,0,255});
    black_emitter.forward_vertex=2;black_emitter.forward_diffuse={1,1,1,1};
    const std::array materials{white_material(),black_emitter,white_material()};
    const Vec3 origin{74.2382f,-117.899f,.21363f};
    std::vector<Vertex> world;quad(world,-256,-144,256,144,32,{0,0,-1},0);
    for(auto& vertex:world)vertex.position={origin.x+vertex.position.x,origin.y+vertex.position.y,origin.z+vertex.position.z};
    auto geometry=world;quad(geometry,origin.x+700,origin.y+700,origin.x+764,origin.y+764,origin.z-300,{0,0,1},1);
    pt::Light sun;sun.direction=Vec3{0,0,-1};sun.color={2,2,2};
    std::array<Vec3,6> sky;sky.fill({.08f,.08f,.08f});
    sky[0]={.9f,.9f,.9f};sky[3]={.7f,.7f,.7f};sky[5]={.2f,.2f,.2f};
    const auto scene=pt::build_scene(geometry,materials,std::span(&sun,1));
    const auto initial_vertices=foreground_vertices(origin,0);
    const auto initial_camera=foreground_camera(origin,extent,0);
    const auto raw=noise_metrics(gpu.draw(scene,initial_vertices,{},initial_camera,~0u,false,false,&sky),extent);
    NoiseMetrics world_value;
    float minimum_clip_depth=std::numeric_limits<float>::infinity();
    const auto render=[&](unsigned frame,float depth,bool capture) {
        const auto actual=viewport(extent,frame,true);
        const auto world_camera=perspective_camera(origin,extent,frame,true);
        const auto world_frame=gpu.draw(scene,world,{},world_camera,~0u,false,true,&sky,true,capture,capture,&actual);
        if(capture)world_value=noise_metrics(world_frame,extent);
        const auto camera=foreground_camera(origin,extent,frame);
        const auto primary=foreground_vertices(origin,frame,depth);
        for(const auto& vertex:primary) {
            const auto p=vertex.position;const auto& m=camera.matrix;
            const float z=((m[2]*p.x+m[6]*p.y)+m[10]*p.z)+m[14];
            const float w=((m[3]*p.x+m[7]*p.y)+m[11]*p.z)+m[15];
            minimum_clip_depth=std::min(minimum_clip_depth,z);
            require(z>0 && w>z,"Foreground fixture crosses its perspective near/far clip plane");
        }
        // Both roles share one upload/frame serial and independent banks.
        auto output=gpu.draw(scene,primary,{},camera,~0u,false,true,&sky,false,capture || trace,capture,&actual);
        if(trace) {
            const auto history=gpu.renderer->temporal_stats();
            std::cout<<"Foreground frame: {\"frame\":"<<frame<<",\"depth\":"<<depth
                <<",\"serial\":"<<history.frame_serial<<",\"world\":"<<history.banks[0].mean_count
                <<",\"foreground\":"<<history.banks[1].mean_count<<",\"foreground_max\":"<<history.banks[1].maximum_count
                <<",\"response\":["<<scene.materials[2].response[0]<<','<<scene.materials[2].response[1]<<','
                <<scene.materials[2].response[2]<<"]}"<<std::endl;
        }
        return output;
    };
    const auto first=noise_metrics(render(0,32,true),extent);
    const auto bytes=gpu.renderer->allocated_bytes();
    for(unsigned frame=1;frame<15;++frame)render(frame,32,false);
    const auto final=noise_metrics(render(15,32,true),extent);
    const auto stable_world=world_value;
    const auto stable=gpu.renderer->temporal_stats();
    const auto moved=noise_metrics(render(15,34,true),extent);
    const auto unchanged_world=world_value;
    const auto changed=gpu.renderer->temporal_stats();
    std::cout<<"Path Tracing foreground reprojection: {\"width\":"<<extent.width<<",\"height\":"<<extent.height
        <<",\"frames\":16,\"camera_attached_primary\":true,\"asymmetric_lens\":[0.35,-0.22]"
        <<",\"common_fp_mvp_scale\":0.15,\"foreground_depth_change\":2,\"minimum_clip_depth\":"<<minimum_clip_depth<<',';
    print_noise_metrics("raw",raw);std::cout<<',';print_noise_metrics("first",first);std::cout<<',';
    print_noise_metrics("sixteenth",final);std::cout<<',';print_noise_metrics("moved_foreground_first",moved);
    std::cout<<',';print_noise_metrics("world_before_foreground_pose",stable_world);
    std::cout<<',';print_noise_metrics("world_after_foreground_pose",unchanged_world);
    std::cout<<",\"world_history_mean\":"<<stable.banks[0].mean_count
        <<",\"foreground_history_mean\":"<<stable.banks[1].mean_count
        <<",\"foreground_history_max\":"<<stable.banks[1].maximum_count
        <<",\"after_pose_world_history_mean\":"<<changed.banks[0].mean_count
        <<",\"after_pose_foreground_history_mean\":"<<changed.banks[1].mean_count
        <<",\"after_pose_foreground_history_max\":"<<changed.banks[1].maximum_count
        <<",\"validation_errors\":"<<gpu.validation_errors.load()<<'}'<<std::endl;
    require(raw.mean>35 && raw.deviation>2 && raw.high_frequency>2,
        "Foreground reprojection control did not establish actual stochastic lighting");
    require(std::abs(final.mean-raw.mean)<3 && std::abs(moved.mean-raw.mean)<3,
        "Foreground reprojection changed receiver lighting energy");
    require(final.deviation<raw.deviation*.2 && final.high_frequency<raw.high_frequency*.2 &&
            final.deviation<first.deviation*.65+.25,
        "Camera-attached foreground lighting failed to converge under an asymmetric projection");
    require(stable.banks[0].mean_count>8 && stable.banks[0].maximum_count==16 &&
            stable.banks[1].mean_count>15.5 && stable.banks[1].maximum_count==16,
        "Camera-attached foreground or WORLD history was discarded during rigid camera motion");
    require(changed.banks[0].mean_count>8 && changed.banks[0].maximum_count==16 &&
            changed.banks[1].sampled_valid_count>16 && changed.banks[1].mean_count<1.1 &&
            changed.banks[1].maximum_count==1,
        "Foreground pose reused its former depth history or invalidated the independent WORLD bank");
    require(std::abs(stable_world.mean-unchanged_world.mean)<1,
        "Foreground-only pose changed the independent WORLD lighting");
    require(gpu.renderer->allocated_bytes()==bytes,"Foreground reprojection history grew across fixed-size frames");
    require(gpu.validation_errors.load()==0,"Foreground reprojection fixture reported Vulkan errors");
    return 0;
}
