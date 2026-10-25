#pragma once
#include "halo2/multiplayer_world.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <cstring>

namespace halo2::test {
// Declared source position/control inputs to the genuine original objective
// owners. No score, timer, objective ownership, winner or completion output is
// supplied. Shared with the ONLINE lifecycle regression, separate from played
// route/physical input acceptance.
inline nlohmann::json complete_objective_match_tu15(MultiplayerPreparedWorld& prepared,
    const UiVariant& selected,std::array<Controls,multiplayer_capacity>& controls,
    const std::function<void()>& step) {
    const auto need=[](bool value,const char* why){if(!value)throw FormatError(why);};
    auto& match=prepared.match;auto& objects=*prepared.source_objects;
    auto& world=objects.tu15_world();auto& memory=world.memory();
    const auto word=[](const SourceRuntimeMemory& m,std::uint32_t p){return Bytes(m.view(p,4)).u32(0);};
    const auto half=[](const SourceRuntimeMemory& m,auto p){return std::bit_cast<std::int16_t>(Bytes(m.view(p,2)).u16(0));};
    const auto variant=source_game_variant(selected);const auto type=source_game_variant_type(variant);
    need(type==1 || type==9,"Objective completion requires original CTF/Assault rules");
    std::vector<unsigned> participants;
    for(unsigned slot=0;slot<multiplayer_capacity;++slot)if(match.player(slot).connected)participants.push_back(slot);
    need(participants.size()==2,"Objective ONLINE cohort requires its actual two admitted players");
    const auto carrier_item=[&](unsigned slot) {
        auto abi=world.prepare_source_call(std::array{1u});abi.gp[0]=match.player(slot).source_datum;
        world.run_actual(0x1a8fd0u,abi);return abi.gp[0];
    };
    nlohmann::json row={{"goals",nlohmann::json::array()}};
    using Json=nlohmann::json;
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
                    for(unsigned n=0;n<count && carried==null_datum;++n) {
                        const auto item=word(memory,engine+0xc1cu+n*16u);if(item==null_datum)continue;
                        const auto owner=half(memory,objects.object_body(item)+0x17eu);
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
                }
                need(match.state().team_scores[unsigned(member)]==before+1,"Complete match goal did not award exactly one original point");
                row["goals"].push_back({{"round",round},{"carrier",carrier},{"team",member},{"marker",goal},
                    {"before",before},{"after",match.state().team_scores[unsigned(member)]},
                    {"pickup_and_goal_frames",match.state().tick-before_tick},{"goal_frames",frames}});
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
    return row;
}
}
