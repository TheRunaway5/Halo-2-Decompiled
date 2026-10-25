#include "halo2/multiplayer_world.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_objective_native_tu15.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/native_vista_flow.hpp"
#include "halo2/native_vista_announcer.hpp"
#include "halo2/objective_waypoints.hpp"
#include "halo2/source_frustum_camera.hpp"
#include "helpers/multiplayer_snapshot_checks.hpp"
#include <nlohmann/json.hpp>
#include <openssl/sha.h>
#include <algorithm>
#include <bit>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string_view>
#include <vector>
#include <thread>
#include <chrono>
#include <cmath>

using namespace halo2;
using Json=nlohmann::json;
namespace {
void need(bool value,const char* why){if(!value)throw FormatError(why);}
std::uint32_t word(const SourceRuntimeMemory& m,std::uint32_t p){return Bytes(m.view(p,4)).u32(0);}
template<class Address>std::int16_t half(const SourceRuntimeMemory& m,Address p){return std::bit_cast<std::int16_t>(Bytes(m.view(p,2)).u16(0));}
void choose(MenuState& state,unsigned row,unsigned screen) {
    state.select(row);state.accept();need(state.frame().screen==screen,"Objective lifecycle missed an authored menu screen");
}
unsigned entry(const MenuState& state,int value) {
    const auto& entries=state.frame().entries;const auto at=std::ranges::find(entries,value,&MenuEntry::value);
    need(at!=entries.end(),"Objective lifecycle lost an authored menu choice");return unsigned(at-entries.begin());
}
struct Files {
    std::filesystem::path path;
    explicit Files(unsigned n) {
        const auto* scratch=std::getenv("HALO2_TEST_SCRATCH");need(scratch,"Complete objective matches require owned scratch");
        path=std::filesystem::path(scratch)/("objective-match-"+std::to_string(n));std::filesystem::create_directories(path);
    }
    ~Files(){std::error_code error;std::filesystem::remove_all(path,error);}
};
}
int main(int argc,char** argv)try {
    std::cout<<std::unitbuf;
    need(argc==4,"Usage: ctf_assault_match_tu15_tests MCC_MAP ORIGINAL_PRESETS REPORT");
    if(!std::filesystem::is_regular_file(argv[1]))return 77;
    Json fixture;std::ifstream(argv[2])>>fixture;
    Json report={{"passed",false},{"cases",Json::array()},{"scope",
        "All nine unchanged authored CTF/Assault presets, actual pickup/capture/arming/plant/fuse, authored score limits, continuing rounds, accumulated wins and natural match completion. Initial/approach/goal player positions are declared B95D0 inputs. No objective, timer, score, phase, winner or finalization result is supplied. Played routes, presentation, frontend admission and network objective actions remain separate."}};
    const auto save=[&]{std::ofstream out(argv[3]);out<<report.dump(2)<<'\n';need(bool(out),"Cannot retain complete objective match report");};
    unsigned index{},failures{};
    const bool network=std::getenv("HALO2_OBJECTIVE_NETWORK");
    const bool lobby=std::getenv("HALO2_OBJECTIVE_LOBBY");
    need(!lobby || !network,"Offline lifecycle and remote transport cohorts require separate genuine owners");
    report["network_objective_actions"]=network;
    const bool edited=std::getenv("HALO2_OBJECTIVE_EDITED_RULES");
    report["edited_rules"]=edited;
    if(network)report["scope"]="All nine unchanged authored CTF/Assault presets, complete original matches with a genuine remote player. Pickup controls arrive through authenticated loopback UDP; complete authority/client snapshots agree on every observed objective and postgame frame. Player approach/goal positions remain declared B95D0 inputs. Physical played routes, cross-machine delivery and presentation remain separate.";
    if(lobby)report["scope"]="All nine original presets: genuine local menu/profile/team setup and countdown; complete source-owned objective matches with original local views bound; native announcer selection/PCM, waypoint CPU consumers and goal geometry; actual two-team PCR, world destruction, Done to a fresh lobby, next Start and cancellation. Approach/goal positions are declared source position inputs. GPU drawing and physical Xbox capture comparison are separate.";
    Json cohort=Json::array();
    for(const auto& preset:fixture.at("variants")) {
        const auto type=preset.at("source_type").get<unsigned>();if(type!=1 && type!=9)continue;
        if(!edited){cohort.push_back(preset);continue;}
        for(const auto label:{"immediate","long-arm","carrier-traits","kind-0","kind-1","kind-2"}) {
            auto item=preset;item["rule_case"]=label;
            item["runtime_edits"]=Json::array({Json::array({2,1})});
            const auto set=[&](unsigned parameter,int value){item["runtime_edits"].push_back({parameter,value});};
            if(std::string_view(label)=="immediate") {
                set(type==1?54:61,0);set(type==1?52:92,1);set(type==1?53:62,0);
                if(type==9){set(63,0);set(95,0);}
            } else if(std::string_view(label)=="long-arm") {
                set(type==1?54:61,3);set(type==1?53:62,1);
                if(type==9){set(63,15);set(95,15);}
            } else if(std::string_view(label)=="carrier-traits") {
                set(type==1?55:64,1);set(type==1?57:66,1);set(type==1?58:67,3);set(type==1?56:65,1);
            } else {
                set(type==1?50:59,std::string_view(label).back()-'0');
            }
            cohort.push_back(std::move(item));
        }
    }
    if(edited)report["scope"]="54 explicit valid original-rule edits across all nine CTF/Assault presets: all three authored flag/bomb kinds including Neutral Flag CTF, immediate and maximum arming/fuse intervals, return/touch/home options, carrier traits, hit-damage mode and speed, score limit one, actual pickup and natural match completion. Native/literal full-frame comparison retains every gameplay field. B95D0 player positions remain declared inputs; this is a runtime option cohort, not every possible rule combination or a played route.";
    const auto* selected=std::getenv("HALO2_OBJECTIVE_VARIANT");
    if(selected)report["selected_variant"]=selected;
    for(const auto& preset:cohort) {
        const auto type=preset.at("source_type").get<unsigned>();if(type!=1 && type!=9)continue;
        const auto name=preset.at("name").get<std::string>();
        if(selected && name!=selected)continue;
        Json row={{"name",name},{"passed",false},{"goals",Json::array()},{"stage","construct"}};
        if(edited) {row["name"]=name+" / "+preset.at("rule_case").get<std::string>();row["option_edits"]=preset.at("runtime_edits");}
        const auto native_before=source_objective_native_tu15_stats;
        try {
            Files files(index++);MultiplayerOptions options;options.map=argv[1];options.local_players=2;
            options.selected_teams={0,1};
            options.source_object_world=true;options.source_object_savegame_directory=files.path;
            options.variant={name,preset.at("frontend_engine").get<unsigned>(),{}};
            for(const auto& write:preset.at("writes"))options.variant.values.append(write[0].get<unsigned>(),write[1].get<int>());
            const auto original_variant=source_game_variant(options.variant);
            const auto authored=preset.at("raw").get<std::string>();
            need(authored.size()==original_variant.bytes.size()*2,"Original objective preset has an incomplete option buffer");
            for(unsigned n=0;n<original_variant.bytes.size();++n)
                need(original_variant.bytes[n]==std::stoul(authored.substr(n*2,2),nullptr,16),"Complete objective match edited an authored option byte");
            if(edited)for(const auto& edit:preset.at("runtime_edits"))options.variant.values.append(edit[0].get<unsigned>(),edit[1].get<int>());
            const auto variant=source_game_variant(options.variant);
            need(source_game_variant_valid(variant),"Runtime objective edit fails the original checker");
            std::unique_ptr<FrontendSession> frontend;
            std::unique_ptr<NativeVistaOfflineFlow> flow;
            std::shared_ptr<MultiplayerPreparedWorld> prepared_owner;
            std::optional<FrontendLaunch> actual_launch;
            if(lobby) {
                frontend=std::make_unique<FrontendSession>();const auto maps=options.map.parent_path();
                auto content=Cache(maps/"mainmenu.map").load_frontend();content.vista=load_vista_settings(maps/"mainmenu.map");
                const auto selected_map=std::ranges::find_if(content.multiplayer_maps,[&](const auto& m){return m.scenario_path.ends_with(options.map.stem().string());});
                const auto selected_variant=std::ranges::find_if(content.variants,[&](const auto& v) {
                    std::string_view title=v.name;if(title.ends_with(" \xee\x80\x88"))title.remove_suffix(4);return title==name;
                });
                need(selected_map!=content.multiplayer_maps.end() && selected_variant!=content.variants.end(),"Objective lifecycle lost its real map/preset");
                const auto map_index=int(selected_map-content.multiplayer_maps.begin()),variant_index=int(selected_variant-content.variants.begin());
                const auto engine=selected_variant->engine;
                frontend->retain_content(std::move(content));auto& menu=frontend->create_state();
                need(menu.controller_slot(100,true)==0,"Objective lifecycle host controller missing");
                menu.accept();choose(menu,0,167);menu.text("Objective Host");menu.commit_text();menu.back();
                need(menu.frame().screen==6,"Objective lifecycle host profile failed");choose(menu,2,30);
                need(menu.controller_slot(101,true)==1 && menu.frame().screen==10,"Objective lifecycle guest picker missing");
                menu.accept();menu.text("Objective Guest");menu.commit_text();menu.back();need(menu.frame().screen==30,"Objective guest profile failed");
                choose(menu,0,14);choose(menu,1,23);choose(menu,0,206);choose(menu,entry(menu,map_index),14);
                choose(menu,1,23);choose(menu,1,207);choose(menu,entry(menu,int(engine)),208);choose(menu,entry(menu,variant_index),14);
                menu.auxiliary();menu.navigate(1,0);menu.accept();
                need(menu.local_players().size()==2 && menu.lobby_view()->player_team==1,"Objective lobby lost its selected two-team roster");
                options.mute=true;flow=std::make_unique<NativeVistaOfflineFlow>(*frontend,maps,options);
                menu.select(0);menu.accept();need(menu.launch && NativeVistaOfflineFlow::supports(*menu.launch),"Objective Start missed its real lifecycle owner");
                actual_launch=*menu.launch;unsigned beeps{};const auto start=std::chrono::steady_clock::now();
                while(!flow->launch_ready()) {
                    const auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count();
                    need(elapsed<20000,"Objective real-clock countdown did not complete");
                    flow->update_frontend(menu,std::uint32_t(elapsed),[&](auto sound) {
                        need(sound==FrontendUiSound::PregameCountdown,"Unexpected objective countdown cue");++beeps;
                    });
                    if(!flow->launch_ready())std::this_thread::sleep_for(std::chrono::milliseconds(5));
                }
                need(beeps==4,"Objective countdown lost an original final-stage beep");
                const auto launched=flow->take_launch();need(source_game_variant(launched.variant).bytes==variant.bytes,"Lifecycle changed an original objective option");
                prepared_owner=flow->prepared_world();
            } else prepared_owner=std::make_shared<MultiplayerPreparedWorld>(options);
            auto& prepared=*prepared_owner;auto& match=prepared.match;auto& objects=*prepared.source_objects;
            std::unique_ptr<NativeVistaAnnouncer> announcer;std::uint32_t audio_serial{};unsigned audio_events{},audio_choices{},audio_end{};double audio_peak{};
            if(flow) {
                // Match the ordinary runtime's genuine local-view admission.
                // Original1A83B0 consults player+28 before consuming its audio
                // RNG/queue; explicit native PCM bindings alone are weaker.
                prepared.bind_source_local_views(prepared.local_slots);
                flow->world_loaded(match,prepared.local_slots);
                NativeVistaAnnouncerConfiguration cfg{prepared.cache->shared_directory(),prepared.cache->scenario(),prepared.cache->original_data(),0,30,{0x27f,0x1f80},false,0x63746661u,
                    {SdlSoundPlatformConfiguration::Delivery::Manual,512,{}}};
                cfg.source_tu15=true;announcer=NativeVistaAnnouncer::open(cfg);
            }
            bool return_requested{};unsigned waypoint_frames{},maximum_waypoints{},goal_frames{},quad_count{},waypoint_vertices{};
            Mesh waypoint_mesh;std::unique_ptr<ObjectiveWaypoints> waypoint_consumer;
            if(lobby)waypoint_consumer=std::make_unique<ObjectiveWaypoints>(*prepared.cache,waypoint_mesh,unsigned(prepared.local_slots.size()));
            auto& world=objects.tu15_world();auto& memory=world.memory();
            need(prepared.rules.requires_source_engine && match.source_engine_owned(),"Objective match selected a bounded adapter");
            std::array<unsigned char,SHA256_DIGEST_LENGTH> chain{};unsigned observed_frames{};
            const auto chain_hex=[&] {
                constexpr char digits[]="0123456789abcdef";std::string value;
                for(auto byte:chain) {value+=digits[byte>>4u];value+=digits[byte&15u];}return value;
            };
            std::array<Controls,multiplayer_capacity> controls{};
            auto participants=prepared.local_slots;
            std::unique_ptr<LanHost> host;std::unique_ptr<LanClient> client;
            unsigned remote=multiplayer_capacity,replicated_frames{},remote_goals{},remote_pickup_inputs{},unitless_frames{};
            if(network) {
                host=std::make_unique<LanHost>(match,0,0x6374666173730000ull+index,options.variant);
                const std::array<std::int8_t,1> team{0};
                client=std::make_unique<LanClient>("127.0.0.1:"+std::to_string(host->port()),0x6374666173730000ull+index,1,team);
                const std::array<Controls,1> input{};
                for(unsigned n=0;n<240 && (!client->ready() || client->slots().empty() || match.player(client->slots()[0]).source_current_object==null_datum);++n) {
                    client->send(input);host->receive(controls);match.step(controls);host->broadcast();client->receive();
                    if(!client->error().empty())throw FormatError("Objective network join: "+client->error());
                    if(!client->ready())std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
                need(client->ready() && client->slots().size()==1,"Objective network join did not admit one remote player");
                remote=client->slots()[0];need(match.player(remote).source_current_object!=null_datum,"Objective remote player has no genuine current unit");
                participants.insert(participants.begin(),remote);
                test::verify_multiplayer_snapshot(match.state(),client->state());
                row["remote_slot"]=remote;
            }
            const auto configured_rate=unsigned(Bytes(memory.view(word(memory,0x51df54u)+2u,2)).u16(0));
            need(configured_rate>0 && configured_rate<=120,"Objective match has no original tick rate");
            const auto network_interval=std::chrono::nanoseconds(1000000000ull/configured_rate);
            auto next_network_frame=std::chrono::steady_clock::now();
            const auto step=[&] {
                if(announcer)announcer->pre_world_tick();
                if(client) {
                    // The genuine runtime delivers configured-rate ticks.
                    // Accelerated offline frames would exhaust the client's
                    // 128-new-assemblies/second transport protection. Keep
                    // that owner intact and avoid catch-up packet bursts.
                    std::this_thread::sleep_until(next_network_frame);
                    next_network_frame=std::chrono::steady_clock::now()+network_interval;
                    const std::array<Controls,1> input{controls[remote]};
                    client->send(input);controls[remote]={};host->receive(controls);
                    // The real protocol also delivers completed interaction
                    // pulses on release. Held pickup must arrive unchanged;
                    // a release completion remains transport-owned.
                    need(!input[0].pickup || controls[remote].pickup,"Remote objective pickup control did not arrive through UDP");
                    remote_pickup_inputs+=input[0].pickup;
                }
                match.step(controls);const auto original=world.multiplayer_engine_state();
                if(announcer) {
                    const auto state=match.state();
                    for(const auto slot:prepared.local_slots) {
                        const auto& presentation=state.players[slot].source_objective_presentation;
                        waypoint_frames+=!presentation.waypoints.empty();goal_frames+=!presentation.goals.empty();
                        maximum_waypoints=std::max(maximum_waypoints,unsigned(presentation.waypoints.size()));
                        if(waypoint_consumer && observed_frames%30==0) {
                            // Declared view input exercises complete original CPU consumers;
                            // it is not substituted for an ordinary gameplay camera receipt.
                            const auto& player=state.players[slot];
                            SourceFrustumProjectionInput projection_input;
                            projection_input.projection_angle=1.2f;
                            projection_input.outer={0,0,480,640};projection_input.safe={24,32,456,608};
                            projection_input.scenario_present=true;projection_input.scenario_type=1;
                            const auto projection=source_frustum_projection(projection_input);
                            const SourceObjectiveViewTu15 view{player.position,source_view_projection({player.position,
                                {1,0,0},{0,0,1},.015625f,1024.f,{},projection.projection_program}),
                                projection_input.outer,projection_input.safe,1.f,waypoint_consumer->bitmap_views(),unsigned(prepared.local_slots.size())};
                            const auto quads=source_objective_waypoint_quads_tu15(memory,player.source_player,presentation.waypoints,view,
                                [&](Vec3 from,Vec3 to,std::uint32_t flags,std::uint32_t viewer,std::uint32_t target) {
                                    return world.objective_waypoint_occluded(from,to,flags,viewer,target);
                                },{0x27f,0x1f80});
                            const auto vertices=waypoint_consumer->draw(quads,state,slot,
                                unsigned(std::find(prepared.local_slots.begin(),prepared.local_slots.end(),slot)-prepared.local_slots.begin()),640,480);
                            quad_count+=quads.size();waypoint_vertices+=vertices.size();
                            for(const auto& vertex:vertices)need(std::isfinite(vertex.position.x) && std::isfinite(vertex.position.y),"Objective UI produced nonfinite vertices");
                        }
                    }
                    for(const auto& event:state.source_engine_events)if(event.serial>audio_serial) {
                        audio_serial=event.serial;const auto choices=announcer->publish_source_event(event,prepared.local_slots);
                        if(event.words[0]==(type==1?3u:10u))++audio_events;
                        for(const auto& choice:choices)if(choice.sound!=null_datum){++audio_choices;if(event.words[0]==0 && event.words[1]==19)++audio_end;}
                    }
                    for(float sample:announcer->render_manual(1600)){need(std::isfinite(sample),"Objective PCM is not finite");audio_peak=std::max(audio_peak,std::abs(double(sample)));}
                    announcer->audio_update(1.f,match.simulation().sound_classes().gains());
                }
                if(flow && !return_requested)return_requested=flow->world_update(match);
                if(client) {
                    try {host->broadcast();}
                    catch(...) {
                        Json players=Json::array();const auto state=match.state();
                        for(unsigned slot=0;slot<state.players.size();++slot) {
                            const auto& player=state.players[slot];if(!player.connected)continue;
                            players.push_back({{"slot",slot},{"dead",player.dead},{"source_unit",player.source_unit},
                                {"source_previous_unit",player.source_previous_unit},{"source_flags",player.source_flags},
                                {"health",player.health},{"shield",player.shield}});
                        }
                        row["network_failure"]={{"stage","authority-encode"},{"tick",state.tick},
                            {"round_phase",original.round.phase},{"round_index",original.round.index},{"players",players}};
                        throw;
                    }
                    client->receive();
                    const auto expected=match.state();
                    for(const auto slot:participants)if(expected.players[slot].source_unit==null_datum) {
                        need(expected.players[slot].dead && expected.players[slot].source_weapon==null_datum,
                            "Round teardown published an active native player without its actual source unit");
                        ++unitless_frames;
                    }
                    try {test::verify_multiplayer_snapshot(expected,client->state());}
                    catch(...) {
                        const auto wire=host->snapshot_diagnostics();const auto& received=client->state();
                        row["network_failure"]={{"expected_tick",expected.tick},{"received_tick",received.tick},
                            {"finished",expected.finished},{"team_mode",expected.team_mode},{"winner",expected.winner},
                            {"winner_team",expected.winner_team},{"retained_team_mask",expected.retained_team_mask},
                            {"round_mode",expected.round_mode},{"round_phase",original.round.phase},{"round_index",original.round.index},
                            {"expected_device_tick",expected.devices.tick},{"received_device_tick",received.devices.tick},
                            {"body_bytes",wire.body_bytes},{"fragments",wire.fragments},
                            {"buffered_bytes",client->snapshot_buffered_bytes()},
                            {"rejected_packets",client->rejected_packet_count()},
                            {"last_rejection",client->last_packet_rejection()},{"client_error",client->error()}};
                        throw;
                    }
                    ++replicated_frames;
                }
                need(std::memcmp(&original.statistics,&match.source_match_statistics(),sizeof(original.statistics))==0,
                    "Complete objective match advanced a second score/win owner");
                const auto* retained=match.recorded_statistics();
                need(retained && std::memcmp(retained,memory.view(0x51eef4u,sizeof(*retained)).data(),sizeof(*retained))==0,
                    "Complete objective match lost original retained statistics");
                // A bounded digest of every observed gameplay frame. Retain
                // no raw banks: chain objective/registry state, statistics,
                // each local player row and its actual current unit body.
                std::vector<unsigned char> frame(chain.begin(),chain.end());
                const auto add=[&](std::uint32_t address,std::size_t size) {
                    for(const auto value:{address,std::uint32_t(size)})for(unsigned n=0;n<4;++n)frame.push_back(value>>(n*8u));
                    for(auto byte:memory.view(address,size))frame.push_back(std::to_integer<unsigned char>(byte));
                };
                add(word(memory,0x52d710u),0x208u);add(word(memory,0x4f63b8u)+0xc14u,0xa0u);add(0x51eef4u,sizeof(*retained));
                const auto players=word(memory,word(memory,0x4f54f4u)+0x44u);
                for(const auto slot:participants) {
                    add(players+(match.player(slot).source_datum&65535u)*0x21cu,0x21cu);
                    const auto unit=match.player(slot).source_current_object;
                    if(unit!=null_datum)add(memory.source_address(objects.object_body(unit),0x200u),0x200u);
                }
                if(std::getenv("HALO2_OBJECTIVE_FRAME_CAPTURE") && observed_frames<64) {
                    const auto directory=std::filesystem::path(std::getenv("HALO2_TEST_SCRATCH"))/"objective-frame-capture";
                    std::filesystem::create_directories(directory);
                    std::ofstream output(directory/(std::to_string(observed_frames)+".bin"),std::ios::binary);
                    output.write(reinterpret_cast<const char*>(frame.data()),std::streamsize(frame.size()));
                    need(bool(output),"Cannot write disposable objective frame diagnostic");
                }
                SHA256(frame.data(),frame.size(),chain.data());++observed_frames;
            };
            const auto carrier_item=[&](unsigned slot) {
                auto abi=world.prepare_source_call(std::array{1u});abi.gp[0]=match.player(slot).source_datum;
                world.run_actual(0x1a8fd0u,abi);return abi.gp[0];
            };
            bool objective_late_join{};
            const auto late_join=[&](std::string_view phase) {
                need(client && !objective_late_join,"Objective late join has no genuine transport owner");
                const std::array<std::int8_t,1> team{1};unsigned joined=multiplayer_capacity;
                {
                    LanClient observer("127.0.0.1:"+std::to_string(host->port()),0x6374666173730000ull+index,1,team);
                    const std::array<Controls,1> input{};
                    for(unsigned n=0;n<80 && !observer.ready() && observer.error().empty();++n) {
                        observer.send(input);step();observer.receive();
                        if(!observer.ready())std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    }
                    need(observer.ready() && observer.error().empty() && observer.slots().size()==1,
                        "Late objective observer failed genuine UDP admission");
                    joined=observer.slots()[0];test::verify_multiplayer_snapshot(match.state(),observer.state());
                    row["objective_late_join"]={{"phase",phase},{"tick",observer.state().tick},{"slot",joined},
                        {"source_player",observer.state().players[joined].source_player}};
                }
                for(unsigned n=0;n<80 && !(match.player(joined).source_flags&2u);++n) {
                    step();if(!(match.player(joined).source_flags&2u))std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
                need((match.player(joined).source_flags&2u) && match.player(joined).source_current_object==null_datum && host->peers()==1,
                    "Late objective observer did not complete actual departure/current-unit release");
                objective_late_join=true;
            };
            for(unsigned ordinal=0;ordinal<12 && !match.state().finished;++ordinal) {
                row["stage"]="ordinary-round-or-respawn-admission";
                const auto rate=unsigned(Bytes(memory.view(word(memory,0x51df54u)+2u,2)).u16(0));
                need(rate>0 && rate<=120,"Complete match has no original tick rate");
                const auto sample=[&] {
                    Json players=Json::array();const auto state=match.state();
                    const auto pool=word(memory,0x4f54f4u),rows=word(memory,pool+0x44u),stride=word(memory,pool+0x24u);
                    need(stride==0x21c,"Complete match has no original player rows");
                    for(const auto slot:participants) {
                        const auto at=rows+(match.player(slot).source_datum&65535u)*stride;
                        players.push_back({{"slot",slot},{"source_unit",word(memory,at+0x2cu)},
                            {"native_unit",match.player(slot).source_current_object},
                            {"source_countdown",std::bit_cast<std::int32_t>(word(memory,at+0x164u))},
                            {"native_countdown",state.players[slot].source_respawn_countdown},
                            {"lives",half(memory,at+0x1acu)},{"dead",state.players[slot].dead}});
                    }
                    return Json{{"tick",state.tick},{"phase",world.multiplayer_engine_state().round.phase},{"players",players}};
                };
                unsigned waiting{},countdown{};Json admission={{"samples",Json::array({sample()})}};
                for(const auto slot:participants)
                    countdown=std::max(countdown,unsigned(std::max(0,match.state().players[slot].source_respawn_countdown)));
                need(countdown<=rate*600u,"Source respawn countdown exceeds the finite diagnostic budget");
                // A detonation can genuinely kill the carrier before a later
                // capture in the same round. Honor its actual remaining
                // source timer, then the existing round/factory margin.
                const auto wait_budget=countdown+256u;admission["wait_budget"]=wait_budget;
                while(waiting<wait_budget && (world.multiplayer_engine_state().round.phase!=1 ||
                    std::ranges::any_of(participants,[&](auto slot){return match.player(slot).source_current_object==null_datum;}))) {
                    step();++waiting;
                    if(waiting%rate==0)admission["samples"].push_back(sample());
                }
                admission["frames"]=waiting;admission["samples"].push_back(sample());
                if(!row.contains("admissions"))row["admissions"]=Json::array();
                row["admissions"].push_back(std::move(admission));
                need(waiting<wait_budget,"Original round/respawn did not admit its replacement players");
                const auto round=world.multiplayer_engine_state().round.index;
                const auto before_tick=match.state().tick;
                const auto engine=word(memory,0x4f63b8u);
                std::array<Vec3,multiplayer_capacity> starts{};
                for(const auto slot:participants) {
                    starts[slot]=objects.biped_motion(match.player(slot).source_current_object).position;
                    controls[slot].yaw=match.unit(slot).yaw;controls[slot].pitch=match.unit(slot).pitch;
                }
                row["stage"]="ordinary-pickup";std::uint32_t carried=null_datum;unsigned carrier{};
                for(unsigned scan=0;scan<4 && carried==null_datum;++scan) {
                    const auto count=word(memory,engine+0xc18u);need(count<=8,"Original objective registry exceeds capacity");
                    for(unsigned priority=0;priority<(network?2u:1u) && carried==null_datum;++priority)
                    for(unsigned n=0;n<count && carried==null_datum;++n) {
                        const auto item=word(memory,engine+0xc1cu+n*16u);if(item==null_datum)continue;
                        const auto owner=half(memory,objects.object_body(item)+0x17eu);
                        if(network) {
                            const auto team=match.player(remote).source_membership;
                            const bool remote_eligible=owner==8 || (type==1?owner!=team:owner==team);
                            if((priority==0)!=remote_eligible)continue;
                        }
                        for(const auto slot:participants) {
                            if(carried!=null_datum)break;
                            const auto team=match.player(slot).source_membership;
                            if(owner!=8 && (type==1?owner==team:owner!=team))continue;
                            for(const auto other:participants)if(other!=slot)
                                world.set_object_position(match.player(other).source_current_object,starts[other]);
                            auto position=objects.object_transform(item).position;position.x-=.2f;
                            const auto unit=match.player(slot).source_current_object;
                            world.set_object_position(unit,position);controls[slot].yaw=0;controls[slot].pitch=0;
                            bool admitted{};
                            for(unsigned frame=0;frame<64 && carried==null_datum;++frame) {
                                controls[slot].pickup=frame>=2 && !admitted;
                                controls[slot].pickup_milliseconds=controls[slot].pickup?std::uint16_t((frame-2)*1000/30):0;
                                step();const auto held=carrier_item(slot);admitted|=held!=null_datum;
                                const auto equipment=world.player_equipment(unit);
                                if(held==item && objects.object_parent(item)==unit && match.state().players[slot].source_weapon==item &&
                                    std::ranges::find(equipment.weapons,item)!=equipment.weapons.end()){carried=item;carrier=slot;}
                            }
                            controls[slot].pickup=false;controls[slot].pickup_milliseconds=0;
                        }
                    }
                    if(carried==null_datum)step();
                }
                need(carried!=null_datum,"Complete match failed genuine objective pickup");
                if(network && type==1 && !objective_late_join)late_join("carried-flag");
                row["stage"]="ordinary-authored-goal";
                const auto member=match.player(carrier).source_membership;
                need(member>=0 && member<8,"Complete match carrier has no original playing team");
                const auto bank=word(memory,0x52d710u),scenario=word(memory,0x4ecc20u);
                int goal=-1;
                for(const auto slot:participants) {
                    const auto team=match.player(slot).source_membership;
                    if(type==1?team!=member:team==member)continue;
                    auto mapping=world.prepare_source_call(std::array{std::uint32_t(team)});world.run_actual(0x15b3b0u,mapping);
                    const auto mapped=std::bit_cast<std::int32_t>(mapping.gp[0]);if(mapped<0 || mapped>=9)continue;
                    goal=half(memory,bank+0x14u+unsigned(mapped)*2u);if(goal>=0)break;
                }
                need(goal>=0 && unsigned(goal)<word(memory,scenario+0x118u),"Complete match has no original selected goal");
                const auto marker=Bytes(memory.view(word(memory,scenario+0x11cu)+unsigned(goal)*32u,12));
                const auto unit=match.player(carrier).source_current_object;
                const auto before=match.state().team_scores[unsigned(member)];
                world.set_object_position(unit,{marker.f32(0),marker.f32(4),marker.f32(8)});
                const auto arm=source_game_variant_get(variant,63),fuse=source_game_variant_get(variant,95);
                const auto budget=type==1?64u:rate*unsigned(std::max(0,arm)+std::max(10,fuse)+2)+64u;
                unsigned frames{};
                while(frames<budget && match.state().team_scores[unsigned(member)]==before && !match.state().finished) {
                    step();++frames;
                    if(network && type==9 && !objective_late_join &&
                        (word(memory,bank+0x170u+unsigned(half(memory,objects.object_body(carried)+0x17eu)))&2u))
                        late_join("planted-bomb");
                }
                need(match.state().team_scores[unsigned(member)]==before+1,"Complete match goal did not award exactly one original point");
                if(carrier==remote)++remote_goals;
                row["goals"].push_back({{"round",round},{"carrier",carrier},{"team",member},{"marker",goal},
                    {"before",before},{"after",match.state().team_scores[unsigned(member)]},
                    {"pickup_and_goal_frames",match.state().tick-before_tick},{"goal_frames",frames},
                    {"gameplay_frame_chain",chain_hex()}});
                // Deliver pending original phase requests before admitting the
                // next pickup; the real owner chooses continuation/completion.
                for(unsigned n=0;n<8 && !match.state().finished;++n)step();
            }
            need(match.state().finished,"Authored objective rules did not complete a match within twelve actual goals");
            auto original=world.multiplayer_engine_state();const auto completed=original.statistics;
            const auto winner=match.state().winner_team;need(winner>=0 && winner<8,"Completed team objective has no original winner");
            const auto wins=original.statistics.teams[unsigned(winner)].fields[7];
            need(wins==(source_game_variant_get(variant,0)==5?3:1),"Original objective completion ignored authored first-to-three/single-round rules");
            for(unsigned n=0;n<211;++n) {
                step();original=world.multiplayer_engine_state();
                need(std::memcmp(&original.statistics,&completed,sizeof(completed))==0,"Objective postgame changed completed scores/wins");
            }
            need(original.finalized && original.recording.frozen && original.game_over.complete() && original.game_over.fade_started,
                "Authored objective match did not finish retained results/countdown/fade");
            row["winner_team"]=winner;row["winner_rounds"]=wins;row["tick"]=match.state().tick;
            row["observed_frames"]=observed_frames;row["gameplay_frame_chain"]=chain_hex();
            if(network) {
                need(remote_goals>0 && remote_pickup_inputs>0 && objective_late_join && replicated_frames==observed_frames,"Network match did not complete a remotely controlled objective goal, late join and every replicated frame");
                if(source_game_variant_get(variant,0)==5)need(unitless_frames>0,"Continuing rounds lost actual source-unit teardown frames");
                row["unitless_player_frames"]=unitless_frames;
                row["remote_goals"]=remote_goals;row["replicated_frames"]=replicated_frames;
                row["remote_pickup_inputs"]=remote_pickup_inputs;
            }
            if(flow) {
                need(return_requested && flow->has_report() && flow->lifecycle_kind()==4,
                    "Completed objective match did not request its authentic postgame/lobby handoff");
                need(audio_events && audio_choices && audio_end && audio_peak>0,
                    "Objective events/start/end did not reach actual native PCM playback");
                need(waypoint_frames && maximum_waypoints && (type!=9 || goal_frames),
                    "Original objective view callbacks did not deliver their native waypoint/goal packets");
                need(quad_count && waypoint_vertices==quad_count*6,"Original objective UI consumers did not emit their native quads");
                row["waypoints"]={{"recipient_frames",waypoint_frames},{"maximum_count",maximum_waypoints},{"goal_frames",goal_frames},{"cpu_quads",quad_count},{"cpu_vertices",waypoint_vertices}};
                row["audio"]={{"mode_events",audio_events},{"selected_cues",audio_choices},{"end_cues",audio_end},{"pcm_peak",audio_peak}};
                auto& menu=frontend->state();const auto old_lobby=menu.frame().instance;
                std::weak_ptr<MultiplayerPreparedWorld> retired=prepared_owner;
                announcer.reset();prepared_owner.reset();flow->world_unloaded();
                need(retired.expired(),"Objective return retained its departed source world");
                {Cache menu_cache(options.map.parent_path()/"mainmenu.map");(void)menu_cache.load_bsp(0);}
                flow->mainmenu_loaded();
                need(menu.frame().screen==vista_menu(16) && menu.frame().vista_pcr &&
                    menu.frame().vista_pcr->results->variant==type && menu.frame().vista_pcr->results->player_count==2 &&
                    menu.frame().vista_pcr->results->team_count==2,"Objective PCR lost its actual selected family, players or teams");
                const auto report_instance=menu.frame().instance;
                menu.back();flow->update_frontend(menu,20000,[](auto){throw FormatError("Unexpected returned-lobby countdown cue");});
                need(menu.frame().screen==14 && menu.stack().size()==1 && menu.frame().instance!=old_lobby &&
                    menu.frame().instance!=report_instance && flow->lifecycle_kind()==1 && !menu.launch,
                    "Objective Done did not allocate its genuine ready lobby");
                const auto ready_instance=menu.frame().instance;
                for(unsigned frame=0;frame<120;++frame) {
                    flow->update_frontend(menu,20000+frame*16,[](auto){throw FormatError("Unexpected idle lobby cue");});
                    need(menu.frame().instance==ready_instance && menu.frame().screen==14 &&
                        flow->countdown()==-1 && !menu.launch,"Objective lobby return reopened a retired screen");
                }
                menu.select(0);menu.accept();need(menu.launch && actual_launch &&
                    source_game_variant(menu.launch->variant).bytes==variant.bytes &&
                    menu.launch->selected_profiles==actual_launch->selected_profiles &&
                    menu.launch->selected_teams==actual_launch->selected_teams,
                    "Returned objective lobby changed the next match's real selected configuration");
                flow->update_frontend(menu,24000,[](auto){throw FormatError("Unexpected early countdown cue");});
                need(flow->countdown()==10 && !menu.launch,"Returned objective lobby cannot begin the next authentic Start");
                menu.back();need(flow->countdown()==-1 && menu.frame().instance==ready_instance,
                    "Cancelling the next objective Start left its returned lobby");
                row["lobby_return"]={{"real_countdown",true},{"world_released",true},{"pcr_players",2},{"pcr_teams",2},
                    {"idle_frames",120},{"next_start",true},{"next_cancel",true}};
            }
            row["passed"]=true;row["stage"]="complete";
        }catch(const std::exception& error){row["error"]=error.what();++failures;}
        row["native_owner_calls"]=Json::array();
        for(unsigned n=0;n<source_objective_native_owner_entries.size();++n)
            row["native_owner_calls"].push_back({{"entry",source_objective_native_owner_entries[n]},
                {"calls",source_objective_native_tu15_stats.owners[n]-native_before.owners[n]}});
        std::cout<<row.dump()<<'\n';report["cases"].push_back(std::move(row));save();
    }
    need(index==(selected?(edited?6u:1u):(edited?54u:9u)),"Complete objective match cohort lost an authored preset");report["passed"]=failures==0;save();return failures?1:0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
