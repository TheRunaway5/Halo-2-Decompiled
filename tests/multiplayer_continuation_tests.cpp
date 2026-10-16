#include "halo2/multiplayer_authority.hpp"
#include "halo2/original_data.hpp"
#include <algorithm>
#include <bit>
#include <cstring>
#include <iostream>

using namespace halo2;
namespace {
void check(bool ok,const char *why){if(!ok)throw FormatError(why);}
template<class F>void rejects(F f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}
    check(rejected,"Invalid, active or faulted match checkpoint accepted");}
constexpr MultiplayerMatch::AuthorityDigest binding{std::byte{0x15},std::byte{0x4d}};
bool same(float a,float b){return std::bit_cast<std::uint32_t>(a)==std::bit_cast<std::uint32_t>(b);}
bool same(Vec3 a,Vec3 b){return same(a.x,b.x)&&same(a.y,b.y)&&same(a.z,b.z);}
void view(const FirstPersonViewBinding &a,const FirstPersonViewBinding &b) {
    check(a.unit==b.unit&&a.representation==b.representation,"View unit/profile owner changed");
    for(unsigned i=0;i<2;++i){const auto &x=a.hands[i],&y=b.hands[i];
        check(x.flags==y.flags&&x.weapon==y.weapon&&x.graph==y.graph&&x.activity==y.activity&&x.animation_identifier==y.animation_identifier&&
            x.weapon_nodes==y.weapon_nodes&&x.arms_nodes==y.arms_nodes&&x.events==y.events&&x.future_events==y.future_events&&
            x.channel_delay==y.channel_delay&&x.fire_ticks==y.fire_ticks&&same(x.frame,y.frame)&&same(x.rate,y.rate),"First-person channel continuation changed");}
}
void equal(const MultiplayerMatch::AuthorityCheckpoint &a,const MultiplayerMatch::AuthorityCheckpoint &b) {
    check(a.admitted_binding==b.admitted_binding&&a.configuration_binding==b.configuration_binding&&a.service_roles==b.service_roles&&
        a.simulation.ticks==b.simulation.ticks&&a.simulation.random_state==b.simulation.random_state&&
        a.simulation.shots==b.simulation.shots&&a.simulation.hits==b.simulation.hits&&a.simulation.scripts==b.simulation.scripts&&
        a.finished==b.finished&&a.results_finalized==b.results_finalized&&a.winner==b.winner&&a.winner_team==b.winner_team,
        "Match clock/RNG/script/lifecycle continuation changed");
    check(a.retained_stats.players==b.retained_stats.players&&a.retained_stats.teams==b.retained_stats.teams&&a.retained_stats.pvp==b.retained_stats.pvp&&
        a.retained_recording.enabled==b.retained_recording.enabled&&a.retained_recording.frozen==b.retained_recording.frozen&&
        a.result_events.index==b.result_events.index&&a.result_events.start==b.result_events.start&&a.result_events.last==b.result_events.last&&
        a.result_events.events==b.result_events.events&&a.result_events_started==b.result_events_started,
        "Retained result statistics, recording gates or event ring changed");
    check(a.round_lifecycle.flags==b.round_lifecycle.flags&&a.round_lifecycle.phase==b.round_lifecycle.phase&&
        a.round_lifecycle.index==b.round_lifecycle.index&&a.round_lifecycle.start_tick==b.round_lifecycle.start_tick&&
        a.round_lifecycle.ranking==b.round_lifecycle.ranking&&a.round_lifecycle.clock_seconds==b.round_lifecycle.clock_seconds&&
        a.round_lifecycle.countdown==b.round_lifecycle.countdown&&a.round_lifecycle.status==b.round_lifecycle.status&&
        a.game_over.active==b.game_over.active&&a.game_over.countdown==b.game_over.countdown&&a.game_over.fade_started==b.game_over.fade_started&&
        a.team_scores==b.team_scores&&a.retained_team_mask==b.retained_team_mask&&
        a.garbage_queue.head==b.garbage_queue.head&&a.garbage_queue.active_count==b.garbage_queue.active_count,
        "Round, postgame clock, ranking or garbage ownership changed");
    check(std::memcmp(&a.king_state,&b.king_state,sizeof(SourceKingState))==0&&
        a.king_hills.count==b.king_hills.count&&a.king_hills.indices==b.king_hills.indices&&
        a.team_mapping.assigned==b.team_mapping.assigned&&a.team_mapping.teams==b.team_mapping.teams&&
        a.team_mapping.designators==b.team_mapping.designators,"Original hill, movement/occupancy or team mapping bank changed");
    check(std::ranges::equal(a.owned_players->elements(),b.owned_players->elements())&&
        std::ranges::equal(a.owned_players->bitmap(),b.owned_players->bitmap())&&a.owned_players->next_salt()==b.owned_players->next_salt()&&
        a.retained_units.size()==b.retained_units.size()&&a.corpses.size()==b.corpses.size()&&a.weapon_pickups.size()==b.weapon_pickups.size(),
        "Player pool, retained units, corpse or pickup bank changed");
    for(unsigned i=0;i<16;++i){const auto &p=a.players[i],&q=b.players[i];
        check(p.connected==q.connected&&p.source_datum==q.source_datum&&p.name==q.name&&p.source_configuration==q.source_configuration&&
            p.source_profile_request==q.source_profile_request&&p.score==q.score&&p.kills==q.kills&&p.deaths==q.deaths&&
            p.assists==q.assists&&p.suicides==q.suicides&&p.source_current_object==q.source_current_object&&
            p.source_previous_unit==q.source_previous_unit&&p.respawn.countdown==q.respawn.countdown&&p.source_flags==q.source_flags,
            "Player identity/profile/score/respawn continuation changed");
        const auto &u=(i?a.simulation.actors[i-1]:a.simulation.player).value,&v=(i?b.simulation.actors[i-1]:b.simulation.player).value;
        check(u.source_datum==v.source_datum&&u.source_player==v.source_player&&u.exists==v.exists&&u.vitality.dead==v.vitality.dead&&
            same(u.body.position,v.body.position)&&same(u.body.velocity,v.body.velocity)&&same(u.vitality.body,v.vitality.body)&&
            same(u.vitality.shield,v.vitality.shield)&&u.weapon.loaded==v.weapon.loaded&&u.weapon.reserve==v.weapon.reserve&&
            u.source_inventory.slots==v.source_inventory.slots,"Native live unit/inventory continuation changed");
        view(a.replicated_fp_views[i],b.replicated_fp_views[i]);view(a.replicated_fp_origins[i],b.replicated_fp_origins[i]);
    }
    for(unsigned i=0;i<4;++i){view(a.first_person_views[i],b.first_person_views[i]);view(a.first_person_clock_origins[i],b.first_person_clock_origins[i]);}
    for(unsigned i=0;i<16;++i)check(a.end_stats.players[i].fields==b.end_stats.players[i].fields&&
        a.end_stats.players[i].last_kill_tick==b.end_stats.players[i].last_kill_tick,"Original per-player match statistics changed");
    for(unsigned i=0;i<8;++i)check(a.end_stats.teams[i].fields==b.end_stats.teams[i].fields,"Original team match statistics changed");
    check(a.weapon_effect_serial==b.weapon_effect_serial&&a.weapon_effect_journal.size()==b.weapon_effect_journal.size()&&
        a.rumble_serial==b.rumble_serial&&a.rumble_events.size()==b.rumble_events.size(),"Effect/rumble journal frontier changed");
}
void place(MultiplayerMatch &match,unsigned id,Vec3 position) {
    Spawn spawn;spawn.position=position;DebugGameRequest request;request.kind=DebugGameRequest::Teleport;request.player=id;match.debug_request(request,&spawn);
}
void run(const char *xbe,const char *map) {
    Cache ac(map,{},OriginalData::load_xbe(xbe)),bc(map,{},OriginalData::load_xbe(xbe));
    auto first=ac.load_content(0,true),second=bc.load_content(0,true);
    check(first.original_data->image==OriginalData::Image::TitleUpdate15&&second.original_data->image==OriginalData::Image::TitleUpdate15,
        "Match acceptance requires the selected TU1.5 executable");
    // Declared native debug-arena inputs, retaining independently loaded TU1.5
    // player/SMG, animation, stat and lifecycle operands. These are not an
    // original Lockout simulation, peer promotion or a retained-departure test.
    first.collision.clear();first.kill_triggers.clear();second.collision.clear();second.kill_triggers.clear();
    auto rules=slayer_rules({"Slayer",0,{}});rules.weapon=ac.slayer_starting_weapon(5);
    rules.score_limit=2;rules.time_ticks=0;rules.respawn_ticks=30;
    unsigned a_callbacks=0,b_callbacks=0,a_events=0,b_events=0;bool interrupt=false;
    std::uint32_t a_wall=100000,b_wall=900000;
    MultiplayerMatch a(first,0,ac.spawns(),rules,{},[&]{return a_wall;});
    MultiplayerMatch b(second,0,bc.spawns(),rules,{},[&]{return b_wall;});
    const std::array<unsigned,4> local{0,1,2,3};
    for(auto *match:{&a,&b}) {
        for(unsigned i=0;i<4;++i)check(match->join("Native seat "+std::to_string(i))==i,"Four-seat native admission failed");
        const auto fp=(match==&a?ac:bc).load_first_person_binding(rules.weapon,2);
        match->configure_replicated_first_person(fp);match->configure_local_views(local,fp);match->configure_weapon_pickups();
        DebugGameSettings settings;for(unsigned i=0;i<4;++i)settings.players[i].noclip=true;match->debug_settings(settings);
    }
    a.set_game_engine_presentation_owner([&](const auto&){++a_callbacks;rejects([&]{a.authority_checkpoint(binding);});});
    b.set_game_engine_presentation_owner([&](const auto&){++b_callbacks;rejects([&]{b.authority_checkpoint(binding);});if(interrupt)throw 17;});
    a.configure_first_person_event_owner([&](auto,auto,const auto&,const auto&){++a_events;rejects([&]{a.authority_checkpoint(binding);});});
    b.configure_first_person_event_owner([&](auto,auto,const auto&,const auto&){++b_events;rejects([&]{b.authority_checkpoint(binding);});});
    std::array<Controls,16> inputs{};for(unsigned i=0;i<100;++i){++a_wall;a.step(inputs);}
    place(a,0,{0,0,0});place(a,1,{1,0,0});place(a,2,{100,100,0});place(a,3,{-100,100,0});
    inputs[0].fire=true;
    for(unsigned i=0;i<180&&!a.unit(1).vitality.dead;++i){++a_wall;a.step(inputs);}
    check(a.player(0).kills==1&&a.player(1).deaths==1&&!a.finished(),"Actual stock SMG did not produce a recorded first death");
    inputs={};for(unsigned i=0;i<5;++i){++a_wall;a.step(inputs);}
    const auto saved=a.authority_checkpoint(binding),before=b.authority_checkpoint(binding);
    check(saved.players[1].respawn.countdown>0&&saved.result_events_started&&saved.retained_recording.enabled&&
        !saved.corpses.empty()&&!saved.weapon_pickups.empty(),"Fixture lost pending respawn/recording/corpse/pickup owners");
    {auto cancelled=b.prepare_authority(saved,binding);rejects([&]{b.step(inputs);});rejects([&]{b.take_sounds();});
        rejects([&]{b.authority_checkpoint(binding);});rejects([&]{b.prepare_authority(saved,binding);});
        check(!a.commit_authority(std::move(cancelled)),"Foreign match transaction committed");}
    equal(before,b.authority_checkpoint(binding));
    auto malformed=saved;malformed.version=1;rejects([&]{b.prepare_authority(malformed,binding);});
    malformed=saved;malformed.configuration_binding[0]^=std::byte{1};rejects([&]{b.prepare_authority(malformed,binding);});
    malformed=saved;malformed.service_roles^=1u;rejects([&]{b.prepare_authority(malformed,binding);});
    malformed=saved;malformed.players[1].source_datum^=0x10000u;rejects([&]{b.prepare_authority(malformed,binding);});
    malformed=saved;malformed.players[0].source_current_object^=0x10000u;rejects([&]{b.prepare_authority(malformed,binding);});
    malformed=saved;malformed.result_events.index=1000;rejects([&]{b.prepare_authority(malformed,binding);});
    malformed=saved;malformed.simulation.scripts.version=99;rejects([&]{b.prepare_authority(malformed,binding);});
    equal(before,b.authority_checkpoint(binding));
    auto staged=b.prepare_authority(saved,binding);check(b.commit_authority(std::move(staged))&&!b.commit_authority(std::move(staged)),"Match publication failed or reused token");
    equal(saved,b.authority_checkpoint(binding));
    unsigned compared=0;
    for(unsigned i=0;i<180;++i) {
        if(i==55){place(a,0,{0,0,0});place(b,0,{0,0,0});place(a,1,{1,0,0});place(b,1,{1,0,0});inputs[0].fire=true;}
        if(a.finished())inputs={};
        ++a_wall;++b_wall;a.step(inputs);b.step(inputs);equal(a.authority_checkpoint(binding),b.authority_checkpoint(binding));++compared;
    }
    check(a.finished()&&b.finished()&&a.player(0).kills==2&&b.player(1).deaths==2&&
        a_callbacks>0&&b_callbacks==compared&&a_events>0&&b_events>0,"Continued match lost scoring/end phase or local callback owners");
    const auto final=a.authority_checkpoint(binding);
    check(!final.retained_units.empty()&&final.retained_recording.frozen,"Final checkpoint omitted respawned corpse or frozen results");
    malformed=final;malformed.retained_units.begin()->second.value.custom_animation=std::make_shared<AnimationClip>();
    rejects([&]{b.prepare_authority(malformed,binding);});
    malformed=final;const auto retained=malformed.retained_units.begin()->first;
    malformed.simulation.owned_lifetimes->release(retained);rejects([&]{b.prepare_authority(malformed,binding);});
    staged=b.prepare_authority(saved,binding);check(b.commit_authority(std::move(staged)),"Match rewind preparation failed");
    interrupt=true;bool interrupted=false;++b_wall;try{b.step({});}catch(int value){interrupted=value==17;}
    check(interrupted,"Declared presentation interruption did not execute");rejects([&]{b.authority_checkpoint(binding);});interrupt=false;
    staged=b.prepare_authority(final,binding);check(b.commit_authority(std::move(staged)),"Faulted match could not restore an admitted boundary");
    equal(final,b.authority_checkpoint(binding));
    std::cout<<"PASS TU1.5 native match transaction: four local views, real stock SMG deaths/respawn, original score/stat/event recording, pickups/corpses, 180 future steps through game over, cancellation/foreign/malformed/staged/callback/fault recovery\n";
}
Vec3 interior(const SourceKingState &hill) {
    SourceKingPlayer player;player.datum=0;player.current_unit=0;
    for(float z:{hill.center[2],(hill.minimum_z+hill.maximum_z)*.5f})for(int x=-9;x<=9;++x)for(int y=-9;y<=9;++y) {
        const std::array<float,3> point{hill.center[0]+hill.radius*float(x)*.1f,hill.center[1]+hill.radius*float(y)*.1f,z};
        if(source_king_contains(hill,player,[&](auto){return point;}))return {point[0],point[1],point[2]};
    }
    throw FormatError("Actual authored King hill has no sampled interior");
}
void king(const char *xbe,const char *map) {
    Cache ac(map,{},OriginalData::load_xbe(xbe)),bc(map,{},OriginalData::load_xbe(xbe));
    auto first=ac.load_content(0,true),second=bc.load_content(0,true);
    first.collision.clear();first.kill_triggers.clear();second.collision.clear();second.kill_triggers.clear();
    auto rules=king_rules({"Native King continuation",3,{{91,0},{40,1},{2,0},{0x59,5}}},true);
    rules.weapon=ac.slayer_starting_weapon(5,4);rules.time_ticks=0;rules.score_limit=0;
    MultiplayerMatch *ap=nullptr,*bp=nullptr;unsigned a_records=0,b_records=0,b_notices=0;
    MultiplayerKingServices as,bs;
    as.record=[&](const auto&,auto,auto,auto){++a_records;rejects([&]{ap->authority_checkpoint(binding);});};
    bs.record=[&](const auto&,auto,auto,auto){++b_records;rejects([&]{bp->authority_checkpoint(binding);});};
    as.score_changed=[](const auto&,auto,auto){};bs.score_changed=[](const auto&,auto,auto){};
    as.notification=[](const auto&,const auto&){};bs.notification=[&](const auto&,const auto&){++b_notices;};
    std::uint32_t a_wall=200000,b_wall=900000;
    MultiplayerMatch a(first,0,ac.spawns(),rules,as,[&]{return a_wall;});ap=&a;
    MultiplayerMatch b(second,0,bc.spawns(),rules,bs,[&]{return b_wall;});bp=&b;
    for(auto *match:{&a,&b}){match->join("King native participant");DebugGameSettings settings;settings.players[0].noclip=true;match->debug_settings(settings);}
    for(unsigned i=0;i<80;++i){place(a,0,interior(*a.source_king_state()));++a_wall;a.step({});}
    const auto saved=a.authority_checkpoint(binding);
    check(saved.result_events.index>0&&a_records>0&&saved.players[0].score>0,"Actual King scorer did not produce retained events/points");
    auto staged=b.prepare_authority(saved,binding);check(b.commit_authority(std::move(staged)),"King match publication failed");
    equal(saved,b.authority_checkpoint(binding));
    for(unsigned i=0;i<90;++i){place(a,0,interior(*a.source_king_state()));place(b,0,interior(*b.source_king_state()));++a_wall;++b_wall;a.step({});b.step({});
        equal(a.authority_checkpoint(binding),b.authority_checkpoint(binding));}
    const auto final=b.authority_checkpoint(binding);
    check(final.result_events.index>saved.result_events.index&&b_records>0,"King continuation did not append new real recording events");
    std::cout<<"PASS TU1.5 native King match transaction: actual authored hill/state, scorer/retained event production, local callback retention and 90 future occupancy/recording steps (declared debug positioning)\n";
}
}
int main(int argc,char **argv){try{if(argc!=3||!std::filesystem::exists(argv[1])||!std::filesystem::exists(argv[2]))return 77;
    run(argv[1],argv[2]);king(argv[1],argv[2]);return 0;}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
