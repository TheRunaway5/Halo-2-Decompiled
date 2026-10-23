#pragma once
#include "halo2/cache.hpp"
#include "halo2/multiplayer_audio.hpp"
#include "halo2/source_bsp_effect_lights.hpp"
#include "halo2/source_foreground_world_handoff.hpp"
#include "halo2/source_peer_host_takeover.hpp"
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <thread>
#include <utility>

namespace halo2::test {
// A local routing/collision subtransaction on the already genuine candidate.
// These separate slots never publish the caller's live runtime bindings and
// establish neither full foreground readiness nor migration activation.
inline void held_foreground_controls(SourcePeerHostTakeover &candidate,
    const EffectPresentationHistory::RuntimeCheckpoint &checkpoint,
    std::shared_ptr<const Cache> resident_cache,Mesh &mesh,
    const SourceWorldParticleDrawsTu15 &particles,const SourceBspEffectLights &models,
    std::size_t uploaded_materials) {
    const auto require=[](bool value,const char *why){if(!value)throw FormatError(why);};
    const auto refuses=[](auto run){try{run();}catch(const FormatError &){return true;}return false;};
    // Manual delivery opens an actual SDL endpoint. Refuse before creating it
    // unless the enclosing selected-MCC CTest declared its synthetic driver.
    const auto driver=std::getenv("SDL_AUDIODRIVER");
    require(driver&&std::string_view(driver)=="dummy","Held foreground fixture requires SDL dummy audio");
    auto &target=candidate.world();const auto cache=target.cache;
    require(cache&&cache->is_mcc()&&target.source_objects,
        "Held foreground fixture requires its actual selected-MCC native candidate");
    auto *const native=target.source_objects.get();
    auto provider=native->tu15_world().sound_variation_provider(*cache);
    SourceMatchAuthority authority(target.match,*native);const auto before=authority.execution_boundary();
    MultiplayerObjectRuntime *particle_route{},*model_route{};
    SourceForegroundWorldHandoff foreground(std::move(resident_cache),mesh,particles,models,
        uploaded_materials,{true,true},particle_route,model_route);
    auto expired=foreground.prepare_host(candidate,checkpoint);
    require(foreground.ready(expired)&&!particle_route&&!model_route,
        "Actual foreground preparation changed its separate outgoing routes");
    SourceNativePublication::Boundary first;
    {
        auto scope=candidate.hold_native_publication();first=scope.boundary();const auto copy=first;
        require(scope.current()&&first.current()&&first.same_scope(copy),
            "Genuine native scope did not mint its exact weak boundary");
        require(!foreground.ready(expired)&&!foreground.commit(expired),
            "Unbound foreground bypassed the ordinary native publication guard");
        require(!foreground.bind_publication(expired,SourceNativePublication::Boundary{}),
            "Default native boundary authorized foreground publication");
        require(foreground.bind_publication(expired,first)&&foreground.bind_publication(expired,copy)&&
            foreground.ready(expired),"Actual same-scope foreground binding failed or lost idempotence");
        bool foreign_refused{};
        std::thread foreign([&]{foreign_refused=!scope.current()&&!first.current()&&
            !foreground.bind_publication(expired,first)&&!foreground.ready(expired)&&!foreground.commit(expired);});
        foreign.join();require(foreign_refused,"Held foreground accepted a foreign executor");
        require(refuses([&]{(void)candidate.world();})&&refuses([&]{(void)authority.execution_boundary();}),
            "Held foreground weakened the normal target or full native-boundary getter");
        require(foreground.ready(expired)&&!particle_route&&!model_route,
            "Rejected held/foreign operations consumed foreground preparation");
    }
    require(!first.current()&&!foreground.ready(expired)&&!foreground.commit(expired)&&
        refuses([&]{(void)expired.muzzle_collision();})&&refuses([&]{(void)expired.world_collision();})&&
        !particle_route&&!model_route,"Released native scope left a foreground token or query getter eligible");
    {
        auto scope=candidate.hold_native_publication();const auto replacement=scope.boundary();
        require(replacement.current()&&!first.same_scope(replacement)&&
            !foreground.bind_publication(expired,replacement)&&!foreground.ready(expired)&&!foreground.commit(expired),
            "New scope on the same native target revived a previously bound foreground token");
    }
    require(authority.execution_boundary()==before,"Held foreground refusal changed the actual full native boundary");
    auto prepared=foreground.prepare_host(candidate,checkpoint);
    require(foreground.ready(prepared),"Fresh genuine foreground preparation failed after old scope retirement");
    // Empty playback still uses the actual retained native metadata owner and
    // queued SDL platform. Playing PCM/script continuation has separate tests.
    const SdlSoundPlatformConfiguration manual{SdlSoundPlatformConfiguration::Delivery::Manual,256,{}};
    MultiplayerAudio audio(cache,0x8b5a0217u,manual);audio.enroll_continuation();
    audio.bind_native_sound_metadata(provider);const auto audio_checkpoint=audio.checkpoint();
    const auto physical=audio.mixed_frames();
    require(audio_checkpoint.voices()==0,"Held foreground Audio control unexpectedly owns playback");
    {
        auto scope=candidate.hold_native_publication();const auto boundary=scope.boundary();
        require(foreground.bind_publication(prepared,boundary)&&foreground.ready(prepared),
            "Fresh foreground token did not bind its actual native scope");
        particle_route=native;
        require(!foreground.ready(prepared)&&!foreground.commit(prepared)&&
            !foreground.bind_publication(prepared,boundary)&&!model_route,
            "Intervening local routing change published or consumed foreground eligibility");
        particle_route=nullptr;
        require(foreground.ready(prepared)&&foreground.bind_publication(prepared,boundary),
            "Rejected routing change consumed the same actual foreground preparation");
        const auto audio_binding=provider.native_binding(scope,cache);
        auto audio_prepared=audio.prepare_restore(audio_checkpoint,audio_binding);
        {
            // Retain the genuine native scope first; Audio acquires timing,
            // then queued SDL. Foreground ready/commit perform no native read.
            auto held=audio.hold_publication(audio_prepared);
            require(audio.ready(audio_prepared,held)&&scope.current()&&foreground.ready(prepared),
                "Actual native/timing/SDL holds rejected the bound foreground witness");
            require(refuses([&]{(void)candidate.world();})&&refuses([&]{(void)authority.execution_boundary();}),
                "Held foreground commit reopened ordinary native getters");
            require(audio.begin_commit(audio_prepared,held)&&foreground.ready(prepared)&&foreground.commit(prepared)&&
                audio.commit(audio_prepared,held),"Actual held foreground/empty Audio publication failed");
            require(particle_route==native&&model_route==native&&!foreground.ready(prepared)&&
                !foreground.commit(prepared)&&!audio.commit(audio_prepared,held),
                "Consumed held foreground or Audio token published twice");
        }
        require(scope.current(),"Audio hold release retired its enclosing actual native scope");
    }
    require(authority.execution_boundary()==before&&audio.mixed_frames()==physical&&audio.voices()==0&&
        particle_route==native&&model_route==native,
        "Held foreground publication changed native state or physical output instead of only local routes");
    std::cout<<"PASS held native foreground boundary: genuine same-scope binding, default/foreign/expired/reacquired refusal, routing retry, native/timing/SDL local commit and unchanged full native boundary\n";
}
}
