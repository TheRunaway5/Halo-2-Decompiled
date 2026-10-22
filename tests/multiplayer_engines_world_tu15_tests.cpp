#include "halo2/cache.hpp"
#include "halo2/original_data.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/datum_pool.hpp"
#include "halo2/source_physics_entry_observation.hpp"
#include "halo2/multiplayer_world.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#if defined(__linux__)
#include <ifaddrs.h>
#include <net/if.h>
#include <netpacket/packet.h>
#endif

using namespace halo2;
using Json=nlohmann::json;
namespace {
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t address,unsigned width=4) {
    const auto bytes=memory.view(address,width);std::uint32_t value{};
    for(unsigned i=0;i<width;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(8*i);
    return value;
}
struct Files {
    std::filesystem::path path;
    explicit Files(unsigned index) {
        const auto scratch=std::getenv("HALO2_TEST_SCRATCH");
        if(!scratch)throw std::runtime_error("Native engine tests require tools/run_test.py scratch ownership");
        path=std::filesystem::path(scratch)/("engine-world-"+std::to_string(index));
        std::filesystem::create_directories(path);
    }
    ~Files(){std::error_code ignored;std::filesystem::remove_all(path,ignored);}
};
void check(bool condition,const char* message) {
    if(!condition)throw std::runtime_error(message);
}
SourceMachineIdentity machine_identity() {
#if defined(__linux__)
    ifaddrs* interfaces=nullptr;
    check(::getifaddrs(&interfaces)==0,"Native machine interface enumeration failed");
    struct Owner {ifaddrs* value;~Owner(){::freeifaddrs(value);}} owner{interfaces};
    for(auto* at=interfaces;at;at=at->ifa_next) {
        if(!at->ifa_addr || at->ifa_addr->sa_family!=AF_PACKET ||
            !(at->ifa_flags&IFF_UP) || (at->ifa_flags&IFF_LOOPBACK))continue;
        const auto* address=reinterpret_cast<const sockaddr_ll*>(at->ifa_addr);
        if(address->sll_halen!=6)continue;
        SourceMachineIdentity result;
        std::copy_n(address->sll_addr,result.size(),result.begin());return result;
    }
#endif
    throw std::runtime_error("Native source player identity requires an up six-byte hardware interface");
}
}
int main(int argc,char** argv)try {
    std::cout<<std::unitbuf;
    if(argc!=4)throw std::runtime_error("Usage: multiplayer_engines_world_tu15_tests MCC_MAP VARIANT_FIXTURE REPORT");
    if(!std::filesystem::is_regular_file(argv[1]))return 77;
    const Cache cache(argv[1],{},OriginalData::native_tu15());
    check(cache.is_mcc(),"Native engine integration requires imported MCC content");
    Json fixture;std::ifstream(argv[2])>>fixture;
    check(fixture.at("variants").size()==39,"Native engine integration requires all 39 authored variants");
    Json report={{"passed",false},{"map",argv[1]},{"original_runtime_image_loaded",false},
        {"supplied_service_results",Json::array()},{"cases",Json::array()},
        {"scope","Native session, activation, one complete shared engine frame and physical home return of registered CTF/Assault items after that frame. No players; complete matches, active-round effects and replication remain separate acceptance."}};
    const bool occupied=std::getenv("HALO2_ENGINE_WORLD_OCCUPIED")!=nullptr;
    const bool rounds=std::getenv("HALO2_ENGINE_WORLD_ROUNDS")!=nullptr;
    const bool spawn=std::getenv("HALO2_ENGINE_WORLD_SPAWN")!=nullptr;
    const bool world_tick=std::getenv("HALO2_ENGINE_WORLD_TICK")!=nullptr;
    check(!rounds || occupied,"Native phase checks require a real joined roster");
    check(!spawn || (occupied && !rounds),"Native player spawning requires its occupied frame sequence");
    check(!world_tick || spawn,"Native world ticking requires the occupied spawning cohort");
    report["occupied"]=occupied;
    report["round_transitions"]=rounds;
    report["finalization"]=rounds;
    report["player_spawning"]=spawn;
    report["source_world_ticks"]=world_tick;
    if(occupied)report["scope"]="Native player allocation, real platform identities, original join/activation/frame paths and physical CTF/Assault home return. Full match, normal scheduler and replication acceptance remain open.";
    if(world_tick)report["scope"]="Actual source-world ticks with engine/player, object/effect/light/first-person phases and their clock, then explicit match-adapter ownership. Ordinary MultiplayerMatch state/consumer adoption, full matches and replication remain open.";
    if(std::getenv("HALO2_ENGINE_PREPARED_MATCH")) {
        report["occupied"]=true;report["player_spawning"]=true;report["source_world_ticks"]=true;
        report["finalization"]=std::getenv("HALO2_ENGINE_PREPARED_FINALIZATION")!=nullptr;
        report["scope"]="Ordinary prepared MCC matches with original variant inputs, source roster/current units, frame ordering, clocks, scores, lives, membership, King state and camouflage. Optional original match-over requests exercise finalization/countdown/fade through ordinary frames. Full objective-driven matches, presentation and replication remain open.";
    }
    const auto save=[&] {std::ofstream output(argv[3]);output<<report.dump(2)<<'\n';
        check(bool(output),"Could not retain native engine integration report");};
    if(std::getenv("HALO2_ENGINE_MATCH_ADAPTER_TICK")) {
        Json result={{"name","ordinary-Slayer-adapter"},{"passed",false},{"stage","construct"}};
        report["occupied"]=true;
        report["scope"]="Actual prepared MCC Slayer match consuming original engine/player phases, lifecycle, lives and score banks. Declared placement retains adapter ownership. Full seven-mode, presentation and transport acceptance remain open.";
        try {
            Files files(0);Json entries=Json::array();
            std::vector<std::uint32_t> expected{0x14fec0u,0x1d0670u,0x15eba0u,0x13b6f0u,0xb8f40u,
                0x1cc8d0u,0xb9090u,0xb9180u,0x179cd0u,0xc3430u,0x15f450u,0x1503d0u,0x13b990u,0x168ca1u};
            MultiplayerOptions options;options.map=argv[1];options.local_players=2;
            options.variant={"Slayer",0,{}};options.source_object_world=true;
            const bool declared=std::getenv("HALO2_ENGINE_MATCH_DECLARED_PLACEMENT")!=nullptr;
            if(declared) {
                for(auto entry:{0x15eba0u,0x15f450u,0x1503d0u,0x13b990u})std::erase(expected,entry);
                check(!cache.spawns().empty(),"Declared-placement regression requires an authored reference start");
                auto start=cache.spawns().front();start.facing+=0.25f;
                options.native_spawn_inputs={start};options.local_players=1;
                result["name"]="declared-placement-adapter";
            }
            const bool finite=std::getenv("HALO2_ENGINE_MATCH_FINITE_LIVES")!=nullptr;
            if(finite)options.variant.values.append(16,2);
            if(std::getenv("HALO2_ENGINE_MATCH_TEAM_PLAYERS")) {
                options.variant.values.append(91,1);options.selected_teams={0,1};
                result["name"]="ordinary-Team-Slayer-adapter";
            }
            options.source_object_savegame_directory=files.path;
            MultiplayerPreparedWorld prepared(options);
            check(prepared.source_objects && prepared.source_objects->owns_physics(),"Prepared match lost its actual source world");
            auto& world=prepared.source_objects->tu15_world();auto& memory=world.memory();
            unsigned completed_births{},life_stores{};
            world.observe_physics_stores([&](auto pc,const auto&) {
                if(pc==0x152181u)++completed_births;
                if(pc==0x15fa83u)++life_stores;
            });
            const auto player_pool=world.source_player_pool();result["adopted_source_births"]=Json::array();
            for(const auto slot:prepared.local_slots) {
                const auto record=word(memory,player_pool+0x44u)+slot*word(memory,player_pool+0x24u);
                const auto unit=word(memory,record+0x2cu),flags=word(memory,record+2u,2);
                check(unit!=DatumPool::none && (declared || (flags&9u)==1u),"Ordinary source player did not complete its factory-owned birth");
                if(declared) {
                    check(!prepared.source_objects->uses_scenario_player_spawns(options.native_spawn_inputs),
                        "Declared placement was mistaken for the source scenario start bank");
                    const auto root=prepared.source_objects->object_transform(unit);
                    const auto p=options.native_spawn_inputs.front().position;
                    check(std::bit_cast<std::uint32_t>(root.position.x)==std::bit_cast<std::uint32_t>(p.x) &&
                        std::bit_cast<std::uint32_t>(root.position.y)==std::bit_cast<std::uint32_t>(p.y) &&
                        std::bit_cast<std::uint32_t>(root.position.z)==std::bit_cast<std::uint32_t>(p.z),
                        "Explicit native placement lost its caller's actual XYZ bits");
                    result["declared_position"]={root.position.x,root.position.y,root.position.z};
                }
                result["adopted_source_births"].push_back({{"slot",slot},{"unit",unit},{"flags",flags}});
            }
            SourcePhysicsEntryObserverBinding observation(memory,[&](bool entering,auto entry,const auto&) {
                if(entering && (std::ranges::find(expected,entry)!=expected.end() ||
                    entry==0x15eba0u || entry==0x15f450u || entry==0x1503d0u || entry==0x151da0u))entries.push_back(entry);
            });
            const auto clock=word(memory,0x51df54u);result["ticks"]=Json::array();
            for(unsigned tick=0;tick<2;++tick) {
                result["stage"]="ordinary-match-tick-"+std::to_string(tick);entries=Json::array();
                const auto source_before=word(memory,clock+8u),match_before=prepared.match.state().tick;
                prepared.match.step({});
                result["ticks"].push_back({{"entries",entries},{"source_clock",word(memory,clock+8u)},
                    {"match_clock",prepared.match.state().tick}});
                check(entries==Json(expected),"Ordinary match duplicated or omitted source world phases");
                check(word(memory,clock+8u)==source_before+1u && prepared.match.state().tick==match_before+1u,
                    "Ordinary match and source world did not each advance once");
                if(!declared) {
                    const auto source=world.multiplayer_engine_state();
                    const auto& round=prepared.match.source_round_lifecycle();
                    check(source.round.phase==round.phase && source.round.status==round.status &&
                        source.round.clock_seconds==round.clock_seconds && source.round.start_tick==round.start_tick,
                        "Ordinary match did not consume the original lifecycle bank");
                    check(std::memcmp(&source.statistics,&prepared.match.source_match_statistics(),sizeof(source.statistics))==0,
                        "Ordinary match advanced a separate score bank");
                }
            }
            const auto pool=world.source_player_pool();result["current_units"]=Json::array();
            for(const auto slot:prepared.local_slots) {
                const auto unit=word(memory,word(memory,pool+0x44u)+slot*word(memory,pool+0x24u)+0x2cu);
                check(unit!=DatumPool::none && world.source_object_body(unit)!=0,"Ordinary match lost a genuine current unit");
                result["current_units"].push_back(unit);
            }
            if(std::getenv("HALO2_ENGINE_MATCH_PLAYER_FACTORY_FLOW")) {
                const auto slot=prepared.local_slots.at(0);
                const auto record=word(memory,pool+0x44u)+slot*word(memory,pool+0x24u);
                const auto before=word(memory,record+0x2cu);
                const auto start=prepared.source_objects->biped_motion(before).position;
                std::array<Controls,multiplayer_capacity> controls{};
                controls[slot].forward=1;controls[slot].yaw=prepared.match.state().players[slot].yaw;
                result["stage"]="source-born-player-movement";
                for(unsigned tick=0;tick<12;++tick)prepared.match.step(controls);
                const auto moved=prepared.source_objects->biped_motion(before).position;
                result["source_movement"]={{"ticks",12},{"before",{start.x,start.y,start.z}},
                    {"after",{moved.x,moved.y,moved.z}}};
                check((moved.x-start.x)*(moved.x-start.x)+(moved.y-start.y)*(moved.y-start.y)>1e-6f,
                    "Genuine source-born player did not respond to delivered movement");
                result["stage"]="source-player-fatal-stimulus";
                result["fatal_inputs"]={{"life_limit",finite?2:0},{"lives",word(memory,record+0x1acu,2)},
                    {"statistics_header",world.multiplayer_engine_state().statistics.header},
                    {"phase",world.multiplayer_engine_state().round.phase},{"status",world.multiplayer_engine_state().round.status}};
                //1F273C..1F2740 supplies these four inputs to the whole
                // engine death owner before physical cleanup. This focused
                // engine-event delivery does not simulate a projectile hit.
                const auto player=prepared.match.player(slot).source_datum;
                const auto fatal=[&](std::uint32_t object) {
                    world.call_actual(0x15f8b0u,std::array{DatumPool::none,player,0u,63u});
                    prepared.source_objects->kill_biped(object);
                };
                result["fatal_boundary"]="Original15F8B0 engine event (credit NONE, victim datum, relationship0, reporting63), then native D8C20/D8D50/CCF40 body cleanup; no hit-finding/projectile proof";
                fatal(before);
                result["fatal_outputs"]={{"life_stores",life_stores},{"lives",word(memory,record+0x1acu,2)},
                    {"respawn_countdown",word(memory,record+0x164u)}};
                result["factory_flow"]={{"previous_unit",before},{"birth_calls",0},{"steps",0}};
                bool respawned=false,saw_dead=false;unsigned births{};
                for(unsigned tick=0;tick<300 && !respawned;++tick) {
                    result["stage"]="source-player-respawn-tick-"+std::to_string(tick);entries=Json::array();
                    prepared.match.step({});
                    births=completed_births;
                    const auto current=word(memory,record+0x2cu);const auto state=prepared.match.state();
                    saw_dead=saw_dead || state.players[slot].dead;
                    respawned=current!=DatumPool::none && current!=before;
                    result["factory_flow"]={{"previous_unit",before},{"current_unit",current},
                        {"birth_calls",births},{"steps",tick+1},{"observed_death",saw_dead},
                        {"raw_lives",word(memory,record+0x1acu,2)},
                        {"adopted_lives",state.players[slot].source_lives_remaining},{"life_stores",life_stores}};
                    if(respawned) {
                        check(state.players[slot].source_unit==current && !state.players[slot].dead,
                            "Ordinary match did not adopt its genuine replacement source unit");
                        check((word(memory,record+2u,2)&9u)==1u,"Source replacement did not complete its birth flags");
                        check(state.players[slot].source_lives_remaining==(finite?1:-1),
                            "Source unit adoption differs from original remaining lives");
                    }
                }
                check(respawned && saw_dead && births>0,"Ordinary source unit did not complete actual death and factory respawn");
                if(finite) {
                    result["stage"]="source-player-last-life";
                    fatal(word(memory,record+0x2cu));
                    for(unsigned tick=0;tick<8;++tick)prepared.match.step({});
                    const auto state=prepared.match.state();
                    result["finite_life_outcome"]={{"remaining",state.players[slot].source_lives_remaining},
                        {"current_unit",word(memory,record+0x2cu)},{"finished",state.finished}};
                    check(state.players[slot].source_lives_remaining==0 &&
                        word(memory,record+0x2cu)==DatumPool::none && state.finished,
                        "Exhausted source-player adapter received another unit or lost its single-round ending");
                }
            }
            if(std::getenv("HALO2_ENGINE_MATCH_GAME_OVER")) {
                result["stage"]="original-score-limit-round-end";
                const bool team=std::getenv("HALO2_ENGINE_MATCH_TEAM_PLAYERS")!=nullptr;
                const bool tie=std::getenv("HALO2_ENGINE_MATCH_TIED_END")!=nullptr;
                const auto winner=tie?DatumPool::none:team?1u:prepared.match.player(prepared.local_slots.back()).source_datum;
                const auto winning_player=prepared.match.player(prepared.local_slots.back()).source_datum;
                const auto limit=source_game_variant_get(source_game_variant(options.variant),2);
                if(tie) {
                    auto end=world.prepare_source_call(std::array{1u});end.gp[3]=winner;
                    world.run_actual(0x15ddc0u,end);
                } else {
                    auto score=world.prepare_source_call(std::array{winning_player});score.gp[0]=std::uint32_t(limit);
                    world.run_actual(0x15e1e0u,score);
                }
                for(unsigned tick=0;tick<211;++tick) {
                    prepared.match.step({});const auto source=world.multiplayer_engine_state();
                    const auto state=prepared.match.state();
                    check(state.finished && state.winner==(team||tie?-1:int(winner&0xffffu)) &&
                        state.winner_team==(!team||tie?-1:int(winner)),"Native result lost the actual original round-end winner");
                    check(state.game_over_countdown==source.game_over.countdown &&
                        state.game_over_fade_started==source.game_over.fade_started,
                        "Native game-over consumer advanced a parallel countdown or fade");
                }
                const auto source=world.multiplayer_engine_state();
                check(source.finalized && source.game_over.complete() && source.game_over.fade_started,
                    "Original match ending did not finalize and complete its countdown/fade");
                if(!tie) {
                    check(source.statistics.players[winning_player&0xffffu].fields[0]==limit &&
                        source.statistics.players[winning_player&0xffffu].fields[7]==1,
                        "Original score-limit ending lost its score or awarded the native winner twice");
                    if(team)check(source.statistics.teams[winner].fields[0]==limit &&
                        source.statistics.teams[winner].fields[7]==1,"Original team score/win publication differs");
                }
                result["completed_game_over"]={{"winner",winner},{"team",team},{"tie",tie},
                    {"countdown",source.game_over.countdown},{"fade",source.game_over.fade_started},{"finalized",source.finalized}};
            }
            result["passed"]=true;result["stage"]="complete";
        }catch(const std::exception& error){result["error"]=error.what();}
        report["cases"].push_back(result);report["passed"]=result["passed"];save();
        return result["passed"].get<bool>()?0:1;
    }
    unsigned index{},failures{};
    for(const auto& row:fixture.at("variants")) {
        const auto name=row.at("name").get<std::string>();
        if(const auto selected=std::getenv("HALO2_ENGINE_WORLD_VARIANT");selected && name!=selected)continue;
        Json result={{"name",name},{"source_type",row.at("source_type")},{"passed",false},{"stage","construct"}};
        if(std::getenv("HALO2_ENGINE_PREPARED_MATCH")) {
            try {
                Files files(index++);MultiplayerOptions options;options.map=argv[1];
                options.source_object_world=true;options.source_object_savegame_directory=files.path;
                options.variant={name,row.at("frontend_engine").get<unsigned>(),{}};
                for(const auto& write:row.at("writes"))options.variant.values.append(write[0].get<unsigned>(),write[1].get<int>());
                auto variant=source_game_variant(options.variant);
                const auto raw=row.at("raw").get<std::string>();
                check(raw.size()==variant.bytes.size()*2,"Authored variant input extent changed");
                for(unsigned n=0;n<variant.bytes.size();++n)
                    check(variant.bytes[n]==std::stoul(raw.substr(n*2,2),nullptr,16),"Prepared match did not receive its authored original variant");
                const auto edited_limit=std::getenv("HALO2_ENGINE_PREPARED_ACTIVE_LIMIT");
                if(edited_limit) {
                    options.variant.values.append(90,std::stoi(edited_limit));
                    variant=source_game_variant(options.variant);
                }
                const auto type=source_game_variant_type(variant);
                options.local_players=edited_limit||type==7?4:2;
                const bool teams=source_game_variant_get(variant,91)!=0;
                if(teams)for(unsigned n=0;n<options.local_players;++n)options.selected_teams.push_back(std::int8_t(edited_limit?n/2:n%2));
                MultiplayerPreparedWorld prepared(options);
                check(prepared.rules.requires_source_engine && prepared.match.source_engine()==type,"Prepared match selected a compatibility rule adapter");
                if(std::getenv("HALO2_ENGINE_PREPARED_VIEWS")) {
                    result["stage"]="initial-first-person-configuration";
                    const auto binding=prepared.cache->load_first_person_binding(prepared.rules.weapon,2);
                    prepared.match.configure_replicated_first_person(binding);
                    prepared.match.configure_local_views(prepared.local_slots,binding);
                    result["configured_local_views"]=prepared.local_slots.size();
                }
                auto& world=prepared.source_objects->tu15_world();auto& memory=world.memory();
                check(world.multiplayer_engine_state().recording.enabled,"Prepared host omitted original retained-result startup");
                const auto pool=world.source_player_pool(),records=word(memory,pool+0x44u),stride=word(memory,pool+0x24u);
                check(word(memory,word(memory,0x4f63b8u)+0xc14u)==std::uint32_t(type),"Prepared engine selector differs from the actual source registry");
                unsigned current_units{};std::array<unsigned,8> counts{};
                result["players"]=Json::array();
                for(const auto slot:prepared.local_slots) {
                    const auto record=records+slot*stride,current=word(memory,record+0x2cu),membership=word(memory,record+0xc0u,1);
                    if(current!=DatumPool::none)++current_units;
                    if(teams){check(membership<8,"Prepared source team membership is invalid");++counts[membership];}
                    result["players"].push_back({{"slot",slot},{"unit",current},{"flags",word(memory,record+2u,2)},{"membership",membership}});
                }
                const auto limit=source_game_variant_get(variant,90);
                unsigned expected=options.local_players;
                if(limit>0) {
                    if(teams){expected=0;for(auto count:counts)expected+=std::min(count,unsigned(limit));}
                    else expected=std::min(expected,unsigned(limit));
                }
                check(prepared.local_slots.size()==options.local_players && current_units==expected,
                    "Prepared source startup changed roster capacity or bypassed its actual active-unit limit");
                result["current_units"]=current_units;result["joined"]=prepared.local_slots.size();result["active_limit"]=limit;
                const std::array entries{0x14fec0u,0x1d0670u,0x15eba0u,0x13b6f0u,0xb8f40u,0x1cc8d0u,0xb9090u,
                    0xb9180u,0x179cd0u,0xc3430u,0x15f450u,0x1503d0u,0x13b990u,0x168ca1u};
                Json observed=Json::array();SourcePhysicsEntryObserverBinding observation(memory,[&](bool entering,auto entry,const auto&) {
                    if(entering && std::ranges::find(entries,entry)!=entries.end())observed.push_back(entry);
                });
                result["stage"]="ordinary-source-frames";
                for(unsigned n=0;n<3;++n) {
                    observed=Json::array();const auto before=prepared.match.state().tick;prepared.match.step({});
                    const auto state=prepared.match.state();const auto source=world.multiplayer_engine_state();
                    check(observed==Json(entries),"Prepared mode duplicated or omitted original engine/world phases");
                    check(state.tick==before+1 && state.tick==word(memory,word(memory,0x51df54u)+8u),"Prepared source and native clocks diverged");
                    check(std::memcmp(&source.statistics,&prepared.match.source_match_statistics(),sizeof(source.statistics))==0,"Prepared mode retained a parallel score bank");
                    for(const auto slot:prepared.local_slots) {
                        const auto record=records+slot*stride,current=word(memory,record+0x2cu);
                        const auto& p=state.players[slot];
                        check(p.source_lives_remaining==std::bit_cast<std::int16_t>(std::uint16_t(word(memory,record+0x1acu,2))) &&
                            std::uint8_t(p.source_membership)==word(memory,record+0xc0u,1),"Prepared mode lost source life or role/team membership");
                        if(current!=DatumPool::none) {
                            check(p.source_unit==current && !p.dead,"Prepared mode did not adopt its source-born unit");
                            check(std::memcmp(&p.source_camouflage,memory.view(world.source_object_body(current)+0x2b0u,sizeof(SourceCamouflageAmounts)).data(),sizeof(SourceCamouflageAmounts))==0,
                                "Prepared mode retained a separate camouflage fade");
                        }
                    }
                    if(type==4) {
                        const auto king=word(memory,0x4f63b8u)+0xfcu;
                        check(std::memcmp(prepared.match.source_king_state(),memory.view(king,sizeof(SourceKingState)).data(),sizeof(SourceKingState))==0,
                            "Prepared King retained a separate hill bank");
                    }
                }
                if(std::getenv("HALO2_ENGINE_PREPARED_FINALIZATION")) {
                    result["stage"]="original-match-over-request";
                    // Deliver the real shared request; the normal frame must
                    // consume it and run the selected engine's finalizer.
                    // This is not a played objective/score-limit outcome.
                    world.call_actual(0x13b4a0u);
                    SourceGameEndStats finalized{};bool captured=false;
                    for(unsigned tick=0;tick<211;++tick) {
                        observed=Json::array();prepared.match.step({});
                        const auto state=prepared.match.state();const auto source=world.multiplayer_engine_state();
                        check(observed==Json(entries),"Prepared finalization changed ordinary source frame ownership");
                        check(state.tick==word(memory,word(memory,0x51df54u)+8u),"Prepared finalization lost the original world clock");
                        check(state.finished && source.game_over.active && state.results_finalized==source.finalized,
                            "Prepared match lost original match-over or finalization state");
                        check(state.game_over_countdown==source.game_over.countdown &&
                            state.game_over_fade_started==source.game_over.fade_started,
                            "Prepared mode advanced a separate postgame countdown or fade");
                        check(std::memcmp(&source.statistics,&prepared.match.source_match_statistics(),sizeof(source.statistics))==0,
                            "Prepared finalization retained a separate completed result bank");
                        if(source.finalized) {
                            if(captured)check(std::memcmp(&source.statistics,&finalized,sizeof(finalized))==0,
                                "Repeated ordinary postgame frames changed finalized scores or wins");
                            else {finalized=source.statistics;captured=true;}
                        }
                    }
                    const auto source=world.multiplayer_engine_state();
                    check(captured && source.game_over.complete() && source.game_over.fade_started,
                        "Prepared original finalization did not complete its countdown and fade");
                    check(source.recording.enabled && source.recording.frozen && word(memory,0x51dfb1u,1)==1u,
                        "Original finalization did not complete and freeze its actually enabled recording");
                    unsigned retained_players{},retained_teams{};
                    for(unsigned n=0;n<16;++n) {
                        retained_players+=word(memory,0x51e334u+n*0xa4u,1)!=0u;
                        retained_teams+=word(memory,0x51ed74u+n*0x18u,1)!=0u;
                    }
                    check(word(memory,0x52d698u)==retained_players && word(memory,0x52d69cu)==retained_teams,
                        "Original completed-result projection lost retained player/team rows");
                    result["completed_result_projection"]={{"player_rows",retained_players},{"team_rows",retained_teams},
                        {"mode",word(memory,0x50ed24u)},{"recording_enabled",true},{"recording_frozen",true}};
                    result["completed_game_over"]={{"frames",211},{"countdown",source.game_over.countdown},
                        {"fade",source.game_over.fade_started},{"finalized",source.finalized}};
                }
                result["final_tick"]=prepared.match.state().tick;result["passed"]=true;result["stage"]="complete";
            }catch(const std::exception& error){result["error"]=error.what();++failures;}
            std::cout<<name<<": "<<result.dump()<<'\n';report["cases"].push_back(result);save();continue;
        }
        std::uint32_t last_store{};
        try {
            Files files(index++);Json player_flag_stores=Json::array();bool observing_tick=false;
            const std::array source_tick_entries{0x14fec0u,0x1d0670u,0x15eba0u,0x13b6f0u,
                0xb8f40u,0x1cc8d0u,0xb9090u,0xb9180u,0x179cd0u,0xc3430u,
                0x15f450u,0x1503d0u,0x13b990u,0x168ca1u};
            PhysicsLiveWorldTu15 world(cache,files.path);auto& observed_bank=world.memory();
            SourcePhysicsEntryObserverBinding entry_observation(observed_bank,[&](bool entering,auto entry,const auto&) {
                if(entering && observing_tick && std::ranges::find(source_tick_entries,entry)!=source_tick_entries.end())
                    result["world_tick_entries"].push_back(entry);
            });
            world.observe_physics_stores([&](auto pc,const auto& store){
                last_store=pc;
                // Authenticated first stack stores in the actual owners;
                // no synthetic phase callbacks or admission results.
                if(observing_tick && (pc==0x14fec3u || pc==0x15ebbdu || pc==0xb8f40u ||
                    pc==0x179cd0u || pc==0xc343bu || pc==0x15f453u || pc==0x1503d6u || pc==0x168ca1u))
                    result["world_tick_store_anchors"].push_back(pc);
                if(!spawn || player_flag_stores.size()>=128)return;
                const auto pool=word(observed_bank,0x4f54f4u);
                if(!pool)return;
                const auto records=word(observed_bank,pool+0x44u),stride=word(observed_bank,pool+0x24u);
                if(!stride || store.address<records || store.address>=std::uint64_t(records)+16u*stride)return;
                const auto slot=(store.address-records)/stride,offset=(store.address-records)%stride;
                if(offset>3 || offset+store.written.size()<=2)return;
                const auto after=word(observed_bank,records+slot*stride+2u,2);auto before=after;
                for(unsigned byte=2;byte<4;++byte)if(byte>=offset && byte-offset<store.before.size()) {
                    const auto shift=(byte-2)*8;
                    before=(before&~(255u<<shift))|(std::to_integer<unsigned>(store.before[byte-offset])<<shift);
                }
                if(before!=after)player_flag_stores.push_back({{"pc",pc},{"slot",slot},{"before",before},{"after",after}});
            });
            check(!world.memory().is_read_only(0x56e224u,1) && word(world.memory(),0x56e224u,1)==0,
                "Original time-extension latch lacks its mutable cold byte");
            check(world.memory().is_read_only(0x56e226u,2),
                "Time-extension latch declaration widened into neighboring immutable padding");
            auto variant=source_game_variant_default(row.at("frontend_engine").get<unsigned>());
            for(const auto& write:row.at("writes"))source_game_variant_set(variant,write.at(0).get<int>(),write.at(1).get<int>());
            result["stage"]="configure-session";
            world.configure_session(variant,1,30,0x12345678u);
            auto& memory=world.memory();const auto engine=word(memory,0x4f63b8u);
            check(word(memory,engine+0xc14u)==row.at("source_type").get<unsigned>(),"Native session selected another original engine");
            result["stage"]="initialize-map";world.initialize_map();
            std::vector<std::uint32_t> identities;
            std::vector<SourcePlayerIdentity> platform_identities;
            if(occupied) {
                const auto machine=machine_identity();
                const auto participants=row.at("source_type").get<unsigned>()==7?4u:2u;
                DatumPool players(memory,world.player_pool());
                const bool teams=(word(memory,word(memory,0x4f3218u)+0x184u,1)&1u)!=0;
                result["stage"]="construct-roster";
                for(unsigned slot=0;slot<participants;++slot) {
                    const auto membership=teams?slot%2u:slot;
                    if(teams)world.publish_roster_team(slot,std::int8_t(membership));
                    const auto player=players.allocate_at(std::int32_t(slot));
                    check(player!=DatumPool::none,"Original player allocation rejected the declared roster");
                    auto configuration=source_default_player_configuration();
                    configuration[0x7c]=std::uint8_t(membership);
                    // Match admission runs the recovered14BC80 profile
                    // sanitizer before fresh-player construction. Keep that same multiplayer
                    // representation mapping and authored globals count.
                    SourcePlayerConfigurationContext context;
                    context.game_type=2;
                    context.representation_count=std::int32_t(word(memory,word(memory,0x4ecc1cu)+0x138u));
                    context.selected_engine=true;context.team_mode=teams;context.team_mask=0xff;
                    context.representation_policy=SourceProfileRepresentationPolicy::MultiplayerMapping;
                    source_sanitize_player_configuration(configuration,context);
                    world.construct_player(player,slot,configuration,machine);identities.push_back(player);
                    platform_identities.push_back(source_offline_player_identity(machine,std::int32_t(slot)));
                }
                check(players.count()==std::int32_t(participants),"Native player census lost an allocated source identity");
                result["participants"]=participants;result["player_identities"]=identities;
                result["active_player_census"]=word(memory,word(memory,0x4f54f0u));
                check(result["active_player_census"]==participants,"Original joins did not publish the active player census");
                result["team_mask"]=word(memory,engine+8u,2);
            }
            result["stage"]="activate-round";world.call_actual(0x15afb0u);
            check(word(memory,engine+0x6cu,2)==1 && word(memory,engine+0xc04u)==1,
                "Original round activation did not enter its playing phase");
            const auto owner=word(memory,0x56dfa0u+word(memory,engine+0xc14u)*4u);
            check(owner!=0,"Native playing engine lost its registered interface");
            const auto tick=word(memory,word(memory,owner)+18u*4u);
            if(spawn) {
                // Reproduce the original player's scheduler/engine order with
                // declared caller clock inputs. Object/light/physics phases
                // are separate acceptance, and no birth result is supplied.
                const auto clock=word(memory,0x51df54u);
                const auto initial_tick=word(memory,clock+8u);
                check(word(memory,engine+0x6eu,2)==0,"Fresh player spawning must begin in round zero");
                result["initial_round"]=word(memory,engine+0x6eu,2);
                result["initial_lives"]=Json::array();
                const auto lives_limit=word(memory,word(memory,0x4f3218u)+0x1b8u);
                const auto lives_pool=world.source_player_pool();
                for(const auto player:identities) {
                    const auto record=word(memory,lives_pool+0x44u)+(player&0xffffu)*word(memory,lives_pool+0x24u);
                    const auto lives=word(memory,record+0x1acu,2);
                    check(lives==(lives_limit?lives_limit:0xffffu),"Original new-player notification did not initialize lives");
                    result["initial_lives"].push_back(lives);
                }
                const auto active_limit=source_game_variant_get(variant,90);
                check(word(memory,word(memory,0x4f3218u)+0x1b4u)==std::uint32_t(active_limit),
                    "Original active-player limit did not reach its runtime consumer");
                unsigned expected_units=unsigned(identities.size());
                if(active_limit>0) {
                    if((word(memory,word(memory,0x4f3218u)+0x184u,1)&1u)!=0) {
                        std::array<unsigned,8> teams{};const auto pool=world.source_player_pool();
                        for(const auto player:identities) {
                            const auto record=word(memory,pool+0x44u)+(player&0xffffu)*word(memory,pool+0x24u);
                            const auto team=word(memory,record+0xc0u,1);
                            check(team<teams.size(),"Original team roster has no active-limit group");++teams[team];
                        }
                        expected_units=0;for(const auto count:teams)expected_units+=std::min(count,unsigned(active_limit));
                    }else expected_units=std::min(expected_units,unsigned(active_limit));
                }
                result["active_player_limit"]=active_limit;result["expected_current_units"]=expected_units;
                const auto simulation_world=word(memory,0x4dc04cu);
                result["activation_inputs"]={{"simulation_active",word(memory,0x4dc040u,1)},
                    {"simulation_world_kind",word(memory,simulation_world+8u)},{"players",Json::array()}};
                const auto input_pool=world.source_player_pool();
                for(const auto player:identities) {
                    const auto record=word(memory,input_pool+0x44u)+(player&0xffffu)*word(memory,input_pool+0x24u);
                    auto active=world.prepare_source_call();active.gp[0]=record+0x14u;
                    world.run_actual(0x69ad0u,active);
                    const auto expected=std::as_bytes(std::span(platform_identities.at(player&0xffffu)));
                    check(std::equal(expected.begin(),expected.end(),memory.view(record+4u,12).begin()),
                        "Original player ID lost its actual platform-address producer");
                    check((active.gp[0]&255u)==1,"Original activation rejected this simulation's local machine key");
                    result["activation_inputs"]["players"].push_back({{"player",player},
                        {"source_predicate",active.gp[0]&255u},
                        {"platform_player_id_preserved",true},
                        {"machine_matches_world",std::equal(memory.view(record+0x14u,6).begin(),
                            memory.view(record+0x14u,6).end(),memory.view(simulation_world+0xdu,6).begin())}});
                }
                result["spawn_frames"]=Json::array();
                Json prior_units,prior_flags;
                bool complete=false;
                for(unsigned frame=0;frame<240 && !complete;++frame) {
                    const auto now=initial_tick+frame;
                    if(world_tick) {
                        result["stage"]="source-world-tick-"+std::to_string(frame);
                        check(word(memory,clock+8u)==now,"Source world lost its own sequential clock");
                        result["world_tick_entries"]=Json::array();
                        result["world_tick_store_anchors"]=Json::array();observing_tick=true;
                        world.step_objects();observing_tick=false;
                        check(word(memory,clock+8u)==now+1u,"Source world tick did not advance its clock once");
                        const std::array expected{0x14fec3u,0x15ebbdu,0xb8f40u,0x179cd0u,
                            0xc343bu,0x15f453u,0x1503d6u,0x168ca1u};
                        check(result["world_tick_store_anchors"]==Json(expected),
                            "Source world phase stores lost their original order or single owner");
                        check(result["world_tick_entries"]==Json(source_tick_entries),
                            "Source world entry order differs from its original engine/object/player schedule");
                    }else {
                        std::array<std::byte,4> clock_input{};
                        for(unsigned i=0;i<4;++i)clock_input[i]=std::byte(now>>(i*8));
                        memory.write(clock+8u,clock_input);
                        result["stage"]="source-player-scheduler-"+std::to_string(frame);
                        world.call_actual(0x14fec0u,std::array{0x60028000u});
                        result["stage"]="spawn-shared-engine-frame-"+std::to_string(frame);
                        world.call_actual(0x15eba0u);
                        result["stage"]="spawn-post-object-engine-phase-"+std::to_string(frame);
                        world.call_actual(0x15f450u);
                        result["stage"]="spawn-late-player-phase-"+std::to_string(frame);
                        world.call_actual(0x1503d0u);
                    }
                    check(word(memory,engine+0x6eu,2)==0,"Fresh player spawning advanced beyond round zero");
                    Json units=Json::array(),flags=Json::array(),timers=Json::array();unsigned live_units{};
                    const auto pool=world.source_player_pool();
                    for(const auto player:identities) {
                        const auto record=word(memory,pool+0x44u)+(player&0xffffu)*word(memory,pool+0x24u);
                        const auto unit=word(memory,record+0x2cu);units.push_back(unit);
                        flags.push_back(word(memory,record+2u,2));timers.push_back(word(memory,record+0x164u));
                        if(unit!=DatumPool::none) {
                            ++live_units;check(world.source_object_body(unit)!=0,"Original spawn published an absent native object body");
                        }
                    }
                    complete=live_units==expected_units;
                    if(frame<4 || frame%30==0 || frame==239 || complete ||
                        units!=prior_units || flags!=prior_flags)
                        result["spawn_frames"].push_back({{"caller_tick",now},{"units",units},
                            {"player_flags",flags},{"respawn_timers",timers},
                            {"round",word(memory,engine+0x6eu,2)},{"phase",word(memory,engine+0x6cu,2)}});
                    prior_units=units;prior_flags=flags;result["spawn_phase_calls"]=frame+1;
                    result["player_flag_stores"]=player_flag_stores;
                }
                check(complete,"Original player scheduler did not create its permitted unit count within the declared clock inputs");
                result["spawned_loadouts"]=Json::array();
                result["active_limit_waiters"]=Json::array();
                const auto pool=world.source_player_pool();
                for(const auto player:identities) {
                    const auto record=word(memory,pool+0x44u)+(player&0xffffu)*word(memory,pool+0x24u);
                    const auto unit=word(memory,record+0x2cu);
                    if(unit==DatumPool::none) {
                        auto blocked=world.prepare_source_call();blocked.gp[0]=player;
                        world.run_actual(0x1601b0u,blocked);
                        check((blocked.gp[0]&255u)==1 && (word(memory,record+2u,2)&0x4000u)!=0,
                            "Absent joined unit lacks the original active-limit predicate and exclusion flag");
                        result["active_limit_waiters"].push_back({{"player",player},{"source_predicate",1},
                            {"player_flags",word(memory,record+2u,2)}});continue;
                    }
                    const auto body=world.source_object_body(unit);
                    const auto equipment=world.player_equipment(unit);
                    Json weapons=Json::array();
                    for(unsigned slot=0;slot<equipment.weapons.size();++slot)if(equipment.weapons[slot]!=DatumPool::none) {
                        const auto* tag=cache.find_tag(TagHandle{equipment.definitions[slot]});
                        check(tag && tag->group=="weap","Original loadout lacks its actual MCC weapon definition");
                        weapons.push_back({{"slot",slot},{"object",equipment.weapons[slot]},
                            {"definition",equipment.definitions[slot]},{"name",tag->name}});
                    }
                    result["spawned_loadouts"].push_back({{"player",player},{"unit",unit},
                        {"definition",word(memory,body)},{"weapons",weapons},{"grenades",equipment.grenades}});
                }
                if(world_tick) {
                    result["stage"]="external-match-adapter-world-tick";
                    result["source_world_tick_store_anchors"]=result["world_tick_store_anchors"];
                    result["source_world_tick_entries"]=result["world_tick_entries"];
                    result["world_tick_store_anchors"]=Json::array();
                    result["world_tick_entries"]=Json::array();
                    const auto before=word(memory,clock+8u);observing_tick=true;
                    world.step_objects(SourceEngineFrameOwnerTu15::MatchAdapter);observing_tick=false;
                    const std::array expected{0x14fec3u,0xb8f40u,0x179cd0u,0xc343bu,0x168ca1u};
                    check(result["world_tick_store_anchors"]==Json(expected),
                        "External match adapter duplicated source engine/player phases");
                    const std::array expected_entries{0x14fec0u,0x1d0670u,0x13b6f0u,0xb8f40u,
                        0x1cc8d0u,0xb9090u,0xb9180u,0x179cd0u,0xc3430u,0x168ca1u};
                    check(result["world_tick_entries"]==Json(expected_entries),
                        "External match adapter delivered a duplicate source engine/player entry");
                    check(word(memory,clock+8u)==before+1u,"External match adapter world clock advanced incorrectly");
                    result["match_adapter_tick_store_anchors"]=result["world_tick_store_anchors"];
                    result["match_adapter_tick_entries"]=result["world_tick_entries"];
                }
            }
            if(!world_tick){result["stage"]="shared-engine-frame";world.call_actual(0x15eba0u);}
            result["phase"]=word(memory,engine+0x6cu,2);
            result["requested_state"]=word(memory,engine+0xc04u);
            result["registered_items"]=word(memory,engine+0xc18u);
            result["object_pool_count"]=word(memory,world.source_object_pool()+0x3cu);
            result["current_units"]=Json::array();
            const auto player_pool=world.source_player_pool();
            for(const auto player:identities)
                result["current_units"].push_back(word(memory,word(memory,player_pool+0x44u)+
                    (player&0xffffu)*word(memory,player_pool+0x24u)+0x2cu));
            result["tick_entry"]=tick;result["shared_frame_entry"]=0x15eba0u;
            result["home_returns"]=Json::array();
            const auto kind=row.at("source_type").get<unsigned>();
            if(kind==1 || kind==9) {
                const auto count=word(memory,engine+0xc18u);
                check(count<=8,"Original objective registry escaped its source capacity");
                for(unsigned slot=0;slot<count;++slot) {
                    const auto item=word(memory,engine+0xc1cu+slot*16u);
                    const auto body=world.source_object_body(item);
                    const auto objective=word(memory,body+0x17eu,2);
                    auto home=world.prepare_source_call();home.gp[0]=objective;
                    world.run_actual(0x24b4c0u,home);
                    if(home.gp[0]==0xffffffffu)continue;
                    const auto scenario=word(memory,0x4ecc20u);
                    check(home.gp[0]<word(memory,scenario+0x118u),"Original home escaped its authored marker block");
                    const auto point=word(memory,scenario+0x11cu)+home.gp[0]*32u;
                    const std::array original{word(memory,point),word(memory,point+4u),word(memory,point+8u)};
                    const Vec3 moved{std::bit_cast<float>(original[0])+1.f,
                        std::bit_cast<float>(original[1]),std::bit_cast<float>(original[2])};
                    result["stage"]="objective-external-movement";world.set_object_position(item,moved);
                    check(word(memory,body+0x64u)==std::bit_cast<std::uint32_t>(moved.x),
                        "Actual objective movement did not retain the external source input");
                    const bool after_match_over=word(memory,word(memory,0x4f3218u)+0x1128u,1)!=0;
                    auto reset=world.prepare_source_call(std::array{1u});reset.gp[0]=item;
                    result["stage"]="objective-home-return";world.run_actual(0x24b950u,reset);
                    for(unsigned axis=0;axis<3;++axis)
                        check(word(memory,body+0x64u+axis*4u)==original[axis],
                            "Original objective return did not restore the authored home position");
                    result["home_returns"].push_back({{"item",item},{"objective_index",objective},
                        {"marker",home.gp[0]},{"after_match_over",after_match_over},{"position_bits",original}});
                }
            }
            if(rounds) {
                // Deliver a pending-state input through original1628D0, then
                // re-enter the actual post-object phase. This tests its source
                // countdown/reset sequence without claiming full game ticks.
                const auto initial_round=word(memory,engine+0x6eu,2);
                auto request=world.prepare_source_call();request.gp[2]=2;
                result["stage"]="request-round-intermission";world.run_actual(0x1628d0u,request);
                check(word(memory,engine+0xc04u)==2,"Original state-request owner did not retain intermission");
                result["round_boundaries"]=Json::array();
                for(unsigned call=1;call<=181;++call) {
                    result["stage"]="post-object-round-phase-"+std::to_string(call);
                    world.call_actual(0x15f450u);
                    if(call==1 || call==150 || call==151 || call==180 || call==181) {
                        const auto phase=word(memory,engine+0x6cu,2);
                        const auto timer=word(memory,engine+0xc00u);
                        const auto round=word(memory,engine+0x6eu,2);
                        const auto expected_phase=call<=150?2u:call<=180?3u:1u;
                        const auto expected_timer=call<=150?151u-call:call<=180?181u-call:0u;
                        check(phase==expected_phase,"Original later phase missed a round boundary");
                        if(call!=181)check(timer==expected_timer,"Original round countdown diverged");
                        check(round==initial_round+(call>=151?1u:0u),"Original later phase lost its round index");
                        result["round_boundaries"].push_back({{"call",call},{"phase",phase},
                            {"timer",timer},{"round",round}});
                    }
                }
                check(word(memory,engine+0xc04u)==1,"Original later phase did not activate the next round");
                check(word(memory,word(memory,0x4f3218u)+0x1128u,1)==0,
                    "Bounded round transition unexpectedly ended the match");
            }
            if(rounds) {
                result["stage"]="request-match-over";world.call_actual(0x13b4a0u);
                result["stage"]="post-object-match-finalization";world.call_actual(0x15f450u);
                check(word(memory,engine+0xc08u,1)==1,"Original match finalizer did not publish its once latch");
                check((word(memory,engine)&0x20u)!=0,"Original match finalizer did not publish retained results");
                check(word(memory,word(memory,0x4f3218u)+0x1128u,1)!=0,
                    "Original match-over request was lost during finalization");
                const auto completed=memory.view(engine,0x1000u);
                const std::vector<std::byte> before(completed.begin(),completed.end());
                result["stage"]="post-object-finalization-repeat";world.call_actual(0x15f450u);
                check(std::equal(before.begin(),before.end(),memory.view(engine,0x1000u).begin()),
                    "Original finalization once guard changed the completed engine bank");
                result["finalized"]=true;result["final_phase"]=word(memory,engine+0x6cu,2);
                result["final_requested_state"]=word(memory,engine+0xc04u);
            }
            result["passed"]=true;result["stage"]="complete";
        }catch(const std::exception& error) {result["error"]=error.what();++failures;}
        result["last_store_instruction"]=last_store;
        std::cout<<name<<": "<<result.dump()<<'\n';report["cases"].push_back(result);save();
    }
    check(index>0,"No authored native engine case selected");
    report["passed"]=failures==0;save();return failures?1:0;
}catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
