// Diagnostic source-gameplay repro; register only in an owned CMake snapshot.
#define SDL_MAIN_HANDLED
#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/particle_draw.hpp"
#include "halo2/effect_light.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/source_light_volume_draw.hpp"
#include "../tests/helpers/gameplay_fixture.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <fstream>
#include <iostream>
#include <future>
#include <set>
#include <thread>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
using Json=nlohmann::json;
void check(bool condition,const char *why){if(!condition)throw FormatError(why);}
std::set<std::uint32_t> lens_definitions(const Cache& cache,std::uint32_t effect) {
    std::set<std::uint32_t> out;
    const auto meta=cache.load_source_tag_metadata(effect);
    const Bytes root(meta.bytes_at(meta.source_address(),0x1c));
    if(!root.u32(0x14))return out;
    const Bytes events(meta.bytes_at(root.u32(0x18),std::size_t(root.u32(0x14))*0x38));
    for(unsigned e=0;e<root.u32(0x14);++e) {
        const auto at=e*0x38;
        if(!events.u32(at+0x18))continue;
        const Bytes parts(meta.bytes_at(events.u32(at+0x1c),std::size_t(events.u32(at+0x18))*0x38));
        for(unsigned p=0;p<events.u32(at+0x18);++p) {
            const auto group=parts.u32(p*0x38+8),tag=parts.u32(p*0x38+0x10);
            if(tag==null_datum)continue;
            if(group==0x6c656e73u)out.insert(tag);
            if(group==0x6c696768u) {
                const auto light=load_effect_light(cache,tag);
                if(light.lens!=null_datum)out.insert(light.lens);
            }
        }
    }
    return out;
}
}
int main(int argc,char **argv)try {
    check(argc==4 || argc==5,"Supply prepared MCC MAP REPORT CAPTURE [BANSHEE_SETUP_JSON]");
    check(std::getenv("HALO2_TEST_SCRATCH"),"Joined effect repro requires owned scratch");
    const bool banshee=argc==5;Json setup;
    if(banshee){std::ifstream file(argv[4]);check(bool(file),"Banshee source placement setup absent");file>>setup;}
    MultiplayerOptions options;options.map=argv[1];options.shared_maps=options.map.parent_path();
    options.local_players=2;options.starting_weapon=5;options.source_object_world=true;
    options.input_instances={-1,-2}; // Top KBM; other local view has no test input.
    options.width=800;options.height=600;options.frames=banshee?0:240;options.mute=true;
    options.reference_tick_cadence=true;options.screenshot=argv[3];
    // Actual source frag starting equipment, shared by both sides.
    options.variant.values.set(83,16);
    Json authority={{"grenade_definitions",Json::array()},{"detonations",Json::array()},
        {"source_light_births",Json::array()},{"projectile_lenses",Json::array()}};
    std::promise<unsigned> bound;auto port=bound.get_future();std::exception_ptr server_error;
    std::jthread server([&](std::stop_token stop) {
        bool ready=false;
        try {
            auto host_options=options;host_options.local_players=1;host_options.frames=0;host_options.input_instances.clear();
            MultiplayerPreparedWorld world(host_options);
            check(world.source_objects&&world.source_objects->owns_physics(),"Host lacks its actual TU1.5 world");
            world.bind_source_local_views(world.local_slots);
            const auto fingerprint=multiplayer_fingerprint(host_options.map,host_options.shared_maps,0,
                world.network_starting_weapon_identity(),host_options.gameplay_xbe);
            LanHost host(world.match,0,fingerprint,host_options.variant);
            world.source_objects->tu15_world().observe_effect_light_births([&](bool before,const auto &light,const auto &,const auto &) {
                if(!before&&light.light!=null_datum&&authority["source_light_births"].size()<128)
                    authority["source_light_births"].push_back({{"tick",light.source_tick},{"definition",light.definition},{"light",light.light}});
            });
            bound.set_value(host.port());ready=true;
            std::array<Controls,multiplayer_capacity> input{};
            std::uint64_t serial{};std::set<std::uint32_t> projectiles;
            std::optional<unsigned> placed,seated;bool airborne=false;std::set<unsigned> inspected_projectiles;
            auto next=std::chrono::steady_clock::now();
            for(unsigned tick=0;tick<900&&!stop.stop_requested();++tick) {
                host.receive(input);
                if(banshee) {
                    const auto state=world.match.state();const auto &pilot=state.players[1];
                    if(pilot.connected && pilot.source_unit!=null_datum) {
                        if(!placed) {
                            Spawn spawn;const auto point=setup.at("position").get<std::array<float,3>>();
                            spawn.position={point[0],point[1],point[2]};spawn.facing=setup.at("yaw");
                            test::place_player(world.match,1,spawn);
                            world.source_objects->tu15_world().set_object_position(pilot.source_unit,spawn.position);
                            placed=tick;
                        }
                        if(pilot.source_parent==null_datum) {
                            input[1].yaw=setup.at("yaw");input[1].pitch=0;
                            input[1].pickup=tick>=*placed+60;input[1].pickup_milliseconds=input[1].pickup?900:0;
                        }else {
                            if(!seated)seated=tick;
                            if(!airborne && tick>=*seated+90) {
                                const auto point=setup.at("airborne").get<std::array<float,3>>();
                                world.source_objects->tu15_world().set_object_position(pilot.source_parent,{point[0],point[1],point[2]});
                                authority["declared_original_B95D0_airborne_input"]={{"vehicle",pilot.source_parent},{"position",point},{"tick",tick}};
                                airborne=true;
                            }
                        }
                    }
                }
                world.match.step(input);
                world.source_objects->advance_effect_frame(tick_seconds);
                const auto state=world.match.state();
                for(const auto &p:state.grenade_projectiles)projectiles.insert(p.datum);
                if(banshee)for(const auto &p:state.weapon_projectiles) {
                    projectiles.insert(p.datum);
                    if(!inspected_projectiles.insert(p.definition).second)continue;
                    const auto meta=world.cache->load_source_tag_metadata(p.definition);
                    const Bytes root(meta.bytes_at(meta.source_address(),0x9c));
                    if(!root.u32(0x94))continue;
                    const Bytes attachments(meta.bytes_at(root.u32(0x98),root.u32(0x94)*0x18));
                    for(unsigned n=0;n<root.u32(0x94);++n)if(attachments.u32(n*0x18)==0x6c656e73u)
                        authority["projectile_lenses"].push_back(attachments.u32(n*0x18+4));
                }
                for(const auto &event:state.grenade_detonations)if(event.serial>serial) {
                    serial=event.serial;
                    const auto lenses=lens_definitions(*world.cache,event.effect);
                    authority["detonations"].push_back({{"tick",event.tick},{"projectile",event.projectile},
                        {"effect",event.effect},{"authored_lens_definitions",lenses},
                        {"source_locations",event.source_locations.size()}});
                }
                host.broadcast();next+=std::chrono::microseconds(33333);std::this_thread::sleep_until(next);
            }
            authority["actual_projectiles"]=projectiles;
            world.source_objects->tu15_world().observe_effect_light_births({});
        }catch(...){server_error=std::current_exception();if(!ready)bound.set_exception(server_error);}
    });
    options.join="127.0.0.1:"+std::to_string(port.get());
    options.desktop=std::make_shared<DesktopSession>();auto *window=options.desktop->window(800,600,false);
    SDL_RaiseWindow(window);SDL_SetWindowInputFocus(window);
    unsigned pulses{},frames{},particles{},fire_steps{},glows{};bool queued=false,fire_pressed=false,fire_released=false,quit=false;std::optional<unsigned> joined,client_seated;
    std::set<std::uint32_t> received_projectiles;
    std::set<std::uint32_t> effects;Json received=Json::array();std::uint64_t serial{};
    options.input_observer=[&](unsigned tick,std::span<const Controls> input,const MultiplayerState &state) {
        if(banshee) {
            const auto &pilot=state.players[1];
            if(pilot.connected && pilot.source_parent!=null_datum && !client_seated)client_seated=state.tick;
            if(client_seated && ((!fire_pressed && state.tick>=*client_seated+100) ||
                (fire_pressed && !fire_released && state.tick>=*client_seated+220))) {
                const bool down=!fire_pressed;
                if(down)fire_pressed=true;else fire_released=true;
                SDL_RaiseWindow(window);check(SDL_SetWindowInputFocus(window)==0,"Could not focus Banshee KBM view");
                SDL_Event event{};event.type=down?SDL_MOUSEBUTTONDOWN:SDL_MOUSEBUTTONUP;
                event.button.button=SDL_BUTTON_LEFT;event.button.windowID=SDL_GetWindowID(window);
                check(SDL_PushEvent(&event)==1,"Cannot queue joined Banshee fire");
                for(const auto key:std::array{SDL_SCANCODE_W,SDL_SCANCODE_SPACE}) {
                    SDL_Event flight{};flight.type=down?SDL_KEYDOWN:SDL_KEYUP;flight.key.windowID=SDL_GetWindowID(window);
                    flight.key.keysym.scancode=key;flight.key.keysym.sym=SDL_GetKeyFromScancode(key);
                    check(SDL_PushEvent(&flight)==1,"Cannot queue joined Banshee flight");
                }
            }
            if(!input.empty())fire_steps+=input[0].fire;
            for(const auto &p:state.weapon_projectiles)received_projectiles.insert(p.datum);
            if(client_seated && fire_released && !quit && state.tick>=*client_seated+250) {
                SDL_Event end{};end.type=SDL_QUIT;check(SDL_PushEvent(&end)==1,"Could not end finite source Banshee encounter");quit=true;
            }
            return;
        }
        if(!joined&&std::any_of(state.players.begin(),state.players.end(),[](const auto &p){return p.connected;}))joined=tick;
        if(joined&&!queued&&tick>=*joined+30) {
            SDL_RaiseWindow(window);check(SDL_SetWindowInputFocus(window)==0,"Could not focus joined KBM view");
            SDL_PumpEvents();check(SDL_GetKeyboardFocus()==window,"Joined KBM view lost OS focus");
            check(SDL_SetRelativeMouseMode(SDL_TRUE)==0,"Joined relative look unavailable");
            SDL_Event look{};look.type=SDL_MOUSEMOTION;look.motion.windowID=SDL_GetWindowID(window);look.motion.yrel=400;
            check(SDL_PushEvent(&look)==1,"Joined look input queue failed");
            SDL_Event key{};key.type=SDL_KEYDOWN;key.key.windowID=SDL_GetWindowID(window);
            key.key.keysym.scancode=SDL_SCANCODE_Q;key.key.keysym.sym=SDLK_q;
            check(SDL_PushEvent(&key)==1,"Joined grenade press queue failed");key.type=SDL_KEYUP;
            check(SDL_PushEvent(&key)==1,"Joined grenade release queue failed");queued=true;
        }
        if(!input.empty())pulses+=input[0].grenade;
        for(const auto &event:state.grenade_detonations)if(event.serial>serial) {
            serial=event.serial;effects.insert(event.effect);
            received.push_back({{"tick",event.tick},{"effect",event.effect},{"projectile",event.projectile}});
        }
    };
    options.effect_observer=[&](unsigned view,std::uint32_t,std::span<const ParticleDraw> draws) {
        if(!view)for(const auto &draw:draws)particles+=effects.contains(draw.effect_definition);
    };
    options.frame_observer=[&](const MultiplayerFrameTiming &frame){frames+=frame.submitted;};
    options.weapon_light_volume_observer=[&](unsigned view,std::uint32_t,const SourceLightVolumeDraw &draw) {
        if(!view && received_projectiles.contains(draw.object))++glows;
    };
    const auto result=run_multiplayer(options);
    server.request_stop();server.join();if(server_error)std::rethrow_exception(server_error);
    const bool actual=result==0&&(banshee?(fire_steps>0&&!received_projectiles.empty()&&glows>0):
        (pulses>0&&!received.empty()&&particles>0));
    Json report={{"runtime_status",result},{"local_players",2},{"top_input","keyboard/mouse"},
        {"actual_SDL_grenade_pulses",pulses},{"submitted_frames",frames},
        {"received_detonations",received},{"top_explosion_particle_draws",particles},
        {"authority",authority},{"actual_encounter_completed",actual},
        {"banshee",banshee},{"actual_SDL_fire_steps",fire_steps},{"received_projectiles",received_projectiles},
        {"top_actual_projectile_glow_draws",glows},
        {"declared_entry_setup",banshee?"Original B95D0 placement and normal source look/pickup controls on authority; subsequent firing comes from joined top-view SDL/UDP input":"none"},
        {"supplied_projectile_hit_lifetime_visibility_or_GPU_result",false},
        {"original_Xbox_pixels_compared",false}};
    std::ofstream(argv[2])<<report.dump(2)<<'\n';
    check(actual,"Joined source grenade encounter did not complete its actual presentation");return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
