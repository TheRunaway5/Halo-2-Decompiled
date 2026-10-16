#include "halo2/renderer.hpp"
#include "halo2/source_lens_query_geometry_tu15.hpp"
#include "halo2/source_lens_query_tu15.hpp"
#include "halo2/source_lens_query_frame_tu15.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <cstdlib>

using namespace halo2;
namespace {
void check(bool value,const char *why){if(!value)throw FormatError(why);}
std::vector<std::byte> bytes(const std::string &hex) {
    std::vector<std::byte> out;for(unsigned n=0;n<hex.size();n+=2)
        out.push_back(std::byte(std::stoul(hex.substr(n,2),nullptr,16)));return out;
}
std::vector<char> file_bytes(const std::filesystem::path &path) {
    std::ifstream f(path,std::ios::binary);check(bool(f),"Missing query capture");
    return {std::istreambuf_iterator<char>(f),{}};
}
SourceLensQueryDraw original_quad(const nlohmann::json &fixture,const nlohmann::json &c,unsigned id) {
    SourceRuntimeMemory memory;
    for(const auto &index:c.at("initial_banks")) {
        const auto &b=fixture.at("banks").at(index.get<unsigned>());
        memory.mount(b.at("address"),bytes(b.at("hex")),SourceRuntimeMemory::Access::ReadWrite,"original query GPU fixture input");
    }
    SourcePhysicsWorldRuntimeAbi a{};a.gp=c.at("entry_gp").get<decltype(a.gp)>();a.eflags=0x202;
    SourcePhysicsWorldRuntimeServices services;
    services.required_owner=[&](auto &m,const auto &call,auto &state,const auto &observe) {
        check(source_lens_query_tu15_owns_entry(call.target),"Query fixture reached an unowned original body");
        source_run_lens_query_tu15(m,call.target,state,{0x27f,0x1f80},services,observe);
    };
    source_run_lens_query_tu15(memory,0x2e520,a,{0x27f,0x1f80},services);
    const auto end=Bytes(memory.view(0x2002000,4)).u32(0);
    check(end>=0x2003000 && end<=0x2003f00,"Original query FIFO exceeds retained input");
    check(a.gp[0]==c.at("sample_area").get<unsigned>(),"Original/native query area differs before GPU submission");
    return source_lens_query_geometry_tu15(memory.view(0x2003000,end-0x2003000),a.gp[0],id,1);
}
}
int main(int argc,char **argv)try {
    check(argc==3,"Supply hardware quad fixture and output directory");
    std::ifstream input(argv[1]);if(!input)return 77;
    const auto fixture=nlohmann::json::parse(input);
    std::array<SourceLensQueryDraw,3> queries;
    std::array<std::uint64_t,3> expected{};
    const auto &cases=fixture.at("cases");
    queries[0]=original_quad(fixture,cases[0],0);
    queries[1]=original_quad(fixture,cases[1],1);
    queries[2]=original_quad(fixture,cases[2],2);
    check(queries[0].has_quad && queries[1].has_quad && !queries[2].has_quad,
        "Expected original near/far/behind query cases");
    expected[0]=cases[0].at("sample_area");
    const std::filesystem::path output=argv[2];std::filesystem::create_directories(output);
    SDL_setenv("HALO2_DEPTH_DIAGNOSTICS","1",1);
    SDL_SetMainReady();check(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());
    auto *window=SDL_CreateWindow("Original lens depth queries",0,0,640,480,SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN);
    check(window,SDL_GetError());
    std::ostringstream diagnostics;auto *previous=std::cerr.rdbuf(diagnostics.rdbuf());
    nlohmann::json report;
    try {
        Mesh mesh;Material material;material.diffuse={1,1,11,{std::byte{32},std::byte{80},std::byte{128},std::byte{255}}};
        mesh.materials.push_back(material);
        // A full-view plane at world X4. Both windings remove cull-policy
        // ambiguity; the query uses retained world W depth on this real image.
        for(const auto p:std::array<Vec3,12>{{{4,-100,-100},{4,100,-100},{4,100,100},
            {4,-100,-100},{4,100,100},{4,-100,100},{4,100,100},{4,100,-100},{4,-100,-100},
            {4,-100,100},{4,100,100},{4,-100,-100}}})mesh.vertices.push_back({p,{1,1,1},{0,0},0});
        const auto *directory=std::getenv("HALO2_NATIVE_GPU_SHADER_DIR");
        auto renderer_owner=std::make_unique<Renderer>(window,mesh,true,std::span<const SourceShadowPlanMaterial>{},nullptr,
            std::function<void()>{},nullptr,directory?directory:std::filesystem::path{});
        auto &renderer=*renderer_owner;
        BspAtmosphere atmosphere;atmosphere.gradient={1,1,11,{std::byte{255},std::byte{255},std::byte{255},std::byte{255}}};
        renderer.scene_bsp_atmosphere(atmosphere);
        Camera camera;camera.field_of_view=70;BspFogSample fog;
        ModelView view;view.camera=camera;view.width=640;view.height=480;
        view.full_surface=true;view.vertices=mesh.vertices;view.atmosphere=fog;
        check(renderer.draw(camera,output/"baseline.ppm",{},false,{},{},{},std::span{&view,1}),"Baseline query scene was not submitted");
        view.lens_queries=queries;
        check(renderer.draw(camera,output/"queries.ppm",{},false,{},{},{},std::span{&view,1}),"Original query scene was not submitted");
        auto results=renderer.take_lens_query_results();
        check(results.size()==queries.size(),"Submitted original queries lack completed GPU results");
        report={{"samples",nlohmann::json::array()},{"gpu",renderer.device_name()}};
        for(const auto &result:results) {
            check(result.id<expected.size() && result.generation==1,"GPU query result lost its retained identity");
            check(result.samples==expected[result.id],"GPU query did not distinguish original visible, occluded and empty geometry");
            report["samples"].push_back({{"id",result.id},{"count",result.samples},{"timestamp",result.timestamp}});
        }
        check(file_bytes(output/"baseline.ppm")==file_bytes(output/"queries.ppm"),"Occlusion queries changed world color presentation");
        for(const auto *suffix:{".depth-d24-low24.bin",".stencil-u8.bin"})
            check(file_bytes(output/(std::string("baseline.ppm")+suffix))==file_bytes(output/(std::string("queries.ppm")+suffix)),
                "Occlusion queries changed actual world depth/stencil pixels");
        check(renderer.take_lens_query_results().empty(),"GPU query result was delivered twice");
        for(auto &q:queries)q.generation=2;
        check(renderer.draw(camera,output/"reuse.ppm",{},false,{},{},{},std::span{&view,1}),"Reused original query identities failed submission");
        results=renderer.take_lens_query_results();check(results.size()==queries.size(),"Reused query results are absent");
        for(const auto &result:results)check(result.generation==2 && result.samples==expected[result.id],"Reused query returned a stale result");
        // Every authored query definition and both full/split projected banks
        // use the same actual native rasterizer. With no world occluder, its
        // original bounded sample area must match precise GPU coverage.
        std::vector<SourceLensQueryDraw> all;std::vector<std::uint64_t> areas;
        for(unsigned n=0;n<cases.size();++n) {
            auto draw=original_quad(fixture,cases[n],n);draw.generation=3;
            all.push_back(draw);areas.push_back(cases[n].at("sample_area"));
        }
        view.vertices={};view.lens_queries=all;
        check(renderer.draw(camera,output/"authored-quads.ppm",{},false,{},{},{},std::span{&view,1}),"Authored query matrix was not submitted");
        results=renderer.take_lens_query_results();check(results.size()==all.size(),"Authored query matrix results are absent");
        for(const auto &result:results) {
            if(result.id>=areas.size() || result.generation!=3 || result.samples!=areas[result.id]) {
                std::cout<<"query="<<result.id<<" samples="<<result.samples<<" original_area="<<areas.at(result.id)<<'\n';
                throw FormatError("Authored original query area differs from native GPU coverage");
            }
        }
        report["authored_quad_cases"]=all.size();report["depth_stencil_unchanged"]=true;
        report["passed"]=true;report["full_goal_complete"]=false;
        report["boundary"]="Actual native world-depth GPU queries from original quads; live map scheduling, query smoothing and final flare rendering remain separate acceptance.";
        auto retired_query_owner=std::make_unique<SourceLensQueryFrameTu15>(renderer);
        renderer_owner.reset();retired_query_owner.reset();
        report["retired_renderer_owner_safe"]=true;
    } catch(...) {std::cerr.rdbuf(previous);std::ofstream(output/"validation.log")<<diagnostics.str();SDL_DestroyWindow(window);SDL_Quit();throw;}
    std::cerr.rdbuf(previous);std::ofstream(output/"validation.log")<<diagnostics.str();
    SDL_DestroyWindow(window);SDL_Quit();
    check(diagnostics.str().find("VUID-")==std::string::npos && diagnostics.str().find("Validation Error")==std::string::npos,
        "Original lens query GPU validation failed");
    std::ofstream(output/"report.json")<<report.dump(2)<<'\n';
    std::cout<<"PASS original lens quads: real visible/occluded/empty GPU samples, immutable presentation and generation reuse\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
