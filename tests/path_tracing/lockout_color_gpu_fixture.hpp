#pragma once
#include "halo2/texture.hpp"
#include "halo2/path_tracing/color_guide.hpp"
#include "halo2/path_tracing/emitter_sampling.hpp"
#include <nlohmann/json.hpp>
#include <fstream>

// This component view retains actual Lockout triangles, materials and source
// emission programs. A small orthographic view isolates an existing receiver;
// the comparison removes emission without changing geometry or base colors.
int actual_lockout_colors(const std::filesystem::path& shaders,const std::filesystem::path& map,
                         const std::filesystem::path& report_path) {
    using Json=nlohmann::json;
    if(!std::filesystem::is_regular_file(map))return 77;
    Cache cache(map);const auto mesh=cache.load_bsp(0);
    auto scene=pt::build_scene(mesh.vertices,mesh.materials);pt::refresh_color_guides(scene,mesh.lightmaps);
    auto dark=scene;for(auto& material:dark.materials)material.emission={};
    dark.emissive_count=0;dark.pixels.resize(dark.texture_texel_count);
    auto pure_bsdf_scene=scene;
    pure_bsdf_scene.pixels.resize(pure_bsdf_scene.texture_texel_count+pure_bsdf_scene.emissive_count);
    Fixture gpu;gpu.initialize(shaders,{64,64},true);gpu.fence_submissions=true;
    gpu.renderer->set_quality(1,1);
    auto paired_renderer=std::make_unique<pt::Render>(pt::RenderContext{gpu.device,gpu.physical,gpu.pass,gpu.material_layout,shaders});
    paired_renderer->set_quality(1,1);
    const auto subtract=[](Vec3 a,Vec3 b){return Vec3{a.x-b.x,a.y-b.y,a.z-b.z};};
    const auto add=[](Vec3 a,Vec3 b){return Vec3{a.x+b.x,a.y+b.y,a.z+b.z};};
    const auto multiply=[](Vec3 a,float b){return Vec3{a.x*b,a.y*b,a.z*b};};
    const auto dot=[](Vec3 a,Vec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;};
    const auto cross=[](Vec3 a,Vec3 b){return Vec3{a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};};
    const auto normalize=[&](Vec3 v){return multiply(v,1.f/std::sqrt(dot(v,v)));};
    Json report{{"map",map.string()},{"scope","Actual BSP receiver/material/emission transport; local emission isolated from sky and occupied native lights; no native gameplay acceptance"},
        {"upscaler","Off"},{"samples",1},{"bounces",1},{"emissive_triangles",scene.emissive_count},{"regions",Json::array()}};
    bool colors_valid=true,noise_valid=true;
    for(const auto first:std::array<unsigned,2>{20124,18645}) {
        require(first+2<mesh.vertices.size(),"Actual Lockout receiver source index is absent");
        const std::array<Vertex,3> primary{mesh.vertices[first],mesh.vertices[first+1],mesh.vertices[first+2]};
        const auto slot=primary[0].material;const auto& material=mesh.materials.at(slot);
        require(material.name.ends_with(first==20124?"\\composite_floor":"\\composite_block"),
            "Actual Lockout receiver source material changed");
        require(scene.materials[slot].emission==ShaderVector{},"Color receiver unexpectedly emits its own light");
        native_uniform(gpu,material);
        const Texture* textures[]{&material.diffuse,&material.detail,&material.bump,&material.auxiliary};
        const unsigned stages=(scene.materials[slot].texture[3]&pt::surface_flag)?4:1;
        for(unsigned stage=0;stage<stages;++stage) {
            if(!material.animated_textures[stage].empty())
                textures[stage]=&material.animated_textures[stage].at(material.texture_frames[stage]);
            auto image=*textures[stage];
            if(image.format==14)image=prepare_xemu_bc1_texture(image);
            else if(image.format==15)image=prepare_xemu_bc2_texture(image);
            else if(image.format==16)image=prepare_xemu_bc3_texture(image);
            require(image.format==11 && image.width && image.height && image.layers==1 && !image.depth,
                "Actual receiver texture is outside the native color fixture's RGBA8 input seam");
            std::vector<std::uint8_t> pixels(width*height*4);
            for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x) {
                const auto source=(std::size_t(y*image.height/height)*image.width+x*image.width/width)*4;
                for(unsigned channel=0;channel<4;++channel)
                    pixels[(y*width+x)*4+channel]=std::to_integer<unsigned char>(image.pixels[source+channel]);
            }
            gpu.primary_texture(pixels,stage);
        }
        const auto point=multiply(add(add(primary[0].position,primary[1].position),primary[2].position),1.f/3);
        const auto normal=normalize(add(add(primary[0].normal,primary[1].normal),primary[2].normal));
        const auto right=normalize(cross(std::abs(normal.z)<.999f?Vec3{0,0,1}:Vec3{0,1,0},normal));
        const auto up=cross(normal,right);constexpr float radius=.025f;
        Camera camera;camera.matrix={};camera.matrix[15]=1;
        const std::array<Vec3,3> axes{multiply(right,1/radius),multiply(up,1/radius),multiply(normal,-.25f)};
        for(unsigned row=0;row<3;++row) {
            camera.matrix[row]=axes[row].x;camera.matrix[4+row]=axes[row].y;camera.matrix[8+row]=axes[row].z;
            camera.matrix[12+row]=-dot(axes[row],point)+(row==2?.5f:0.f);
        }
        const auto eye=add(point,multiply(normal,2));camera.eye={eye.x,eye.y,eye.z,1};
        camera.forward={-normal.x,-normal.y,-normal.z,0};
        const auto draw=[&](const pt::Scene& source,bool filtered,bool capture=true) {
            return gpu.draw(source,primary,{},camera,~0u,false,filtered,nullptr,true,capture,capture);
        };
        const auto evaluate=[&](const pt::Scene& source) {
            const auto off=draw(dark,false),raw=draw(source,false),initial=draw(source,true);
            const auto initial_serial=gpu.renderer->temporal_stats().frame_serial;
            for(unsigned frame=1;frame<15;++frame)draw(source,true,false);
            const auto settled=draw(source,true);
            std::vector<double> reference(width*height*4);
            for(unsigned frame=0;frame<256;++frame) {
                const auto image=draw(source,true);
                for(std::size_t at=0;at<image.size();++at)reference[at]+=double(image[at])/256;
            }
            const auto metrics=[&](const auto& image) {
                std::array<double,3> average{},squared{};unsigned count{};
                for(unsigned y=8;y<56;++y)for(unsigned x=8;x<56;++x) {
                    ++count;const auto at=(std::size_t(y)*width+x)*4;
                    for(unsigned channel=0;channel<3;++channel) {
                        average[channel]+=image[at+channel];const auto difference=image[at+channel]-reference[at+channel];
                        squared[channel]+=difference*difference;
                    }
                }
                for(unsigned channel=0;channel<3;++channel){average[channel]/=count;squared[channel]=std::sqrt(squared[channel]/count);}
                return Json{{"mean",average},{"reference_rmse",squared}};
            };
            return Json{{"initial_frame_serial",initial_serial},{"emission_off",metrics(off)},
                {"raw",metrics(raw)},{"initial_filtered",metrics(initial)},{"settled16",metrics(settled)},
                {"reference256",metrics(reference)}};
        };
        auto receiver=evaluate(scene);
        std::swap(gpu.renderer,paired_renderer);receiver["pure_bsdf"]=evaluate(pure_bsdf_scene);
        std::swap(gpu.renderer,paired_renderer);
        require(receiver["initial_frame_serial"]==receiver["pure_bsdf"]["initial_frame_serial"],
            "Paired actual-source color controls used different lighting-frame seeds");
        receiver.update(Json{{"source_first_vertex",first},{"material",material.name},
            {"position",{point.x,point.y,point.z}},{"source_normal",{normal.x,normal.y,normal.z}},
            {"primary_material_flags",scene.materials[slot].texture[3]}});
        Json groups=Json::array();float maximum_score=0;unsigned selected=0;bool inside_any=false;
        const auto group_header=scene.texture_texel_count+scene.emissive_count;
        for(unsigned index=0;index<scene.pixels.at(group_header+1);++index) {
            const auto record=group_header+pt::emitter_group_header_words+index*pt::emitter_group_words;
            const auto value=[&](unsigned lane){return std::bit_cast<float>(scene.pixels.at(record+lane));};
            const Vec3 center{value(4),value(5),value(6)},power{value(8),value(9),value(10)};
            const auto delta=subtract(center,point);const float squared=dot(delta,delta),distance=std::sqrt(squared),sphere_radius=value(7);
            const float facing=distance>.0002f?std::clamp((dot(normal,delta)+sphere_radius)/distance,0.f,1.f):1.f;
            const float score=dot(power,{.2126f,.7152f,.0722f})*facing/std::max({squared,sphere_radius*sphere_radius,1.e-12f});
            const bool inside=squared<=sphere_radius*sphere_radius;inside_any=inside_any||inside;
            if(score>maximum_score){maximum_score=score;selected=index;}
            groups.push_back({{"index",index},{"material",mesh.materials.at(scene.pixels.at(record)).name},
                {"center",{center.x,center.y,center.z}},{"radius",sphere_radius},{"inside",inside},{"score",score}});
        }
        receiver["proposal_groups"]={{"selected_index",selected},{"inside_any",inside_any},{"groups",groups}};
        report["regions"].push_back(receiver);std::cout<<"Path Tracing actual Lockout color: "<<receiver.dump()<<std::endl;
        const auto source_mean=receiver["reference256"]["mean"].get<std::array<double,3>>();
        const auto initial_error=receiver["initial_filtered"]["reference_rmse"].get<std::array<double,3>>();
        const auto settled_error=receiver["settled16"]["reference_rmse"].get<std::array<double,3>>();
        const auto pure_initial_error=receiver["pure_bsdf"]["initial_filtered"]["reference_rmse"].get<std::array<double,3>>();
        const auto pure_settled_error=receiver["pure_bsdf"]["settled16"]["reference_rmse"].get<std::array<double,3>>();
        if(first==20124) {
            colors_valid=colors_valid && source_mean[2]>3 && source_mean[2]>source_mean[0]*1.25;
            // Baseline first-frame blue RMSE was3.85/255. The same1/1 query
            // budget must improve that actual receiver without losing its hue.
            noise_valid=noise_valid && initial_error[2]<2.5 && settled_error[2]<1.0;
        }else {
            colors_valid=colors_valid && source_mean[1]>30 && source_mean[1]>source_mean[0]*1.3 && source_mean[1]>source_mean[2]*2;
            // Compare identical current-shader seeds/source geometry. A
            // previously captured absolute limit used another classifier and
            // reference; the inside-sphere fallback must preserve this paired
            // BSDF control, or improve it, at both history lengths.
            noise_valid=noise_valid && initial_error[1]<=pure_initial_error[1]+1.e-6 &&
                settled_error[1]<=pure_settled_error[1]+1.e-6;
        }
    }
    report["validation_errors"]=gpu.validation_errors.load();report["source_colors_preserved"]=colors_valid;
    report["bounded_noise"]=noise_valid;report["passed"]=colors_valid && noise_valid && gpu.validation_errors.load()==0;
    std::ofstream output(report_path);require(bool(output),"Cannot write actual Lockout color report");output<<report.dump(2)<<'\n';
    require(colors_valid,"Actual Lockout source emission lost its blue/green receiver hue or energy");
    require(noise_valid,"Actual Lockout blue receiver retained excessive noise or green-room noise increased");
    require(gpu.validation_errors.load()==0,"Actual Lockout color fixture reported Vulkan errors");return 0;
}
