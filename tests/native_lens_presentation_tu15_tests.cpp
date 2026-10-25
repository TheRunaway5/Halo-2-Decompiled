#include "halo2/native_lens_presentation_tu15.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/source_frustum_camera.hpp"
#include "halo2/original_data.hpp"
#include "halo2/renderer.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <nlohmann/json.hpp>

using namespace halo2;
namespace {void check(bool v,const char *why){if(!v)throw FormatError(why);}}
int main(int argc,char **argv)try {
    check(argc==3,"Supply actual MCC map and owned output report");
    if(!std::filesystem::is_regular_file(argv[1]))return 77;
    check(std::getenv("HALO2_TEST_SCRATCH"),"Native lens GPU validation requires its scratch owner");
    Cache cache(argv[1],std::filesystem::path(argv[1]).parent_path(),OriginalData::native_tu15());
    std::uint32_t lens=0xffffffffu,projectile=0xffffffffu;
    for(const auto &tag:cache.tags())if(tag.group=="proj"&&tag.address&&tag.name.find("banshee")!=std::string::npos) {
        const auto metadata=cache.load_source_tag_metadata(tag.datum);
        const Bytes root(metadata.bytes_at(metadata.source_address(),metadata.size()));
        const auto count=root.u32(0x94),first=root.u32(0x98);
        const Bytes rows(cache.read_tag_bytes(first,std::size_t(count)*24));
        for(unsigned n=0;n<count;++n)if(rows.u32(n*24)==0x6c656e73u&&rows.u32(n*24+4)!=0xffffffffu) {
            check(!rows.u32(n*24+8)&&!rows.u16(n*24+12),"Banshee lens requires its separate marker/color producer");
            projectile=tag.datum;lens=rows.u32(n*24+4);break;
        }
        if(lens!=0xffffffffu)break;
    }
    check(lens!=0xffffffffu,"Actual Banshee projectile lacks its authored lens attachment");
    std::uint32_t effect_lens=0xffffffffu;
    for(const auto &tag:cache.tags())if(tag.group=="effe"&&tag.address&&
        tag.name=="effects\\impact\\explosion_large\\rocket_launcher\\rocket_detonation") {
        const auto metadata=cache.load_source_tag_metadata(tag.datum);
        const Bytes root(metadata.bytes_at(metadata.source_address(),metadata.size()));
        const Bytes events(cache.read_tag_bytes(root.u32(0x18),std::size_t(root.u32(0x14))*0x38));
        for(unsigned e=0;e<root.u32(0x14);++e) {
            const auto count=events.u32(e*0x38+0x18);if(!count)continue;
            const Bytes parts(cache.read_tag_bytes(events.u32(e*0x38+0x1c),std::size_t(count)*0x38));
            for(unsigned p=0;p<count;++p)if(parts.u32(p*0x38+8)==0x6c656e73u)effect_lens=parts.u32(p*0x38+16);
        }
    }
    check(effect_lens!=0xffffffffu,"Actual rocket detonation lacks its authored effect-part lens");
    SDL_SetMainReady();check(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());
    auto *window=SDL_CreateWindow("Native replica lens owner",0,0,640,480,SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN);
    check(window,SDL_GetError());
    nlohmann::json report{{"projectile",projectile},{"lens",lens},{"effect_lens",effect_lens},{"physical_world_constructed",false},{"GPU_completion_supplied",false},{"frames",nlohmann::json::array()}};
    std::ostringstream diagnostics;auto *previous=std::cerr.rdbuf(diagnostics.rdbuf());
    try {
        Mesh mesh;mesh.materials.emplace_back();mesh.materials.back().diffuse={1,1,11,{std::byte{16},std::byte{32},std::byte{64},std::byte{255}}};
        for(const auto p:std::array<Vec3,12>{{{2,-100,-100},{2,100,-100},{2,100,100},
            {2,-100,-100},{2,100,100},{2,-100,100},{2,100,100},{2,100,-100},{2,-100,-100},
            {2,-100,100},{2,100,100},{2,-100,-100}}})mesh.vertices.push_back({p,{1,1,1},{0,0},0});
        SourceLensDrawFrameTu15 resident(cache,mesh);
        const auto material_count=mesh.materials.size();
        NativeLensPresentationTu15 owner(cache,resident);
        check(mesh.materials.size()==material_count,"Replica lens owner duplicated its resident bitmap/material bank");
        const auto *directory=std::getenv("HALO2_NATIVE_GPU_SHADER_DIR");
        Renderer renderer(window,mesh,true,{},nullptr,{},nullptr,directory?directory:std::filesystem::path{});
        resident.bind(renderer);owner.bind(renderer);
        Camera camera;camera.field_of_view=.75f*57.295779513f;
        BspAtmosphere atmosphere;atmosphere.gradient={1,1,11,{std::byte{255},std::byte{255},std::byte{255},std::byte{255}}};renderer.scene_bsp_atmosphere(atmosphere);
        SourceFrustumProjectionInput input;input.projection_angle=.75f;input.outer=input.safe={0,0,480,640};input.scenario_present=true;input.scenario_type=1;
        const auto projection=source_view_projection({{0,0,0},{1,0,0},{0,0,1},.015625f,1024.f,{},source_frustum_projection(input).projection_program});
        ModelView view;view.camera=camera;view.width=640;view.height=480;view.full_surface=true;view.atmosphere=BspFogSample{};
        unsigned queries{},flares{},visible_results{},occluded_results{},late_occluded_alpha{};
        for(unsigned frame=0;frame<72;++frame) {
            owner.begin(1);owner.prepare_view(0,{0,0,0},{1,0,0},{0,0,1},.75f,projection,{0,0,480,640},640,480);
            for(const auto &result:owner.completed_queries()) {
                if(frame<24&&result.samples)++visible_results;
                if(frame>=56&&!result.samples)++occluded_results;
            }
            // Declared presentation caller fixture. This does not authenticate
            // any local effect allocation or replace an original admission.
            const SourceEffectReplicaRenderTu15 source{0xe672000du,0xffffffffu,0xffffffffu,1,{1,1,1},{}};
            owner.unattached_effect(source,effect_lens,0,{3,0,0},{1,0,0},1);
            owner.free_projectile(0x80000007u,lens,0,{3,0,0},{1,0,0},1,1,{1,1,1});
            owner.standalone_light({0xe9810000u,lens,{3,0,0},{1,0,0},{.25f,.5f,1.5f},.75f,.5f});
            const auto q=owner.queries();const auto d=owner.draws();queries+=q.size();flares+=d.size();
            if(frame>=56)for(const auto &draw:d)for(const auto &v:draw.vertices)late_occluded_alpha+=(v.rgba>>24)!=0;
            owner.finish_view();view.lens_queries=q;view.lens_draws=d;
            view.vertices=frame>=24?std::span<const Vertex>(mesh.vertices):std::span<const Vertex>{};
            const auto submitted=renderer.draw(camera,{}, {},false,{},{},{},std::span{&view,1});
            owner.finish(submitted);check(submitted,"Actual native lens GPU view was declined");
            report["frames"].push_back({{"frame",frame},{"queries",q.size()},{"flares",d.size()}});
        }
        check(queries>0&&flares>0&&visible_results>0,"Native lens owner failed to produce actual query-completed flares");
        check(occluded_results>0&&!late_occluded_alpha,"Native lens did not retire visibility behind actual GPU world depth");
        owner.suspend();
        report["passed"]=true;report["queries"]=queries;report["flares"]=flares;report["gpu"]=renderer.device_name();
        report["positive_visible_GPU_completions"]=visible_results;report["zero_occluded_GPU_completions"]=occluded_results;report["late_occluded_alpha_vertices"]=late_occluded_alpha;
    }catch(...){std::cerr.rdbuf(previous);std::cerr<<diagnostics.str();SDL_DestroyWindow(window);SDL_Quit();throw;}
    std::cerr.rdbuf(previous);SDL_DestroyWindow(window);SDL_Quit();
    check(diagnostics.str().find("VUID-")==std::string::npos&&diagnostics.str().find("Validation Error")==std::string::npos,"Native lens Vulkan validation failed");
    report["Vulkan_validation_errors"]=false;std::ofstream(argv[2])<<report.dump(2)<<'\n';
    std::cout<<"PASS actual Banshee lens request/query/draw with native replica presentation storage\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
