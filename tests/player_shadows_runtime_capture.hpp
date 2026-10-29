#pragma once
#include "halo2/desktop_session.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_snapshot_origin.hpp"
#include "halo2/source_live_shadow_selection_tu15.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <iostream>
#include <optional>
namespace halo2::shadow_test {
struct Capture {
    std::string mode;
    std::filesystem::path picture;
    unsigned tick=180;
    unsigned expected_players{};
    bool exact_tick=true,joining=true;
    std::optional<unsigned> local_slot;
    std::function<void()> advance,finish,after_capture;
    std::function<bool()> capture_ready;
};
inline void need(bool value,const char *why){if(!value)throw FormatError(why);}
inline nlohmann::json capture(MultiplayerOptions options,const Capture& test) {
    using Clock=std::chrono::steady_clock;
    const auto& mode=test.mode;nlohmann::json report;
    need(!options.capture_tick,"Network shadow helper requires actual frame_capture without a capture-tick override");
    need(options.local_players==1&&options.prepared_world,"Network GPU shadow proof requires one actual local view");
    need(bool(options.prepared_world->source_objects)!=test.joining,"Network shadow helper has the wrong gameplay authority owner");
        options.desktop=std::make_shared<DesktopSession>();
        options.desktop->display_settings.graphics_mode=mode=="Original"?pstencil::GraphicsMode::Original:
            mode=="pStencil"?pstencil::GraphicsMode::pStencil:pstencil::GraphicsMode::PathTracing;
        options.desktop->keyboard_settings.sensitivity=3;options.desktop->keyboard_settings.invert=false;
        auto *window=options.desktop->window(640,480,false);
        SDL_RaiseWindow(window);SDL_SetWindowInputFocus(window);
        need(SDL_SetRelativeMouseMode(SDL_TRUE)==0,"Joined shadow check could not capture actual mouse input");
        report["video_driver"]=SDL_GetCurrentVideoDriver()?SDL_GetCurrentVideoDriver():"";
        need(report["video_driver"]=="x11","Joined shadow GPU check must use its private X11 server");
        unsigned frames{},captured_tick{},last_tick{},local_slot=~0u,root_tick{},last_progress_tick{};std::uint32_t local_unit=~0u,local_player=~0u;
        bool looked{},look_confirmed{},capture_requested{},captured{},selected_local{},current_selected{},living{};
        std::uint64_t generate{},apply{};
        const auto started=Clock::now();
        auto diagnostic_at=started+std::chrono::seconds(5);
        const auto picture=test.picture;
        std::uint64_t ordinary_receipts{};unsigned ordinary_first{},ordinary_last{};
        std::uint32_t ordinary_sequence{};bool ordinary_sequence_set{};
        auto ordinary_begin=started,ordinary_end=started;
        nlohmann::json native_progress=nlohmann::json::array();
        options.replica_observer=[&](const MultiplayerState& state,const MultiplayerSnapshotOrigin& origin,bool advanced) {
            need(origin.tick()&&*origin.tick()==state.tick&&origin.sequence(),
                "Gameplay cadence observation lost its actual accepted body origin");
            if(ordinary_sequence_set&&*origin.sequence()!=ordinary_sequence)
                need(std::int32_t(*origin.sequence()-ordinary_sequence)>0,
                    "Historical native delivery rolled ordinary gameplay sequence backwards");
            ordinary_sequence=*origin.sequence();ordinary_sequence_set=true;
            if(advanced) {
                if(!ordinary_receipts){ordinary_first=state.tick;ordinary_begin=Clock::now();}
                ++ordinary_receipts;ordinary_last=state.tick;ordinary_end=Clock::now();
            }
        };
        // The fixed maximum keeps this a finite capture. The frame callback
        // requests the actual settled tick instead of relabeling a snapshot.
        options.frames=1800;
        options.input_observer=[&](auto,std::span<const Controls> input,const MultiplayerState& state) {
            if(test.advance)test.advance();last_tick=state.tick;
            living=false;
            for(unsigned slot=0;slot<state.players.size();++slot) {
                const auto& player=state.players[slot];
                if((test.local_slot?slot==*test.local_slot:bool(player.source_player_camera))&&
                    player.connected&&!player.dead&&player.source_player!=~0u&&player.source_unit!=~0u&&
                    (player.source_player&65535u)==slot) {
                    local_slot=slot;local_player=player.source_player;local_unit=player.source_unit;living=true;
                }
            }
            if(!looked&&!input.empty()&&local_unit!=~0u) {
                SDL_Event e{};e.type=SDL_MOUSEMOTION;e.motion.windowID=SDL_GetWindowID(window);
                e.motion.yrel=-int(std::lround((-1.5f-input[0].pitch)/.0025f));
                need(SDL_PushEvent(&e)==1,"Could not deliver actual joined down-look input");looked=true;
            }
            if(looked&&!input.empty()&&std::abs(input[0].pitch+1.5f)<=.01f)look_confirmed=true;
            if(Clock::now()>=diagnostic_at) {
                const auto connected=std::count_if(state.players.begin(),state.players.end(),[](const auto& p){return p.connected;});
                std::cerr<<"Shadow GPU client progress tick="<<state.tick<<" submitted_frames="<<frames
                    <<" connected="<<connected<<" local_slot="<<local_slot<<" player="<<local_player
                    <<" unit="<<local_unit<<" living="<<living<<" look_queued="<<looked
                    <<" look_observed="<<look_confirmed<<" input_pitch="<<(input.empty()?0.f:input[0].pitch)<<std::endl;
                diagnostic_at=Clock::now()+std::chrono::seconds(5);
            }
            need(Clock::now()-started<std::chrono::seconds(150),"Joined player shadow never reached its finite capture boundary");
        };
        options.shadow_root_frame_observer=[&](auto,auto tick,std::span<const SourceLiveShadowRootTu15> roots) {
            if(tick!=last_progress_tick) {
                last_progress_tick=tick;
                if(native_progress.size()==64)native_progress.erase(native_progress.begin());
                native_progress.push_back({{"tick",tick},{"elapsed_ms",std::chrono::duration<double,std::milli>(Clock::now()-started).count()}});
            }
            current_selected=false;root_tick=tick;
            for(const auto &root:roots)if(root.owner==local_unit&&root.commands&&root.uniforms)
                current_selected=true;

        };
        options.frame_capture=[&](const MultiplayerState& state)->std::filesystem::path {
            if(!capture_requested&&(test.exact_tick?state.tick==test.tick:state.tick>=test.tick)&&frames>=90&&look_confirmed&&living&&(test.exact_tick?root_tick==state.tick:root_tick>=test.tick&&root_tick<=state.tick)&&(!test.capture_ready||test.capture_ready())) {
                need(local_slot<16&&state.players[local_slot].source_player==local_player&&
                    state.players[local_slot].source_unit==local_unit&&!state.players[local_slot].dead,
                    "Network capture lost its actual living local full-salt identity");
                nlohmann::json players=nlohmann::json::array();
                for(unsigned slot=0;slot<state.players.size();++slot) {
                    const auto& player=state.players[slot];
                    if(player.connected&&!(player.source_flags&2u))players.push_back({{"slot",slot},
                        {"player",player.source_player},{"unit",player.source_unit},{"alive",!player.dead}});
                }
                if(test.expected_players)need(players.size()==test.expected_players,
                    "Network GPU capture lost a genuinely admitted active participant");
                report["players"]=std::move(players);
                capture_requested=true;captured_tick=state.tick;selected_local=root_tick<=state.tick&&current_selected;return picture;
            }
            return {};
        };
        options.frame_observer=[&](const MultiplayerFrameTiming& frame) {
            frames+=frame.submitted;
            if(!capture_requested){current_selected=false;root_tick=0;}
            if(capture_requested&&frame.submitted&&!captured) {
                generate=frame.renderer.world_native_shadow_generate_prepared_draws;
                apply=frame.renderer.world_native_shadow_apply_prepared_draws;captured=true;
                if(test.after_capture)test.after_capture();
                SDL_Event quit{};quit.type=SDL_QUIT;need(SDL_PushEvent(&quit)==1,"Could not finish joined shadow capture");
            }
        };
        const auto result=run_multiplayer(options);if(test.finish)test.finish();
        report["return_code"]=result;report["frames"]=frames;report["tick"]=captured_tick;
        report["native_root_tick"]=root_tick;report["local_unit"]=local_unit;report["local_selected"]=selected_local;
        report["generate_draws"]=generate;report["apply_draws"]=apply;
        report["local_slot"]=local_slot;report["local_player"]=local_player;report["local_unit_alive"]=living;report["last_received_tick"]=last_tick;
        report["down_look_observed"]=look_confirmed;
        report["ordinary_gameplay"]={{"advanced_receipts",ordinary_receipts},{"first_tick",ordinary_first},
            {"last_tick",ordinary_last},{"last_sequence",ordinary_sequence},
            {"elapsed_seconds",std::chrono::duration<double>(ordinary_end-ordinary_begin).count()}};
        report["native_tick_progress"]=std::move(native_progress);
        report["joining_world_has_no_gameplay_authority"]=!options.prepared_world->source_objects;
        need(result==0&&captured&&captured_tick>=test.tick&&(!test.exact_tick||captured_tick==test.tick)&&local_unit!=~0u&&local_player!=~0u,
            "Joined shadow check did not capture an actual living local player");
        need(std::filesystem::file_size(picture)>640u*480u*3u,"Joined shadow capture is missing its actual RGB pixels");
        if(mode!="PathTracing")need(selected_local&&generate&&apply,
            "Joining living player never reached native Generate and Apply GPU submission");
    report["picture"]=test.picture.string();report["passed"]=true;
    report["retained_large_artifacts"]=nlohmann::json::array();return report;
}
}
