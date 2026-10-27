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
#include "halo2/native_unit_interaction.hpp"
#include "halo2/original_data.hpp"
#include "halo2/raw_datum_pool.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/scene.hpp"
#include "halo2/kernels.hpp"
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
    const Bytes events(cache.read_tag_bytes(root.u32(0x18),std::size_t(root.u32(0x14))*0x38));
    for(unsigned e=0;e<root.u32(0x14);++e) {
        const auto at=e*0x38;
        if(!events.u32(at+0x18))continue;
        const Bytes parts(cache.read_tag_bytes(events.u32(at+0x1c),std::size_t(events.u32(at+0x18))*0x38));
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
    const bool rocket=std::getenv("HALO2_JOINED_ROCKET")!=nullptr;
    const bool needler=std::getenv("HALO2_JOINED_NEEDLER")!=nullptr;
    const bool spark=std::getenv("HALO2_JOINED_MAP_SPARK")!=nullptr;
    const bool projectile_weapon=rocket||needler;
    check(!rocket||!needler,"Projectile weapon fixtures must remain distinct");
    const bool airburst=rocket&&std::getenv("HALO2_JOINED_ROCKET_AIRBURST")!=nullptr;
    check(!projectile_weapon||!banshee,"Projectile weapon and vehicle fixtures must remain distinct");
    check(!spark||(!projectile_weapon&&!banshee),"Scenery and projectile fixtures must remain distinct");
    const bool server_only=std::getenv("HALO2_JOINED_SERVER_ONLY")!=nullptr;
    check(!server_only||banshee||projectile_weapon||spark,"Standalone authority fixture requires its declared encounter");
    const std::filesystem::path report_path=argv[2];
    const bool declared_seat=banshee && std::getenv("HALO2_JOINED_BANSHEE_DECLARED_SEAT");
    if(banshee){std::ifstream file(argv[4]);check(bool(file),"Banshee source placement setup absent");file>>setup;}
    MultiplayerOptions options;options.map=argv[1];options.shared_maps=options.map.parent_path();
    options.local_players=2;options.source_object_world=true;
    // Advertise the effective source variant, as the production host does.
    // The joined client adopts this write before computing its fingerprint.
    options.variant.values.set(89,5);
    unsigned starting_choice=5;
    std::uint32_t spark_definition=null_datum;
    std::unique_ptr<Cache> spark_authored;
    if(spark) {
        spark_authored=std::make_unique<Cache>(options.map,options.shared_maps,OriginalData::native_tu15());
        auto &authored=*spark_authored;
        const auto *tag=authored.query_tag({.group=TagGroup::from_string("scen"),
            .name="effects\\scenery\\sparks\\spark_iceblue_large",.require_address=true});
        check(tag,"Actual map lacks the authored spark scenery");spark_definition=tag->datum;
    }
    if(projectile_weapon) {
        Cache authored(options.map,options.shared_maps,OriginalData::native_tu15());
        bool found=false;
        for(unsigned choice=3;choice<=20;++choice) {
            const auto identity=authored.slayer_starting_weapon(choice);
            const auto tag=authored.query_tag({.handle=TagHandle{identity},.require_address=true});
            const std::string expected=needler?"objects\\weapons\\pistol\\needler\\needler":
                "objects\\weapons\\support_high\\rocket_launcher\\rocket_launcher";
            if(!tag||tag->group!="weap"||tag->name!=expected)continue;
            options.variant.values.set(89,choice);starting_choice=choice;found=true;break;
        }
        check(found,"Actual multiplayer weapon bank lacks the requested projectile weapon");
    }
    options.input_instances={-1,-2}; // Top KBM; other local view has no test input.
    options.width=800;options.height=600;options.frames=0;options.mute=true;
    options.screenshot=argv[3];
    // Ordinary Slayer equipment. Parameter83 is a map weapon-set override;
    // it is unnecessary for Banshee fire and requires a separate client owner.
    Json authority={{"grenade_definitions",Json::array()},{"detonations",Json::array()},
        {"source_light_births",Json::array()},{"projectile_lenses",Json::array()}};
    std::promise<std::string> bound;auto port=bound.get_future();std::exception_ptr server_error;
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
            bound.set_value(host.join_address());ready=true;
            std::array<Controls,multiplayer_capacity> input{};
            std::uint64_t serial{};std::set<std::uint32_t> projectiles;
            std::optional<unsigned> placed,seated;bool airborne=false;std::set<unsigned> inspected_projectiles;
            auto next=std::chrono::steady_clock::now();const auto started=next;
            unsigned tick{},received_fire_steps{};
            // Keep the real authority alive through client graphics startup.
            // The wrapper still owns a finite wall deadline; no simulation,
            // snapshot, input or completed rendering result is supplied.
            for(;tick<27000&&!stop.stop_requested()&&
                std::chrono::steady_clock::now()-started<std::chrono::minutes(15);++tick) {
                host.receive(input);
                received_fire_steps+=input[1].fire;
                if(spark&&!placed) {
                    const auto state=world.match.state();const auto &pilot=state.players[1];
                    if(pilot.connected&&pilot.source_unit!=null_datum)
                        for(const auto &root:state.source_scene_roots)if(root.definition==spark_definition) {
                            Spawn spawn;spawn.facing=pilot.yaw;
                            std::optional<Transform> marker;
                            const auto nodes=world.match.source_world_nodes(root.object);
                            for(const auto &effect:root.attached_effects)for(const auto &location:effect.locations)
                                if(world.cache->string_id(location.name)=="marker") {
                                    if(location.node==-1)marker=location.local;
                                    else {
                                        check(location.node>=0&&unsigned(location.node)<nodes.size(),"Spark marker lost its actual WORLD node");
                                        marker=compose_transform(nodes[unsigned(location.node)],location.local);
                                    }
                                    break;
                                }
                            check(marker.has_value(),"Spark approach requires its actual authored effect marker");
                            const CollisionWorld terrain(world.content.collision);
                            authority["spark_authored_marker"]={{"position",{marker->position.x,marker->position.y,marker->position.z}},
                                {"forward",{marker->basis[0].x,marker->basis[0].y,marker->basis[0].z}}};
                            bool supported=false;
                            for(const auto direction:std::array{marker->basis[1],marker->basis[1]*-1.f,marker->basis[2],marker->basis[2]*-1.f,marker->basis[0],marker->basis[0]*-1.f}) {
                                for(const float distance:std::array{3.f,1.5f,.75f}) {
                                    const auto point=marker->position+direction*distance;
                                    const auto floor=terrain.raycast(point+Vec3{0,0,.1f},{0,0,-1},16);
                                    if(!floor)continue;
                                    spawn.position=point;spawn.position.z=floor->position.z+world.content.player.radius;
                                    const auto eye=spawn.position+Vec3{0,0,world.content.player.height*.75f};
                                    const auto delta=marker->position-eye;
                                    const auto obstruction=terrain.raycast(eye,delta*(1.f/length(delta)),length(delta)-.05f);
                                    authority["spark_approach_candidates"].push_back({{"position",{spawn.position.x,spawn.position.y,spawn.position.z}},
                                        {"blocked",bool(obstruction)}});
                                    if(obstruction)continue;
                                    supported=true;break;
                                }
                                if(supported)break;
                            }
                            check(supported,"Declared spark approach has no collision-supported line of sight");
                            test::place_player(world.match,1,spawn);
                            world.source_objects->tu15_world().set_object_position(pilot.source_unit,spawn.position);
                            authority["declared_spark_B95D0_approach"]={{"object",root.object},
                                {"position",{spawn.position.x,spawn.position.y,spawn.position.z}},{"tick",tick},
                                {"marker",{marker->position.x,marker->position.y,marker->position.z}},
                                {"marker_forward",{marker->basis[0].x,marker->basis[0].y,marker->basis[0].z}}};
                            placed=tick;break;
                        }
                }
                if(banshee) {
                    const auto state=world.match.state();const auto &pilot=state.players[1];
                    if(pilot.connected && pilot.source_unit!=null_datum) {
                        if(!placed) {
                            // Network startup outlasts vehicle settling. Resolve
                            // the live WORLD entry marker rather than reuse a
                            // cold authored approach after the vehicle moved.
                            const auto definition=setup.at("definition").get<std::uint32_t>();
                            const auto &asset=world.content.unit_interactions->units.at(definition);
                            const auto &seat=asset.seats.at(setup.at("seat").get<unsigned>());
                            std::optional<Vec3> approach,entry;std::uint32_t entry_vehicle=null_datum;float best=1e30f;
                            const auto cold=setup.at("position").get<std::array<float,3>>();
                            for(const auto &object:world.source_objects->live_objects()) {
                                if(object.definition!=definition||object.type!=1)continue;
                                const auto nodes=world.match.source_world_nodes(object.object);
                                const auto markers=pose_model_markers(asset.markers.at(seat.entry_marker),nodes,asset.permutations,{},1);
                                if(markers.empty())continue;
                                auto point=markers.front().world.position+markers.front().world.basis[0]*(seat.radius*.5f);
                                const CollisionWorld terrain(world.content.collision);
                                const auto floor=terrain.raycast(point+Vec3{0,0,2*world.content.player.height},{0,0,-1},4*world.content.player.height);
                                if(!floor)continue;
                                point.z=floor->position.z+world.content.player.radius;
                                const auto distance=length(point-Vec3{cold[0],cold[1],cold[2]});
                                if(distance>=best)continue;
                                best=distance;approach=point;entry=markers.front().world.position;entry_vehicle=object.object;
                            }
                            check(approach&&entry,"No live Banshee WORLD entry marker approach");
                            Spawn spawn;spawn.position=*approach;
                            const auto delta=*entry-(spawn.position+Vec3{0,0,world.content.player.camera_height});
                            spawn.facing=std::atan2(delta.y,delta.x);
                            test::place_player(world.match,1,spawn);
                            world.source_objects->tu15_world().set_object_position(pilot.source_unit,spawn.position);
                            authority["live_entry_input"]={{"position",{spawn.position.x,spawn.position.y,spawn.position.z}},
                                {"yaw",spawn.facing},{"pitch",std::atan2(delta.z,std::hypot(delta.x,delta.y))},
                                {"vehicle",entry_vehicle},{"seat",setup.at("seat")}};
                            placed=tick;
                        }
                        if(pilot.source_parent==null_datum) {
                            input[1].yaw=authority["live_entry_input"]["yaw"];
                            input[1].pitch=authority["live_entry_input"]["pitch"];
                            // Match's existing typed external-input boundary,
                            // also used by native_vehicle_camera_tu15_tests.
                            // This requests the actual authored seat/live full
                            // generation; original source admission still runs.
                            input[1].vehicle_interaction_target=declared_seat?
                                SourcePlayerActionTarget{5,setup.at("seat").get<std::int16_t>(),
                                    authority["live_entry_input"]["vehicle"].get<std::uint32_t>()}:
                                pilot.action_prompt.target;
                            input[1].pickup=tick>=*placed+60;input[1].pickup_milliseconds=input[1].pickup?900:0;
                        }else {
                            input[1].pickup=false;input[1].pickup_milliseconds=0;input[1].vehicle_interaction_target.reset();
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
                if(banshee&&placed&&authority["entry_samples"].size()<24&&tick%15==0) {
                    const auto &pilot=world.match.state().players[1];
                    authority["entry_samples"].push_back({{"tick",tick},{"unit",pilot.source_unit},
                        {"parent",pilot.source_parent},{"prompt_target",pilot.action_prompt.target.object},
                        {"prompt_action",pilot.action_prompt.target.action},
                        {"position",{pilot.position.x,pilot.position.y,pilot.position.z}}});
                }
                world.source_objects->advance_effect_frame(tick_seconds);
                const auto state=world.match.state();
                if(tick%15==0) {
                    authority["last_tick"]=tick;authority["last_received_fire_steps"]=received_fire_steps;
                    authority["last_live_weapon_projectiles"]=state.weapon_projectiles.size();
                }
                if(server_only&&tick%15==0) {
                    const auto &pilot=state.players[1];
                    Json progress{{"tick",tick},{"connected",pilot.connected},{"unit",pilot.source_unit},
                        {"parent",pilot.source_parent},{"airborne_setup_complete",airborne},
                        {"spark_setup_complete",spark&&placed.has_value()},
                        {"received_fire_steps",received_fire_steps},{"projectiles",state.weapon_projectiles.size()},
                        {"detonations",state.grenade_detonations.size()}};
                    if(spark&&placed) {
                        const auto eye=ordinary_unit_eye(pilot.position,world.content.player.camera_height,
                            world.content.player.crouch_camera_height,pilot.crouch);
                        progress["pilot_eye"]={eye.x,eye.y,eye.z};progress["pilot_yaw"]=pilot.yaw;progress["pilot_pitch"]=pilot.pitch;
                        progress["spark_marker"]=authority["declared_spark_B95D0_approach"]["marker"];
                        progress["spark_placement_tick"]=*placed;
                    }
                    const auto pending=report_path.string()+".progress.tmp",published=report_path.string()+".progress.json";
                    std::ofstream(pending)<<progress.dump();std::filesystem::rename(pending,published);
                }
                for(const auto &p:state.grenade_projectiles)projectiles.insert(p.datum);
                if(banshee||projectile_weapon)for(const auto &p:state.weapon_projectiles) {
                    projectiles.insert(p.datum);
                    if(!inspected_projectiles.insert(p.definition).second)continue;
                    const auto meta=world.cache->load_source_tag_metadata(p.definition);
                    const Bytes root(meta.bytes_at(meta.source_address(),0x9c));
                    if(!root.u32(0x94))continue;
                    const Bytes attachments(world.cache->read_tag_bytes(root.u32(0x98),root.u32(0x94)*0x18));
                    for(unsigned n=0;n<root.u32(0x94);++n)if(attachments.u32(n*0x18)==0x6c656e73u)
                        authority["projectile_lenses"].push_back(attachments.u32(n*0x18+4));
                }
                for(const auto &event:state.grenade_detonations)if(event.serial>serial) {
                    serial=event.serial;
                    const auto lenses=lens_definitions(*world.cache,event.effect);
                    Json render_prefix=nullptr;
                    const auto &memory=world.source_objects->tu15_world().memory();
                    const auto word=[&](unsigned at){return Bytes(memory.view(at,4)).u32(0);};
                    const auto pool=word(0x4f720cu);
                    check(word(pool+0x20u)==256&&word(pool+0x24u)==400,"Actual effect pool changed its original layout");
                    const SourceRawDatumPoolServices storage{.read=[&](auto at,auto n){return memory.view(at,n);}};
                    const auto instance=event.source_effect_instance;
                    if(instance!=null_datum)if(const auto body=source_raw_datum_lookup(pool,instance,storage)) {
                        const auto index=instance&65535u;
                        if(word(word(pool+0x48u)+(index/32u)*4u)&(1u<<(index%32u))) {
                            check(word(body+4u)==event.effect,"Actual live effect definition differs from its event");
                            const Bytes b(memory.view(body,400));
                            render_prefix={{"identity",instance},{"flags",b.u16(2)},{"attached_owner",b.u32(0x48)},
                                {"local_view",b.u32(0x58)},{"event",b.u16(0x5c)},{"elapsed",b.f32(0x60)},
                                {"duration",b.f32(0x64)},{"render_color",{b.f32(0x24),b.f32(0x28),b.f32(0x2c)}}};
                        }
                    }
                    authority["detonations"].push_back({{"tick",event.tick},{"projectile",event.projectile},
                        {"effect",event.effect},{"authored_lens_definitions",lenses},
                        {"source_locations",event.source_locations.size()},{"source_post_step_render_prefix",render_prefix}});
                }
                host.broadcast();next+=std::chrono::microseconds(33333);std::this_thread::sleep_until(next);
            }
            authority["actual_projectiles"]=projectiles;
            authority["actual_ticks"]=tick;
            world.source_objects->tu15_world().observe_effect_light_births({});
        }catch(...){
            server_error=std::current_exception();
            try{std::rethrow_exception(server_error);}catch(const std::exception &e){authority["failure"]=e.what();}catch(...){authority["failure"]="Unknown authority exception";}
            std::ofstream(report_path.string()+".authority-failure.json")<<authority.dump(2)<<'\n';
            if(!ready)bound.set_exception(server_error);
        }
    });
    options.join=port.get();
    if(server_only) {
        std::ofstream(report_path.string()+".ready.json")<<Json{{"join",options.join},{"declared_typed_seat_request",declared_seat}}.dump();
        const auto deadline=std::chrono::steady_clock::now()+std::chrono::minutes(10);
        while(!std::filesystem::exists(report_path.string()+".stop")&&std::chrono::steady_clock::now()<deadline)
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        server.request_stop();server.join();if(server_error)std::rethrow_exception(server_error);
        std::ofstream(report_path)<<Json{{"authority",authority},{"server_only",true},
            {"rocket",rocket},{"needler",needler},{"spark",spark},{"starting_weapon_option",starting_choice},
            {"declared_typed_seat_request",declared_seat},{"projectile_admission_hit_lifetime_or_GPU_result_supplied",false}}.dump(2)<<'\n';
        return 0;
    }
    options.source_object_world=false; // The joined peer owns no authority.
    options.desktop=std::make_shared<DesktopSession>();auto *window=options.desktop->window(800,600,false);
    SDL_RaiseWindow(window);SDL_SetWindowInputFocus(window);
    unsigned pulses{},frames{},particles{},fire_steps{},glows{};bool queued=false,fire_pressed=false,fire_released=false,quit=false;std::optional<unsigned> joined,client_seated,detonated;
    std::set<std::uint32_t> received_projectiles;
    std::set<std::uint32_t> effects;Json received=Json::array();std::uint64_t serial{};
    unsigned spark_snapshots{},spark_mouse_events{};
    options.input_observer=[&](unsigned tick,std::span<const Controls> input,const MultiplayerState &state) {
        if(spark) {
            const auto &pilot=state.players[1];
            if(pilot.connected&&pilot.source_unit!=null_datum&&!joined)joined=state.tick;
            for(const auto &root:state.source_scene_roots)if(root.definition==spark_definition)
                for(const auto &effect:root.attached_effects)if(effect.render) {
                    effects.insert(effect.definition);++spark_snapshots;
                }
            if(joined&&!quit&&state.tick>=*joined+750) {
                SDL_Event end{};end.type=SDL_QUIT;check(SDL_PushEvent(&end)==1,"Could not end finite scenery encounter");quit=true;
            }
            return;
        }
        if(banshee||projectile_weapon) {
            const auto &pilot=state.players[1];
            if(!joined&&pilot.connected&&pilot.source_unit!=null_datum)joined=state.tick;
            if(pilot.connected && (projectile_weapon||pilot.source_parent!=null_datum) && !client_seated)client_seated=state.tick;
            if(client_seated && ((!fire_pressed && state.tick>=*client_seated+100) ||
                (fire_pressed && !fire_released && state.tick>=*client_seated+220))) {
                const bool down=!fire_pressed;
                if(down)fire_pressed=true;else fire_released=true;
                SDL_RaiseWindow(window);check(SDL_SetWindowInputFocus(window)==0,"Could not focus Banshee KBM view");
                SDL_Event event{};event.type=down?SDL_MOUSEBUTTONDOWN:SDL_MOUSEBUTTONUP;
                event.button.button=SDL_BUTTON_LEFT;event.button.windowID=SDL_GetWindowID(window);
                check(SDL_PushEvent(&event)==1,"Cannot queue joined Banshee fire");
                if(projectile_weapon&&down) {
                    check(SDL_SetRelativeMouseMode(SDL_TRUE)==0,"Joined rocket relative look unavailable");
                    SDL_Event look{};look.type=SDL_MOUSEMOTION;look.motion.windowID=SDL_GetWindowID(window);look.motion.yrel=airburst?-800:needler?120:20;
                    check(SDL_PushEvent(&look)==1,"Cannot queue joined rocket look");
                }
                if(banshee)for(const auto key:std::array{SDL_SCANCODE_W,SDL_SCANCODE_SPACE}) {
                    SDL_Event flight{};flight.type=down?SDL_KEYDOWN:SDL_KEYUP;flight.key.windowID=SDL_GetWindowID(window);
                    flight.key.keysym.scancode=key;flight.key.keysym.sym=SDL_GetKeyFromScancode(key);
                    check(SDL_PushEvent(&flight)==1,"Cannot queue joined Banshee flight");
                }
            }
            if(!input.empty())fire_steps+=input[0].fire;
            for(const auto &p:state.weapon_projectiles)received_projectiles.insert(p.datum);
            if(projectile_weapon)for(const auto &event:state.grenade_detonations)if(event.serial>serial) {
                serial=event.serial;effects.insert(event.effect);
                Json row{{"tick",event.tick},{"effect",event.effect},{"projectile",event.projectile},
                    {"local_pool_identity",event.source_effect_instance},{"replica_render_present",event.replica_render.has_value()}};
                if(event.replica_render)row["replica_render"]={{"identity",event.replica_render->identity},
                    {"color",{event.replica_render->color.x,event.replica_render->color.y,event.replica_render->color.z}},
                    {"velocity",{event.replica_render->velocity.x,event.replica_render->velocity.y,event.replica_render->velocity.z}},
                    {"flags",event.replica_render->flags},{"attached_owner",event.replica_render->attached_owner},{"local_view",event.replica_render->local_view}};
                received.push_back(row);
            }
            if(client_seated && fire_released && !quit && state.tick>=*client_seated+(airburst?750:needler?500:250)) {
                SDL_Event end{};end.type=SDL_QUIT;check(SDL_PushEvent(&end)==1,"Could not end finite source Banshee encounter");quit=true;
            }
            if(!client_seated&&joined&&!quit&&state.tick>=*joined+450) {
                SDL_Event end{};end.type=SDL_QUIT;check(SDL_PushEvent(&end)==1,"Could not end unseated source Banshee diagnostic");quit=true;
            }
            return;
        }
        if(!joined && state.players[1].connected && state.players[1].source_unit!=null_datum)joined=state.tick;
        if(joined&&!queued&&state.tick>=*joined+60) {
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
            if(!detonated)detonated=state.tick;
        }
        if(!quit && ((detonated && state.tick>=*detonated+20) || (joined && state.tick>=*joined+300))) {
            SDL_Event end{};end.type=SDL_QUIT;check(SDL_PushEvent(&end)==1,"Cannot end finite joined grenade encounter");quit=true;
        }
    };
    if(spark)options.view_state_observer=[&](unsigned view,const MultiplayerState &state,const Camera &camera) {
        if(view!=0||!joined)return;
        for(const auto &root:state.source_scene_roots)if(root.definition==spark_definition) {
            auto target=root.transform.position;
            for(const auto &effect:root.attached_effects)for(const auto &location:effect.locations)
                if(spark_authored->string_id(location.name)=="marker") {
                    if(location.node==-1)target=location.local.position;
                    else for(const auto &pose:state.source_body_poses)if(pose.object==root.object) {
                        check(location.node>=0&&unsigned(location.node)<pose.nodes.size(),"Spark view lost its actual marker pose");
                        target=compose_transform(pose.nodes[unsigned(location.node)],location.local).position;
                    }
                }
            const auto delta=target-camera.position;
            const auto yaw=std::atan2(delta.y,delta.x),pitch=std::atan2(delta.z,std::hypot(delta.x,delta.y));
            const auto &settings=options.desktop->keyboard_settings;
            const float gain=.0025f*float(settings.sensitivity)/3.f;
            check(gain>0,"Actual KBM sensitivity has no positive mouse gain");
            const auto x=int(std::clamp(std::lround(-std::remainder(yaw-camera.yaw,6.283185307f)/gain),-120l,120l));
            const auto y=int(std::clamp(std::lround(-(pitch-camera.pitch)/(gain*(settings.invert?-1.f:1.f))),-120l,120l));
            if(x||y) {
                SDL_RaiseWindow(window);check(SDL_SetWindowInputFocus(window)==0,"Could not focus scenery KBM view");
                check(SDL_SetRelativeMouseMode(SDL_TRUE)==0,"Scenery relative mouse unavailable");
                SDL_Event event{};event.type=SDL_MOUSEMOTION;event.motion.windowID=SDL_GetWindowID(window);
                event.motion.xrel=x;event.motion.yrel=y;check(SDL_PushEvent(&event)==1,"Could not deliver actual scenery mouse aim");++spark_mouse_events;
            }
            break;
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
    const bool actual=result==0&&(spark?(spark_snapshots>0&&spark_mouse_events>0&&particles>0):banshee?(fire_steps>0&&!received_projectiles.empty()&&glows>0):projectile_weapon?
        (fire_steps>0&&!received.empty()&&particles>0):
        (pulses>0&&!received.empty()&&particles>0));
    Json report={{"runtime_status",result},{"local_players",2},{"top_input","keyboard/mouse"},
        {"actual_SDL_grenade_pulses",pulses},{"submitted_frames",frames},
        {"received_detonations",received},{"top_explosion_particle_draws",particles},
        {"authority",authority},{"actual_encounter_completed",actual},
        {"banshee",banshee},{"rocket",rocket},{"needler",needler},{"actual_SDL_fire_steps",fire_steps},{"received_projectiles",received_projectiles},
        {"spark",spark},{"actual_spark_snapshots",spark_snapshots},{"actual_SDL_spark_mouse_events",spark_mouse_events},
        {"top_actual_projectile_glow_draws",glows},
        {"declared_entry_setup",banshee?(declared_seat?"Original B95D0 approach placement and declared typed live-vehicle/seat request; original entry admission and joined top-view SDL/UDP firing remain actual":"Original B95D0 placement and normal source look/pickup controls on authority; subsequent firing comes from joined top-view SDL/UDP input"):"none"},
        {"declared_typed_seat_request",declared_seat},
        {"supplied_projectile_hit_lifetime_visibility_or_GPU_result",false},
        {"original_Xbox_pixels_compared",false}};
    std::ofstream(argv[2])<<report.dump(2)<<'\n';
    check(actual,"Joined source effect encounter did not complete its actual presentation");return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
