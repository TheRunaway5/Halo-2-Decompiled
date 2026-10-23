#pragma once
#include "halo2/source_peer_host_takeover.hpp"
#include "halo2/source_lens_presentation_handoff_tu15.hpp"
#include "halo2/source_execution_lens.hpp"
#include "halo2/source_match_authority.hpp"
#include "halo2/multiplayer_audio.hpp"
#include "halo2/renderer.hpp"
#ifdef HALO2_VULKAN_LENS_RETIREMENT_OBSERVER
#include "vulkan_lens_retirement_observer.hpp"
#endif
#include <SDL.h>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <span>
#include <string>
#include <string_view>
#include <thread>

// This dedicated GNU/Linux test target wraps the actual const member symbol
// recorded from its native executable. The real function remains authoritative:
// observation forwards its exact member ABI, result and exceptions unchanged.
#if !defined(__linux__) || !defined(__GNUC__)
#error Held native lens read observation requires the qualified GNU/Linux ABI
#endif
namespace halo2::test {
namespace held_lens_detail {
inline thread_local const SourceRuntimeMemory *observed_memory{};
inline thread_local std::uint64_t observed_views{};
inline void begin_memory_views(const SourceRuntimeMemory &memory) noexcept {
    observed_views=0;observed_memory=&memory;
}
inline std::uint64_t end_memory_views() noexcept {
    observed_memory=nullptr;return observed_views;
}
}
}
extern "C" std::span<const std::byte> halo2_test_real_source_memory_view(
    const halo2::SourceRuntimeMemory *,std::uint32_t,std::size_t)
    asm("__real__ZNK5halo219SourceRuntimeMemory4viewEjm");
extern "C" std::span<const std::byte> halo2_test_wrap_source_memory_view(
    const halo2::SourceRuntimeMemory *,std::uint32_t,std::size_t)
    asm("__wrap__ZNK5halo219SourceRuntimeMemory4viewEjm");
extern "C" __attribute__((noinline)) std::span<const std::byte> halo2_test_wrap_source_memory_view(
    const halo2::SourceRuntimeMemory *memory,std::uint32_t address,std::size_t bytes){
    using namespace halo2::test::held_lens_detail;
    if(observed_memory&&memory==observed_memory&&observed_views!=UINT64_MAX)++observed_views;
    return halo2_test_real_source_memory_view(memory,address,bytes);
}

namespace halo2::test {
namespace held_lens_detail {
inline void require(bool value,const char *why){if(!value)throw FormatError(why);}
template<class F>void rejects(F &&call){bool rejected{};try{call();}catch(const FormatError &){rejected=true;}
    require(rejected,"Held native lens control did not reject native reentry");}
inline bool environment(std::string_view key,std::string_view expected){
    const auto value=std::getenv(std::string(key).c_str());return value&&value==expected;
}
struct Video {
    Video(){SDL_SetMainReady();require(SDL_InitSubSystem(SDL_INIT_VIDEO)==0,SDL_GetError());}
    ~Video(){SDL_QuitSubSystem(SDL_INIT_VIDEO);}
};
struct Redirect {
    std::streambuf *previous;
    explicit Redirect(std::ostringstream &out):previous(std::cerr.rdbuf(out.rdbuf())){}
    ~Redirect(){std::cerr.rdbuf(previous);}
};
}

// Component-only fixture. Both candidates already came from genuine native
// imports and learned certificates; this helper creates no native World, peer
// round, election Ready, game window on the user's display or hardware audio.
inline void held_lens_controls(SourcePeerHostTakeover &candidate,SourcePeerHostTakeover &foreign){
    using namespace held_lens_detail;
    using Handoff=SourceLensPresentationHandoffTu15;
#ifndef HALO2_VULKAN_LENS_RETIREMENT_OBSERVER
    throw FormatError("Held native lens fixture requires its actual Vulkan retirement observer");
#endif
    require(environment("HALO2_TAKEOVER_HELD_LENS_PRIVATE_XVFB","1")&&std::getenv("DISPLAY")&&
        environment("SDL_VIDEODRIVER","x11")&&environment("SDL_AUDIODRIVER","dummy"),
        "Held native lens fixture requires its private Xvfb/x11 and synthetic audio CTest");
    const auto scratch=std::getenv("HALO2_TEST_SCRATCH");
    require(scratch&&std::filesystem::is_directory(scratch),"Held native lens fixture requires owned scratch");
    // Every native getter/provider/ring inspection is before the actual hold.
    // Renderer and private presentation SDK calls below never step this world.
    auto &target=candidate.world();auto *const objects=target.source_objects.get();
    require(objects&&objects->owns_physics(),"Held native lens fixture lacks its actual candidate world");
    // candidate.world() above already validated this exact original owner.
    const auto cache=target.cache;const auto &memory=objects->memory();
    auto *const native=&objects->tu15_world();auto provider=native->sound_variation_provider(*cache);
    SourceMatchAuthority authority(target.match,*objects);const auto sealed=authority.execution_boundary();
    source_require_retired_lens_queries(memory);
    Video video;require(SDL_GetCurrentVideoDriver()&&std::string_view(SDL_GetCurrentVideoDriver())=="x11",
        "Held native lens fixture did not acquire its private X11 driver");
    auto window=std::unique_ptr<SDL_Window,decltype(&SDL_DestroyWindow)>(SDL_CreateWindow(
        "Private held native lens component",0,0,640,480,SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN),SDL_DestroyWindow);
    require(bool(window),SDL_GetError());
    Mesh mesh;mesh.materials.emplace_back();SourceLensDrawFrameTu15 draws(*cache,mesh);
    const auto material_count=mesh.materials.size();
    require(material_count>1,"Held native lens fixture did not retain actual authored images before upload");
    std::ostringstream diagnostics;
    unsigned preserved{},accepted{};bool observed_retirement{};
    std::uint64_t strict_ring_views{},bound_ring_views{};
    {
        Redirect redirect(diagnostics);
        const auto directory=std::getenv("HALO2_NATIVE_GPU_SHADER_DIR");
        Renderer renderer(window.get(),mesh,true,{},nullptr,{},nullptr,directory?directory:std::filesystem::path{});
        draws.bind(renderer);auto queries=std::make_unique<SourceLensQueryFrameTu15>(renderer);
        std::uint64_t generation{};
        const auto submit=[&]{
            SourceLensQueryDraw query;query.id=47;query.generation=++generation;
            Camera camera;ModelView view;view.camera=camera;view.width=640;view.height=480;
            view.full_surface=true;view.lens_queries=std::span{&query,1};
            require(renderer.draw(camera,{}, {},false,{},{},{},std::span{&view,1}),
                "Held native lens actual empty GPU query was not submitted");
        };
        const auto preserve=[&]{
            const auto results=renderer.take_lens_query_results();
            require(results.size()==1&&results[0].id==47&&results[0].generation==generation&&results[0].samples==0,
                "Rejected/cancelled held lens transaction retired old actual GPU work");++preserved;
        };
        submit();
        {
            auto prepared=Handoff::prepare_host(*queries,draws,candidate);
            require(Handoff::ready(*queries,draws,prepared,memory)&&!Handoff::ready(*queries,draws,prepared)&&
                !Handoff::bind_publication(prepared,SourceNativePublication::Boundary{}),
                "Default Boundary or unbound token fabricated held lens readiness");
            {
                auto other=foreign.hold_native_publication();
                require(other.current()&&!Handoff::bind_publication(prepared,other.boundary())&&
                    !Handoff::ready(*queries,draws,prepared),"Another actual target's native scope admitted the lens pair");
            }
            {
                auto scope=candidate.hold_native_publication();const auto boundary=scope.boundary();
                require(Handoff::bind_publication(prepared,boundary)&&Handoff::bind_publication(prepared,boundary)&&
                    Handoff::ready(*queries,draws,prepared)&&Handoff::ready(*queries,draws,prepared,memory),
                    "Genuine held native Boundary did not admit the exact prepared lens pair");
                // Native-only exclusion leaves already-borrowed read-only
                // memory available; timing/SDL exclusion is tested below.
                source_require_retired_lens_queries(memory);
                rejects([&]{(void)candidate.world();});
            }
            require(!Handoff::ready(*queries,draws,prepared)&&!Handoff::commit(*queries,draws,std::move(prepared)),
                "Expired native Boundary retained held lens eligibility");
            {
                auto replacement=candidate.hold_native_publication();
                require(replacement.current()&&!Handoff::bind_publication(prepared,replacement.boundary())&&
                    !Handoff::ready(*queries,draws,prepared),"A different real scope revived an expired lens pair");
            }
        }
        preserve();
        // The strict path still reads the actual ring. A native-only hold
        // permits that read through an already-borrowed memory reference.
        {
            auto strict=Handoff::prepare_host(*queries,draws,candidate);
            require(Handoff::ready(*queries,draws,strict,memory),"Unbound actual ring was not admissible before hold");
            {auto scope=candidate.hold_native_publication();
                require(Handoff::ready(*queries,draws,strict,memory)&&!Handoff::ready(*queries,draws,strict),
                    "Native-only hold changed strict read-only lens admission");}
            require(Handoff::ready(*queries,draws,strict,memory),"Unbound strict path lost its unchanged candidate after hold");
        }
        submit();
        {
            auto stale=Handoff::prepare_host(*queries,draws,candidate);
            draws.begin();draws.finish(false); // Actual draw revision, no native call.
            auto scope=candidate.hold_native_publication();
            require(Handoff::bind_publication(stale,scope.boundary())&&!Handoff::ready(*queries,draws,stale)&&
                !Handoff::commit(*queries,draws,std::move(stale)),"Stale draw revision partially published a held lens pair");
        }
        preserve();
        {
            auto stale=Handoff::prepare_host(*queries,draws,candidate);
            // Actual query owner identity retirement, not a claimed native
            // query-frame revision. Strict rebind only reads the retired ring.
            queries->rebind(memory);queries.reset();
            queries=std::make_unique<SourceLensQueryFrameTu15>(renderer);
            auto scope=candidate.hold_native_publication();
            require(Handoff::bind_publication(stale,scope.boundary())&&!Handoff::ready(*queries,draws,stale)&&
                !Handoff::commit(*queries,draws,std::move(stale)),"Retired query identity partially published a held lens pair");
        }
        {
            auto stale=Handoff::prepare_host(*queries,draws,candidate);submit();
            auto scope=candidate.hold_native_publication();
            require(Handoff::bind_publication(stale,scope.boundary())&&!Handoff::ready(*queries,draws,stale)&&
                !Handoff::commit(*queries,draws,std::move(stale)),"Intervening actual renderer work did not invalidate lens retirement");
        }
        preserve();
        // Real cached plus pending GPU results remain owned by the old bank.
        // The consumed pair retains their allocations through publication.
        submit();submit();std::optional<Handoff::Prepared> admitted{Handoff::prepare_host(*queries,draws,candidate)};
        // Keep a separate genuine unbound preparation alive until after every
        // hold. Its original path still reads the ring through already-borrowed
        // const memory; it never receives a native Boundary binding.
        std::optional<Handoff::Prepared> strict{Handoff::prepare_host(*queries,draws,candidate)};
        const SdlSoundPlatformConfiguration manual{SdlSoundPlatformConfiguration::Delivery::Manual,256,{}};
        MultiplayerAudio audio(cache,0x651d83f1u,manual);audio.enroll_continuation();audio.bind_native_sound_metadata(provider);
        const auto checkpoint=audio.checkpoint();
        require(Handoff::ready(*queries,draws,*strict,memory)&&!Handoff::ready(*queries,draws,*strict),
            "Final strict lens control was not eligible before actual publication holds");
        {
            auto scope=candidate.hold_native_publication();const auto boundary=scope.boundary();
            require(Handoff::bind_publication(*admitted,boundary),"Final held lens pair could not bind its actual scope");
            const auto binding=provider.native_binding(scope,cache);auto sound=audio.prepare_restore(checkpoint,binding);
            {
                auto audio_hold=audio.hold_publication(sound);
                require(audio.ready(sound,audio_hold)&&scope.current(),"Final lens control lacks genuine native/timing/SDL holds");
                begin_memory_views(memory);
                const bool strict_ready=Handoff::ready(*queries,draws,*strict,memory);
                strict_ring_views=end_memory_views();
                require(strict_ready&&strict_ring_views>0&&!Handoff::ready(*queries,draws,*strict),
                    "Strict lens control did not observe real allowed raw ring reads under the holds");
                // Already-borrowed const memory has no publication/timing read
                // gate. Do not invent one; the bound path must avoid these
                // real reads rather than rely on their rejection.
                source_require_retired_lens_queries(memory);
                rejects([&]{provider.require_current(*cache);});rejects([&]{(void)candidate.world();});
                bool foreign_refused{};std::thread wrong([&]{foreign_refused=!Handoff::ready(*queries,draws,*admitted)&&
                    !Handoff::commit(*queries,draws,std::move(*admitted));});wrong.join();
                require(foreign_refused,"Held lens pair crossed its native/renderer executor");
#ifdef HALO2_VULKAN_LENS_RETIREMENT_OBSERVER
                begin_vulkan_lens_retirement_observation();
#endif
                begin_memory_views(memory);
                const bool ready=Handoff::ready(*queries,draws,*admitted);
                const bool committed=Handoff::commit(*queries,draws,std::move(*admitted));
                bound_ring_views=end_memory_views();
#ifdef HALO2_VULKAN_LENS_RETIREMENT_OBSERVER
                const auto commit_calls=end_vulkan_lens_retirement_observation();
                require(commit_calls.total()==0,"Held native lens publication performed Vulkan work under native/timing/SDL holds");
#endif
                require(ready&&committed&&bound_ring_views==0&&!Handoff::ready(*queries,draws,*admitted)&&
                    !Handoff::commit(*queries,draws,std::move(*admitted)),"Held native lens pair failed one-shot atomic publication");
                ++accepted;
            }
            require(scope.current(),"Releasing Audio holds invalidated the genuine native scope prematurely");
        }
        require(renderer.take_lens_query_results().empty()&&queries->completed().empty(),
            "Held lens commit exposed old cached/pending GPU results");
        require(authority.execution_boundary()==sealed,"Held lens fixture changed the native target boundary");
        strict.reset(); // Uncommitted token cleanup is outside all publication holds.
#ifdef HALO2_VULKAN_LENS_RETIREMENT_OBSERVER
        begin_vulkan_lens_retirement_observation();
#endif
        admitted.reset(); // After every native/timing/SDL hold, on actual renderer executor.
#ifdef HALO2_VULKAN_LENS_RETIREMENT_OBSERVER
        const auto retired=end_vulkan_lens_retirement_observation();
        require(retired.waits==1&&retired.buffers==1&&retired.memory==1&&retired.pipelines==1&&
            retired.layouts==1&&retired.pools==2&&retired.query_reads==0,
            "Held lens retired GPU resources outside the actual deferred owner boundary");
        observed_retirement=true;
#endif
        require(mesh.materials.size()==material_count,"Held lens handoff changed resident authored material identities");
    }
    const auto validation=diagnostics.str();
    if(validation.find("VUID-")!=std::string::npos||validation.find("Validation Error")!=std::string::npos){
        std::cerr<<validation;throw FormatError("Held native lens component failed Vulkan validation");
    }
    require(authority.execution_boundary()==sealed,"Held lens renderer teardown changed native target state");
    std::cout<<"PASS held native lens boundary: genuine full target/cut/Boundary, default/foreign/expired/replacement refusal, strict unbound ring, draw/renderer revision and query identity rejection, real GPU preservation="
        <<preserved<<", paired commits="<<accepted<<", deferred Vulkan retirement="<<observed_retirement
        <<", strict raw ring views="<<strict_ring_views<<", bound raw ring views="<<bound_ring_views
        <<", actual native/timing/SDL held publication\n";
    std::cout<<"LIMIT preborrowed const memory reads remain allowed; private Xvfb/software-GPU component and empty-bank Manual Audio holds; no native VM/Match mutation, new world, full Ready, visible flare/playable migration or hardware acceptance\n";
}
}
