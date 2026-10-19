#include "halo2/renderer.hpp"
#include "halo2/source_lens_query_geometry_tu15.hpp"
#include "halo2/source_lens_query_tu15.hpp"
#include "halo2/source_lens_query_frame_tu15.hpp"
#include "halo2/source_runtime_memory.hpp"
#ifdef HALO2_VULKAN_LENS_RETIREMENT_OBSERVER
#include "helpers/vulkan_lens_retirement_observer.hpp"
#endif
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <thread>
#include <sstream>
#include <cstdlib>
#include <optional>

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
#ifdef SDL_HINT_SHUTDOWN_DBUS_ON_QUIT
    // This isolated test destroys all renderer/driver owners before SDL_Quit.
    // Ask SDL to release DBus debug allocations too; production keeps its
    // default because other live application libraries can still use DBus.
    SDL_SetHintWithPriority(SDL_HINT_SHUTDOWN_DBUS_ON_QUIT,"1",SDL_HINT_OVERRIDE);
#endif
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
        {
            // The authored-area matrix above intentionally removed the world
            // occluder. Restore it for the near/far/empty handoff cases.
            view.vertices=mesh.vertices;
            SourceRuntimeMemory incoming,bad;
            std::vector<std::byte> bank(0x40c0);
            incoming.mount(0x52dfd8,bank,SourceRuntimeMemory::Access::ReadWrite,"declared retired incoming query bank");
            bank[0x531fbc-0x52dfd8]=std::byte{1};bank[0x531fc0-0x52dfd8]=std::byte{1};
            bad.mount(0x52dfd8,bank,SourceRuntimeMemory::Access::ReadWrite,"declared unretired incoming query bank");
            SourceLensQueryFrameTu15 owner(renderer);
            for(auto &query:queries)query.generation=4;view.lens_queries=queries;
            check(renderer.draw(camera,{}, {},false,{},{},{},std::span{&view,1}),"Pre-handoff query scene was not submitted");
            bool rejected{};try{owner.rebind(bad);}catch(const FormatError &){rejected=true;}
            check(rejected,"Unretired native query bank was admitted");
            results=renderer.take_lens_query_results();check(results.size()==queries.size(),"Rejected handoff retired the live GPU owner");
            for(const auto &result:results)check(result.generation==4,"Rejected handoff changed old GPU generations");
            for(auto &query:queries)query.generation=5;
            check(renderer.draw(camera,{}, {},false,{},{},{},std::span{&view,1}),"Pending old query scene was not submitted");
            owner.rebind(incoming);check(owner.completed().empty()&&renderer.take_lens_query_results().empty(),
                "Old GPU results survived an incoming-world handoff");
            bool foreign_rejected{};std::thread foreign([&]{try{owner.rebind(bad);}catch(const FormatError &){foreign_rejected=true;}});foreign.join();
            check(foreign_rejected,"Query handoff admitted a foreign executor");
            rejected=false;try{owner.rebind(incoming);}catch(const FormatError &){rejected=true;}
            check(rejected,"Incoming query owner committed the same world twice");
            for(auto &query:queries)query.generation=1;
            check(renderer.draw(camera,{}, {},false,{},{},{},std::span{&view,1}),"Replacement query identities were not submitted");
            results=renderer.take_lens_query_results();check(results.size()==queries.size(),"Replacement GPU results are absent");
            for(const auto &result:results){
                if(result.id>=expected.size()||result.generation!=1||result.samples!=expected[result.id])
                    std::cout<<"replacement query="<<result.id<<" generation="<<result.generation<<" samples="<<result.samples
                        <<" expected="<<(result.id<expected.size()?expected[result.id]:0)<<'\n';
                check(result.id<expected.size()&&result.generation==1&&result.samples==expected[result.id],
                    "Replacement query reused an old generation or changed actual coverage");
            }
            report["incoming_query_handoff"]={{"rejected_bank_preserves_GPU",true},{"old_results_retired",true},
                {"foreign_executor_rejected",true},{"duplicate_world_rejected",true},{"fresh_identity_reuse",true}};
        }
        {
            const auto submit=[&](std::uint64_t generation){
                for(auto &query:queries)query.generation=generation;
                view.lens_queries=queries;
                check(renderer.draw(camera,{}, {},false,{},{},{},std::span{&view,1}),
                    "Prepared retirement query scene was not submitted");
            };
            const auto verify=[&](const auto &completed,std::uint64_t generation){
                check(completed.size()==queries.size(),"Prepared retirement lost original query results");
                std::array<bool,3> seen{};
                for(const auto &result:completed){
                    check(result.id<seen.size()&&!seen[result.id]&&result.generation==generation&&
                        result.samples==expected[result.id],"Prepared retirement changed original query identity/coverage");
                    seen[result.id]=true;
                }
            };
            submit(10);
            {
                auto cancelled=renderer.prepare_lens_query_retirement();
                check(renderer.lens_query_retirement_ready(cancelled),"Fresh GPU retirement was not ready");
#ifdef HALO2_VULKAN_LENS_RETIREMENT_OBSERVER
                begin_vulkan_lens_retirement_observation();
#endif
            }
#ifdef HALO2_VULKAN_LENS_RETIREMENT_OBSERVER
            check(end_vulkan_lens_retirement_observation().total()==0,
                "Cancelling GPU retirement destroyed the live owner");
#endif
            verify(renderer.take_lens_query_results(),10);
            auto stale_draw=renderer.prepare_lens_query_retirement();
            submit(11);
            check(!renderer.lens_query_retirement_ready(stale_draw)&&
                !renderer.commit_lens_query_retirement(std::move(stale_draw)),
                "GPU retirement accepted intervening renderer work");
            verify(renderer.take_lens_query_results(),11);
            submit(12);auto stale_harvest=renderer.prepare_lens_query_retirement();
            verify(renderer.take_lens_query_results(),12);
            check(!renderer.lens_query_retirement_ready(stale_harvest)&&
                !renderer.commit_lens_query_retirement(std::move(stale_harvest)),
                "GPU retirement accepted intervening query harvest");
            submit(13);auto stale_resize=renderer.prepare_lens_query_retirement();
            renderer.resized();
            check(!renderer.lens_query_retirement_ready(stale_resize)&&
                !renderer.commit_lens_query_retirement(std::move(stale_resize)),
                "GPU retirement accepted an intervening resize");
            verify(renderer.take_lens_query_results(),13);
            submit(14);auto stale_retire=renderer.prepare_lens_query_retirement();
            renderer.retire_lens_queries();
            check(!renderer.lens_query_retirement_ready(stale_retire)&&
                !renderer.commit_lens_query_retirement(std::move(stale_retire)),
                "GPU retirement accepted a retired owner");
            check(renderer.take_lens_query_results().empty(),"Immediate retirement retained old results");
            // Retain both cached completed results and the next pending query
            // batch. Publication must detach them without waiting or reading.
            submit(15);submit(16);
            std::optional<Renderer::PreparedLensQueryRetirement> admitted{
                renderer.prepare_lens_query_retirement()};
            bool foreign_rejected{},foreign_prepare_rejected{};
            std::thread foreign([&]{
                foreign_rejected=!renderer.lens_query_retirement_ready(*admitted)&&
                    !renderer.commit_lens_query_retirement(std::move(*admitted));
                try{(void)renderer.prepare_lens_query_retirement();}
                catch(const FormatError &){foreign_prepare_rejected=true;}
            });foreign.join();
            check(foreign_rejected&&foreign_prepare_rejected,"GPU retirement accepted a foreign executor");
            check(renderer.lens_query_retirement_ready(*admitted),"Rejected caller consumed GPU retirement");
#ifdef HALO2_VULKAN_LENS_RETIREMENT_OBSERVER
            begin_vulkan_lens_retirement_observation();
#endif
            const auto committed=renderer.commit_lens_query_retirement(std::move(*admitted));
#ifdef HALO2_VULKAN_LENS_RETIREMENT_OBSERVER
            const auto publication=end_vulkan_lens_retirement_observation();
            check(publication.total()==0,"GPU publication waited, destroyed resources or harvested queries");
#endif
            check(committed,"Prepared GPU retirement failed publication");
            check(!renderer.lens_query_retirement_ready(*admitted)&&
                !renderer.commit_lens_query_retirement(std::move(*admitted)),"Consumed GPU retirement replayed");
            check(renderer.take_lens_query_results().empty(),"Prepared retirement exposed old cached/pending queries");
            // The new owner must work while the old owner is still retained.
            // Reuse the original IDs/generation to catch old-pool aliasing.
            submit(1);verify(renderer.take_lens_query_results(),1);
#ifdef HALO2_VULKAN_LENS_RETIREMENT_OBSERVER
            begin_vulkan_lens_retirement_observation();
#endif
            admitted.reset(); // Outside publication, on the renderer executor.
#ifdef HALO2_VULKAN_LENS_RETIREMENT_OBSERVER
            const auto retirement=end_vulkan_lens_retirement_observation();
            check(retirement.waits==1&&retirement.buffers==1&&retirement.memory==1&&
                retirement.pipelines==1&&retirement.layouts==1&&retirement.pools==2&&retirement.query_reads==0,
                "Detached GPU owner did not defer its actual wait/resource destruction");
            report["prepared_gpu_retirement"]["vulkan_observer"]={{"available",true},
                {"publication_calls",publication.total()},{"deferred_waits",retirement.waits},
                {"deferred_buffers",retirement.buffers},{"deferred_memory",retirement.memory},
                {"deferred_pipelines",retirement.pipelines},{"deferred_layouts",retirement.layouts},
                {"deferred_query_pools",retirement.pools},{"deferred_query_reads",retirement.query_reads}};
#else
            report["prepared_gpu_retirement"]["vulkan_observer"]={{"available",false}};
#endif
            report["prepared_gpu_retirement"].update({{"cancellation_preserves_results",true},
                {"draw_harvest_resize_retirement_invalidate",true},{"foreign_executor_rejected",true},
                {"consumed_token_rejected",true},{"cached_and_pending_results_detached",true},
                {"replacement_coverage_while_old_owner_retained",true}});
        }
        report["passed"]=true;report["full_goal_complete"]=false;
        report["boundary"]="Actual native world-depth GPU queries from original quads; live map scheduling, query smoothing and final flare rendering remain separate acceptance.";
        auto retired_query_owner=std::make_unique<SourceLensQueryFrameTu15>(renderer);
        auto retirement=std::make_optional(renderer.prepare_lens_query_retirement());
        const auto lease=renderer.device_lease();
#ifdef HALO2_VULKAN_LENS_RETIREMENT_OBSERVER
        begin_vulkan_lens_retirement_observation();
#endif
        renderer_owner.reset();
#ifdef HALO2_VULKAN_LENS_RETIREMENT_OBSERVER
        check(end_vulkan_lens_retirement_observation().total()==0,
            "Renderer destroyed device resources while a prepared retirement retained their lease");
#endif
        check(!lease.valid(),"Retained device allocation left retired renderer eligible");
        retired_query_owner.reset();retirement.reset();
        report["retired_renderer_owner_safe"]=true;
        report["prepared_retirement_retains_device_without_live_eligibility"]=true;
    } catch(...) {std::cerr.rdbuf(previous);std::ofstream(output/"validation.log")<<diagnostics.str();SDL_DestroyWindow(window);SDL_Quit();throw;}
    std::cerr.rdbuf(previous);std::ofstream(output/"validation.log")<<diagnostics.str();
    SDL_DestroyWindow(window);SDL_Quit();
    check(diagnostics.str().find("VUID-")==std::string::npos && diagnostics.str().find("Validation Error")==std::string::npos,
        "Original lens query GPU validation failed");
    std::ofstream(output/"report.json")<<report.dump(2)<<'\n';
    std::cout<<"PASS original lens quads: real visible/occluded/empty GPU samples, immutable presentation and generation reuse\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
