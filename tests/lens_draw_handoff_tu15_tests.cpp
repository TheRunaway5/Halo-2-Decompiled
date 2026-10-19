#include "halo2/multiplayer_world.hpp"
#include "halo2/source_execution_lens.hpp"
#include "halo2/source_lens_draw_frame_tu15.hpp"
#include "halo2/source_lens_presentation_handoff_tu15.hpp"
#include "halo2/source_match_authority.hpp"
#include "halo2/renderer.hpp"
#include "helpers/vulkan_lens_retirement_observer.hpp"
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <thread>

using namespace halo2;
namespace {
void check(bool value,const char *why){if(!value)throw FormatError(why);}
template<class F>void rejects(F run){bool rejected{};try{run();}catch(const FormatError&){rejected=true;}
    check(rejected,"Invalid final lens presentation handoff accepted");}
struct Clock final:PhysicsLiveClockSource {
    std::uint32_t milliseconds()const noexcept override{return 1000;}
    std::uint64_t cycles()const noexcept override{return 7000000;}
    std::uint64_t utc_filetime()const noexcept override{return 132000000000000000ull;}
};
}
int main(int argc,char **argv)try{
    check(argc==4,"Supply XBE MAP REPORT");
    if(!std::filesystem::exists(argv[1])||!std::filesystem::exists(argv[2]))return 77;
    check(std::getenv("HALO2_TEST_SCRATCH"),"Final lens handoff requires owned scratch");
    MultiplayerOptions options;options.map=argv[2];options.shared_maps=options.map.parent_path();
    options.gameplay_xbe=argv[1];options.local_players=1;options.variant={"Slayer",0,{{0x59,5},{10,0},{2,0}}};
    MultiplayerPreparedWorld first(options,std::nullopt,std::make_shared<Clock>());
    MultiplayerPreparedWorld second(options,std::nullopt,std::make_shared<Clock>());
    check(first.source_objects&&second.source_objects,"Final lens handoff lacks real native worlds");
    first.source_objects->retire_lens_queries();second.source_objects->retire_lens_queries();
    auto &incoming=second.source_objects->memory();
    Mesh mesh;mesh.materials.emplace_back();
    SourceLensDrawFrameTu15 owner(*first.cache,mesh);
    const auto material_count=mesh.materials.size();check(material_count>1,"Actual lens images were not prepared before GPU upload");
    SourceRuntimeMemory pending;std::vector<std::byte> bank(0x40c0);bank[0x531fbc-0x52dfd8]=std::byte{1};
    pending.mount(0x52dfd8,bank,SourceRuntimeMemory::Access::ReadWrite,"declared pending incoming CPU lens control");
    rejects([&]{owner.rebind(incoming);}); // No renderer lease yet.
    SDL_SetMainReady();check(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());
    auto *window=SDL_CreateWindow("Original lens owner handoff",0,0,640,480,SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN);
    check(window,SDL_GetError());
    std::ostringstream diagnostics;auto *previous=std::cerr.rdbuf(diagnostics.rdbuf());
    nlohmann::json report;
    std::unique_ptr<SourceLensQueryFrameTu15> queries;
    std::optional<SourceLensPresentationHandoffTu15::Prepared> expired;
    try{
      {
        const auto *directory=std::getenv("HALO2_NATIVE_GPU_SHADER_DIR");
        Renderer renderer(window,mesh,true,{},nullptr,{},nullptr,directory?directory:std::filesystem::path{});
        owner.bind(renderer);
        unsigned views{},rebindings{};
        const auto frame=[&](MultiplayerPreparedWorld &world){
            owner.begin();
            world.source_objects->render_model_view([&](auto &source){
                const auto draws=owner.view(source,0,{0,0,0},{1,0,0},{0,0,1},.75f,
                    SourceViewProjectionBank{},{0,0,480,640},640,480);
                check(draws.empty(),"Retired empty request bank invented a flare");++views;
            });
            owner.finish(false);
        };
        // A client may upload these resident images while no native producer
        // is bound. The first actual source borrower arrives only here.
        owner.rebind(incoming);++rebindings;frame(second);
        owner.rebind(first.source_objects->memory());++rebindings;frame(first);
        rejects([&]{owner.rebind(pending);});frame(first);
        owner.begin();rejects([&]{owner.rebind(incoming);});owner.finish(false);frame(first);
        bool foreign_rejected{};std::thread foreign([&]{try{owner.rebind(incoming);}catch(const FormatError&){foreign_rejected=true;}});
        foreign.join();check(foreign_rejected,"Final lens owner accepted a foreign executor");frame(first);
        for(unsigned n=0;n<4;++n){
            auto &world=(n&1)?first:second;auto &other=(n&1)?second:first;
            SourceMatchAuthority native(world.match,*world.source_objects);
            const auto sealed=native.execution_boundary();
            owner.rebind(world.source_objects->memory());++rebindings;
            check(native.execution_boundary()==sealed,"Final lens rebind invalidated the sealed native adoption boundary");
            rejects([&]{owner.rebind(world.source_objects->memory());});
            owner.begin();
            other.source_objects->render_model_view([&](auto &source){
                rejects([&]{owner.view(source,0,{},{1,0,0},{0,0,1},.75f,SourceViewProjectionBank{},
                    {0,0,480,640},640,480);});
            });owner.finish(false);
            frame(world);frame(world);
            check(mesh.materials.size()==material_count,"Final lens handoff duplicated resident material identities");
        }
        queries=std::make_unique<SourceLensQueryFrameTu15>(renderer);
        using Handoff=SourceLensPresentationHandoffTu15;
        // An empty actual hardware query is a declared rollback control, not
        // a fabricated native flare/result. Keep its completed GPU work pending
        // while a later owner rejects or a prepared transaction is cancelled.
        std::uint64_t gpu_generation{};unsigned preserved_gpu{};
        const auto submit=[&]{
            SourceLensQueryDraw query;query.id=47;query.generation=++gpu_generation;
            Camera camera;ModelView view;view.camera=camera;view.width=640;view.height=480;
            view.full_surface=true;view.lens_queries=std::span{&query,1};
            check(renderer.draw(camera,{}, {},false,{},{},{},std::span{&view,1}),"Rollback GPU query was not submitted");
        };
        const auto preserved=[&]{
            const auto results=renderer.take_lens_query_results();
            check(results.size()==1&&results[0].id==47&&results[0].generation==gpu_generation&&results[0].samples==0,
                "Uncommitted paired handoff retired or replaced old actual GPU work");++preserved_gpu;
        };
        submit();owner.begin();
        rejects([&]{(void)Handoff::prepare(*queries,owner,incoming);});
        owner.finish(false);preserved();frame(first); // Query preparation succeeded; drawing rejected.
        SourceMatchAuthority native(second.match,*second.source_objects);
        const auto sealed=native.execution_boundary();
        submit();{
            auto cancelled=Handoff::prepare(*queries,owner,incoming);
            check(Handoff::ready(*queries,owner,cancelled,incoming),"Fresh paired preparation is not ready");
            check(native.execution_boundary()==sealed,"Paired preparation mutated the incoming native world");
        }preserved();frame(first);
        submit();auto stale_draw=Handoff::prepare(*queries,owner,incoming);
        owner.begin();owner.finish(false); // Even an unsubmitted frame invalidates its old token.
        check(!Handoff::ready(*queries,owner,stale_draw,incoming)&&
            !Handoff::commit(*queries,owner,std::move(stale_draw),incoming),"Paired handoff accepted an intervening draw frame");
        preserved();frame(first);
        auto stale_query=Handoff::prepare(*queries,owner,incoming);
        first.source_objects->render_model_view([&](auto &source){queries->begin(source);queries->finish(source,false);});
        submit();check(!Handoff::ready(*queries,owner,stale_query,incoming)&&
            !Handoff::commit(*queries,owner,std::move(stale_query),incoming),"Paired handoff accepted an intervening query frame");
        preserved();frame(first);
        auto stale_renderer=Handoff::prepare(*queries,owner,incoming);
        submit();check(!Handoff::ready(*queries,owner,stale_renderer,incoming)&&
            !Handoff::commit(*queries,owner,std::move(stale_renderer),incoming),
            "Paired handoff accepted intervening actual renderer activity");
        preserved();frame(first);
        SourceRuntimeMemory changing;std::vector<std::byte> retired(0x40c0);
        changing.mount(0x52dfd8,retired,SourceRuntimeMemory::Access::ReadWrite,"declared retired bank changed after preparation");
        submit();auto invalid_incoming=Handoff::prepare(*queries,owner,changing);
        const std::array<std::byte,4> one{std::byte{1}};
        changing.write(0x531fbc,one);changing.write(0x531fc0,one);
        check(!Handoff::ready(*queries,owner,invalid_incoming,changing)&&
            !Handoff::commit(*queries,owner,std::move(invalid_incoming),changing),"Paired handoff accepted newly pending incoming queries");
        preserved();frame(first);
        submit();std::optional<Handoff::Prepared> admitted{Handoff::prepare(*queries,owner,incoming)};
        bool paired_foreign_rejected{};std::thread foreign_commit([&]{
            paired_foreign_rejected=!Handoff::ready(*queries,owner,*admitted,incoming)&&
                !Handoff::commit(*queries,owner,std::move(*admitted),incoming);
        });foreign_commit.join();check(paired_foreign_rejected,"Paired handoff accepted a foreign executor");
        check(!Handoff::ready(*queries,owner,*admitted,first.source_objects->memory())&&
            !Handoff::commit(*queries,owner,std::move(*admitted),first.source_objects->memory()),
            "Paired handoff accepted a different incoming owner");
        check(Handoff::ready(*queries,owner,*admitted,incoming),"Rejected caller consumed a valid paired token");
        nlohmann::json retirement_observation={{"available",false}};
#ifdef HALO2_VULKAN_LENS_RETIREMENT_OBSERVER
        begin_vulkan_lens_retirement_observation();
#endif
        const auto committed=Handoff::commit(*queries,owner,std::move(*admitted),incoming);
#ifdef HALO2_VULKAN_LENS_RETIREMENT_OBSERVER
        const auto commit_calls=end_vulkan_lens_retirement_observation();
        check(commit_calls.total()==0,"Paired publication performed a Vulkan call inside its commit boundary");
#endif
        check(committed,"Paired publication failed");
        check(renderer.take_lens_query_results().empty()&&queries->completed().empty(),"Paired commit retained old GPU results");
        check(!Handoff::ready(*queries,owner,*admitted,incoming)&&
            !Handoff::commit(*queries,owner,std::move(*admitted),incoming),"Consumed paired token was replayed");
        check(native.execution_boundary()==sealed,"Paired publication invalidated the sealed native adoption boundary");
        // Destruction is a separate executor operation after publication has
        // returned. The consumed token owns the retired actual GPU pools until
        // this point; any device wait belongs here, outside a native commit.
#ifdef HALO2_VULKAN_LENS_RETIREMENT_OBSERVER
        begin_vulkan_lens_retirement_observation();
#endif
        admitted.reset();
#ifdef HALO2_VULKAN_LENS_RETIREMENT_OBSERVER
        const auto retired_calls=end_vulkan_lens_retirement_observation();
        check(retired_calls.waits==1&&retired_calls.buffers==1&&retired_calls.memory==1&&
            retired_calls.pipelines==1&&retired_calls.layouts==1&&retired_calls.pools==2&&
            retired_calls.query_reads==0,
            "Consumed paired token did not defer actual GPU owner cleanup until outside publication");
        retirement_observation={{"available",true},{"commit_vulkan_calls",commit_calls.total()},
            {"deferred_cleanup_vulkan_calls",retired_calls.total()},
            {"deferred_device_waits",retired_calls.waits},{"deferred_buffers",retired_calls.buffers},
            {"deferred_memory_frees",retired_calls.memory},{"deferred_pipelines",retired_calls.pipelines},
            {"deferred_pipeline_layouts",retired_calls.layouts},{"deferred_query_pools",retired_calls.pools},
            {"deferred_query_reads",retired_calls.query_reads}};
#endif
        frame(second);
        second.source_objects->render_model_view([&](auto &source){queries->begin(source);queries->finish(source,false);});
        check(mesh.materials.size()==material_count,"Paired handoff duplicated resident material identities");
        // A different owner cannot consume a token even on the same executor
        // and renderer. Tokens also stay stale after returning to the same world.
        auto original=Handoff::prepare(*queries,owner,first.source_objects->memory());
        {
            SourceLensQueryFrameTu15 other(renderer);
            check(!Handoff::ready(other,owner,original,first.source_objects->memory())&&
                !Handoff::commit(other,owner,std::move(original),first.source_objects->memory()),
                "Paired handoff accepted another query owner");
        }
        auto replacement=Handoff::prepare(*queries,owner,first.source_objects->memory());
        check(Handoff::commit(*queries,owner,std::move(replacement),first.source_objects->memory()),"Replacement handoff failed");
        auto back=Handoff::prepare(*queries,owner,incoming);
        check(Handoff::commit(*queries,owner,std::move(back),incoming),"Return handoff failed");
        check(!Handoff::ready(*queries,owner,original,first.source_objects->memory())&&
            !Handoff::commit(*queries,owner,std::move(original),first.source_objects->memory()),
            "Paired handoff accepted an old generation after a world round trip");
        frame(second);
        expired.emplace(Handoff::prepare(*queries,owner,first.source_objects->memory()));
        report={{"passed",true},{"gpu",renderer.device_name()},{"actual_native_worlds",2},
            {"rebindings",rebindings},{"native_views",views},{"resident_materials",material_count},
            {"pending_bank_rejected",true},{"active_frame_rejected",true},{"foreign_executor_rejected",true},
            {"wrong_world_rejected",true},{"duplicate_world_rejected",true},{"materials_unchanged",true},
            {"sealed_native_boundary_unchanged",true},
            {"cold_client_binding",true},
            {"boundary","Actual original bitmap/header initialization and empty retired rings across distinct native worlds; visible flare/match takeover acceptance is separate"}};
        report["paired_transaction"]={{"partial_prepare_rollback",true},{"cancel_preserves_old_owners",true},
            {"stale_unsubmitted_draw_rejected",true},{"stale_unsubmitted_query_rejected",true},
            {"stale_renderer_activity_rejected",true},
            {"changed_incoming_ring_rejected",true},{"foreign_executor_rejected",true},
            {"wrong_incoming_owner_rejected",true},{"wrong_query_owner_rejected",true},
            {"consumed_token_rejected",true},{"generation_roundtrip_rejected",true},
            {"real_gpu_results_preserved",preserved_gpu},{"old_gpu_results_retired_on_commit",true},
            {"vulkan_retirement_observation",retirement_observation},
            {"resident_materials_unchanged",true},{"native_boundary_unchanged",true}};
      }
        check(!SourceLensPresentationHandoffTu15::ready(*queries,owner,*expired,first.source_objects->memory())&&
            !SourceLensPresentationHandoffTu15::commit(*queries,owner,std::move(*expired),first.source_objects->memory()),
            "Paired handoff accepted a retired renderer");
        expired.reset();queries.reset();report["paired_transaction"]["expired_renderer_rejected"]=true;
        rejects([&]{owner.rebind(incoming);});report["expired_renderer_rejected"]=true;
    }catch(...){std::cerr.rdbuf(previous);std::ofstream(std::string(argv[3])+".validation.log")<<diagnostics.str();
        expired.reset();queries.reset();
        SDL_DestroyWindow(window);SDL_Quit();throw;}
    std::cerr.rdbuf(previous);SDL_DestroyWindow(window);SDL_Quit();
    std::ofstream(std::string(argv[3])+".validation.log")<<diagnostics.str();
    check(diagnostics.str().find("VUID-")==std::string::npos&&diagnostics.str().find("Validation Error")==std::string::npos,
        "Original final lens owner handoff failed GPU validation");
    std::ofstream(argv[3])<<report.dump(2)<<'\n';
    std::cout<<"PASS completed-frame lens bitmap/header owner: two real native worlds, six rebindings including a cold client, unchanged resident materials and rejected invalid borrowers\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
