#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_event_messages_tu15.hpp"
#include "halo2/multiplayer_event_messages_tu15.hpp"
#include "halo2/collision.hpp"
#include "halo2/kernels.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <set>

using namespace halo2;
using Json=nlohmann::json;
namespace {
void need(bool value,const char* why){if(!value)throw FormatError(why);}
std::uint32_t word(const SourceRuntimeMemory& m,std::uint32_t p){return Bytes(m.view(p,4)).u32(0);}
std::int16_t half(const SourceRuntimeMemory& m,std::uint32_t p){return std::bit_cast<std::int16_t>(Bytes(m.view(p,2)).u16(0));}
float scalar(const SourceRuntimeMemory& m,std::uint32_t p){return Bytes(m.view(p,4)).f32(0);}
struct Files {
    std::filesystem::path path;
    explicit Files(unsigned n) {
        const auto scratch=std::getenv("HALO2_TEST_SCRATCH");need(scratch,"Event-message integration requires owned scratch");
        path=std::filesystem::path(scratch)/("event-messages-"+std::to_string(n));std::filesystem::create_directories(path);
    }
    ~Files(){std::error_code error;std::filesystem::remove_all(path,error);}
};
// Retain source-owned packets before the ordinary 64-event rolling journal
// retires them. Serial is a delivery cursor; the test never supplies a packet.
struct Journal {
    std::vector<SourceMultiplayerEventTu15> events;
    std::uint32_t serial{};
    void observe(const MultiplayerState& state) {
        for(const auto& event:state.source_engine_events)if(event.serial>serial) {
            events.push_back(event);serial=event.serial;
        }
    }
    const SourceMultiplayerEventTu15& require(unsigned type,unsigned subtype,
            std::uint32_t after=0,std::optional<std::uint32_t> cause={})const {
        const auto at=std::ranges::find_if(events,[&](const auto& event) {
            return event.serial>after && event.words[0]==type && event.words[1]==subtype &&
                (!cause || event.words[3]==*cause);
        });
        need(at!=events.end(),"Actual source transition did not publish its expected event packet");return *at;
    }
};
SourceEventMessageTokenResolverTu15 resolver(const SourceEventMessageAssetsTu15& assets,
        const SourceMultiplayerEventTu15& event,const MultiplayerState& state) {
    return [&assets,&event,&state](std::u16string_view token)->std::optional<std::u16string> {
        const auto name=[&](std::uint32_t datum) {
            for(const auto& player:state.players)if(player.connected && player.source_player==datum)
                return source_event_message_utf16_tu15(player.name);
            return std::u16string{};
        };
        if(token==u"cause_player")return name(event.words[3]);
        if(token==u"effect_player")return name(event.words[5]);
        if(token==u"cause_team" || token==u"effect_team") {
            const auto team=event.words[token==u"cause_team"?4:6];
            if(team<assets.team_names.size())return assets.team_names[team];return std::u16string{};
        }
        if(token==u"territory_name" && event.data<assets.territory_names.size())return assets.territory_names[event.data];
        return {};
    };
}
std::vector<SourceEventMessageOutputTu15> output(const SourceEventMessageAssetsTu15& assets,
        const SourceMultiplayerEventTu15& event,const MultiplayerState& state,unsigned slot,unsigned locals=1) {
    const auto& player=state.players[slot];
    return source_event_messages_tu15(assets,event,{player.source_player,player.source_flags,
        std::int8_t(player.source_membership),0},locals,resolver(assets,event,state));
}
Json text_outputs(const std::vector<SourceEventMessageOutputTu15>& rows) {
    auto result=Json::array();for(const auto& row:rows)result.push_back({{"text",source_event_message_utf8_tu15(row.text)},
        {"audience",row.audience},{"response_ordinal",row.response_ordinal},{"string_id",row.string_id},
        {"channel",unsigned(row.channel)}});return result;
}
unsigned message_count(const SourceHudMessages& messages) {
    return unsigned(std::ranges::count_if(messages.rows,[](const auto& row){return row[0x82]!=std::byte{};}));
}
std::vector<std::u16string> message_texts(const SourceHudMessages& messages) {
    std::vector<std::u16string> out;
    for(const auto& row:messages.rows)if(row[0x82]!=std::byte{}) {
        std::u16string text;
        for(unsigned n=4;n<0x80;n+=2) {
            const auto c=std::to_integer<unsigned>(row[n])|(std::to_integer<unsigned>(row[n+1])<<8u);
            if(!c)break;text.push_back(char16_t(c));
        }
        out.push_back(std::move(text));
    }
    return out;
}
void require_text(Json& row,const char* label,const SourceEventMessageAssetsTu15& assets,
        const SourceMultiplayerEventTu15& event,const MultiplayerState& state,unsigned slot,unsigned locals=1) {
    const auto rows=output(assets,event,state,slot,locals);
    need(!rows.empty() && std::ranges::any_of(rows,[](const auto& r){return !r.text.empty();}),
        "Actual authored gameplay event lost its localized native text output");
    // Feed the actual observed packet through the same local consumer used by
    // the runtime. This snapshot copy supplies no expected formatted result.
    auto receipt=state;receipt.source_engine_events={event};
    std::vector<unsigned> local_slots{slot};
    if(locals>1)for(unsigned n=0;n<state.players.size() && local_slots.size()<locals;++n)
        if(n!=slot && state.players[n].connected && !(state.players[n].source_flags&2u))local_slots.push_back(n);
    need(local_slots.size()==locals,"Queue test has insufficient genuine local candidates");
    MultiplayerEventMessageStateTu15 presentation;
    consume_multiplayer_event_messages_tu15(assets,receipt,local_slots,presentation,false);
    const bool literal=std::ranges::any_of(rows,[](const auto& r){return r.channel==SourceEventMessageChannelTu15::literal;});
    // Some assertions inspect an earlier arming packet after its real fuse
    // finished. Its authored lifetime can expire before this actual receipt;
    // advancing its delivery cursor must not let it evict a current notice.
    const bool expired=float(std::bit_cast<std::int32_t>(state.tick-event.tick))*(1.f/30.f)>=4.f;
    if(literal) {
        if(expired)need(presentation.sequence==0,"Expired historical gameplay text entered the current message queue");
        else need(presentation.sequence!=0,"Runtime consumer did not enqueue an actual gameplay literal");
    }
    need(presentation.serial==event.serial,"Runtime consumer did not retire its actual source event delivery cursor");
    const auto queued=presentation.views[slot].messages;
    if(literal && !expired) {
        const auto texts=message_texts(queued);
        for(const auto& expected:rows)if(expected.channel==SourceEventMessageChannelTu15::literal)
            need(std::ranges::find(texts,expected.text.substr(0,62))!=texts.end(),
                "Runtime local queue lost or misformatted its actual recipient's authored message");
    }
    const auto sequence=presentation.sequence;
    consume_multiplayer_event_messages_tu15(assets,receipt,local_slots,presentation,false);
    need(presentation.views[slot].messages==queued && presentation.sequence==sequence,
        "Repeated or overlapping source journal enqueued the same gameplay event twice");
    for(unsigned n=0;n<state.players.size();++n)if(std::ranges::find(local_slots,n)==local_slots.end())
        need(message_count(presentation.views[n].messages)==0,"Runtime consumer leaked text into a nonlocal view");
    row["messages"][label]={{"serial",event.serial},{"tick",event.tick},{"type",event.words[0]},
        {"subtype",event.words[1]},{"recipient_slot",slot},{"local_views",locals},
        {"runtime_queue_rows",message_count(queued)},{"runtime_submissions",sequence},
        {"runtime_expired_skipped",literal && expired},{"outputs",text_outputs(rows)}};
}
UiVariant variant(const Json& preset) {
    UiVariant result{preset.at("name").get<std::string>(),preset.at("frontend_engine").get<unsigned>(),{}};
    for(const auto& w:preset.at("writes"))result.values.append(w[0].get<unsigned>(),w[1].get<int>());return result;
}
struct Flow {
    MultiplayerPreparedWorld& prepared;
    MultiplayerMatch& match;
    MultiplayerObjectRuntime& objects;
    PhysicsLiveWorldTu15& world;
    SourceRuntimeMemory& memory;
    std::vector<unsigned> participants;
    Journal journal;
    std::array<Controls,multiplayer_capacity> controls{};
    Flow(MultiplayerPreparedWorld& p,std::span<const unsigned> roster):prepared(p),match(p.match),objects(*p.source_objects),
        world(objects.tu15_world()),memory(world.memory()),participants(roster.begin(),roster.end()) {
        need(prepared.rules.requires_source_engine && match.source_engine_owned(),"Event flow selected a bounded gameplay adapter");
        need(participants.size()>=2,"Event flow lost its canonical participant roster");
        for(const auto slot:participants){controls[slot].yaw=match.unit(slot).yaw;controls[slot].pitch=match.unit(slot).pitch;}
        journal.observe(match.state());
    }
    void step(){match.step(controls);journal.observe(match.state());}
    void place(unsigned slot,Vec3 position){world.set_object_position(match.player(slot).source_current_object,position);}
    void death(unsigned killer,unsigned victim,unsigned relationship=0) {
        const auto unit=match.player(victim).source_current_object;need(unit!=null_datum,"Declared engine death lacks an actual living unit");
        // The complete original death owner receives declared ABI inputs. It
        // still owns classification, source packet, mode callback and score.
        // This does not qualify the earlier projectile/damage attribution.
        world.call_actual(0x15f8b0u,std::array{match.player(killer).source_datum,
            match.player(victim).source_datum,relationship,63u});
        objects.kill_biped(unit);step();
    }
    void respawn(unsigned slot) {
        const auto previous=match.player(slot).source_current_object;
        for(unsigned n=0;n<600;++n) {
            step();const auto current=match.player(slot).source_current_object;
            if(current!=null_datum && current!=previous && !match.state().players[slot].dead)return;
        }
        throw FormatError("Ordinary engine respawn did not birth the next declared death participant");
    }
};
void ordinary_fatal_hit(Flow& f,unsigned shooter,unsigned victim,Json& row) {
    CollisionWorld collision(f.prepared.content.collision);std::array<Spawn,2> selected{};
    float closest=std::numeric_limits<float>::infinity();
    for(const auto& a:f.prepared.cache->spawns())for(const auto& b:f.prepared.cache->spawns()) {
        const auto distance=length(b.position-a.position);if(distance<2 || distance>8 || distance>=closest)continue;
        const auto eye=a.position+Vec3{0,0,f.prepared.content.player.camera_height};
        const auto delta=b.position+Vec3{0,0,f.prepared.content.player.camera_height}-eye;
        if(collision.raycast(eye,delta/length(delta),length(delta)))continue;
        selected={a,b};closest=distance;
    }
    need(std::isfinite(closest),"Ordinary fatal-hit fixture lacks a clear authored spawn pair");
    // Initial player positions are declared inputs through the complete source
    // mover. Every shot, collision, damage amount, credit, death and packet
    // below must then be produced by ordinary delivered aim/fire controls.
    f.place(shooter,selected[0].position);f.place(victim,selected[1].position);
    unsigned attribution_stores{},death_stores{};
    f.objects.observe_physics_stores_read_only([&](auto pc,const auto&) {
        if(pc>=0x1f2280u && pc<0x1f2888u)++attribution_stores;
        if(pc>=0x15f8b0u && pc<0x1600f4u)++death_stores;
    });
    struct Observe {MultiplayerObjectRuntime& objects;~Observe(){objects.observe_physics_stores_read_only({});}} observation{f.objects};
    const auto aim_step=[&] {
        const auto current=f.match.player(shooter).source_current_object;
        if(current!=null_datum)if(const auto look=f.objects.player_look(f.match.player(shooter).source_datum,current)) {
            const auto& from=f.match.unit(shooter);const auto& to=f.match.unit(victim);
            const auto eye=ordinary_unit_eye(from.body.position,from.definition.camera_height,from.definition.crouch_camera_height,from.crouch);
            const auto delta=to.body.position+Vec3{0,0,to.body.height*.5f}-eye;
            f.controls[shooter].yaw+=std::remainder(std::atan2(delta.y,delta.x)-look->x,Bytes(f.memory.view(0x44923cu,4)).f32(0));
            f.controls[shooter].pitch+=std::atan2(delta.z,std::hypot(delta.x,delta.y))-look->y;
        }
        f.step();
    };
    for(unsigned n=0;n<100;++n)aim_step();
    need(!f.match.unit(shooter).vitality.dead && !f.match.unit(victim).vitality.dead,"Ordinary fatal-hit warmup lost a genuine participant");
    const auto before_kills=f.match.player(shooter).kills,before_deaths=f.match.player(victim).deaths;
    const auto before_loaded=f.match.unit(shooter).weapon.loaded;
    const auto before_tick=f.match.state().tick;
    const auto* weapon=f.prepared.cache->find_tag(TagHandle{f.match.unit(shooter).weapon.definition});
    need(weapon && weapon->group=="weap","Ordinary fatal-hit shooter lacks an actual authored weapon");
    row["stage"]="ordinary-fired-fatal-hit";f.controls[shooter].fire=true;unsigned frames{};
    while(frames<240 && !f.match.unit(victim).vitality.dead && !f.match.finished()){aim_step();++frames;}
    f.controls[shooter].fire=false;
    row["ordinary_fatal_hit"]={{"weapon",weapon->name},{"first_fire_tick",before_tick},{"fatal_tick",f.match.state().tick},
        {"fire_frames",frames},{"before_loaded",before_loaded},{"after_loaded",f.match.unit(shooter).weapon.loaded},
        {"attribution_stores",attribution_stores},{"engine_death_stores",death_stores},
        {"victim_dead",f.match.unit(victim).vitality.dead},{"killer_kills",f.match.player(shooter).kills},
        {"victim_deaths",f.match.player(victim).deaths},{"initial_spawn_separation",closest}};
    need(f.match.unit(victim).vitality.dead && attribution_stores && death_stores,
        "Ordinary firing did not execute actual source attribution and the complete death owner");
    need(f.match.player(shooter).kills==before_kills+1 && f.match.player(victim).deaths==before_deaths+1,
        "Ordinary source fatal hit did not retain its actual credited killer/victim statistics");
}
struct NetworkProbe {
    Flow& flow;
    const SourceEventMessageAssetsTu15& assets;
    LanHost host;
    LanClient client;
    MultiplayerEventMessageStateTu15 presentation;
    unsigned slot{};
    NetworkProbe(Flow& f,const SourceEventMessageAssetsTu15& a,const UiVariant& selected):flow(f),assets(a),
        host(f.match,0,0x6576656e74743135ull,selected),
        client(host.join_address(),0x6576656e74743135ull,1,std::array<std::int8_t,1>{0}) {}
    void synchronize() {
        host.broadcast();client.receive();
        need(client.ready() && client.error().empty(),"Event-message loopback peer is not ready");
        need(client.state().source_engine_events==flow.match.state().source_engine_events,
            "Actual source event packet journal changed through native UDP encode/decode");
    }
    void admit(Json& row) {
        std::array<Controls,1> controls{};
        for(unsigned n=0;n<160 && (!client.ready() || client.slots().empty() ||
                flow.match.player(client.slots()[0]).source_current_object==null_datum);++n) {
            client.send(controls);host.receive(flow.controls);flow.step();host.broadcast();client.receive();
            if(!client.error().empty())throw FormatError("Event-message UDP admission: "+client.error());
        }
        synchronize();need(client.slots().size()==1,"Event-message peer lacks one actual local player");slot=client.slots()[0];
        need(flow.match.player(slot).source_current_object!=null_datum,"Event-message peer never acquired its canonical unit");
        consume_multiplayer_event_messages_tu15(assets,client.state(),client.slots(),presentation,true);
        // There is a genuine pre-join capture in this retained journal. A fresh
        // client must retain its targeted activation, skipping historical
        // score broadcasts regardless of the original authority's raw mode1.
        const auto& state=client.state();bool old_capture{},own_activation{};
        for(const auto& event:state.source_engine_events) {
            old_capture|=event.words[0]==3 && event.words[1]==5;
            own_activation|=event.words[0]==3 && event.words[1]==0 && event.words[2]==state.players[slot].source_player;
        }
        need(old_capture && own_activation,"Late-join fixture lacks real historical capture or targeted admission event");
        const auto& messages=presentation.views[slot].messages;
        need(message_count(messages)==1,"Fresh peer replayed old broadcast text or omitted its actual targeted activation");
        row["network"]={{"remote_slot",slot},{"admission_tick",state.tick},
            {"initial_rows",message_count(messages)},{"journal_packets",state.source_engine_events.size()}};
    }
    void finish(Json& row) {
        synchronize();const auto before=presentation.sequence;
        consume_multiplayer_event_messages_tu15(assets,client.state(),client.slots(),presentation,true);
        need(presentation.sequence!=before,"Future UDP game-over event did not reach the peer's local message queue");
        const auto sequence=presentation.sequence;
        const auto queued=presentation.views[slot].messages;
        consume_multiplayer_event_messages_tu15(assets,client.state(),client.slots(),presentation,true);
        need(presentation.sequence==sequence && presentation.views[slot].messages==queued,"Repeated peer snapshot replayed a received message");
        row["network"]["game_over_rows"]=message_count(queued);row["network"]["final_serial"]=presentation.serial;
    }
};
void objective_flow(Flow& f,unsigned type,const SourceEventMessageAssetsTu15& assets,Json& row) {
    const auto engine=word(f.memory,0x4f63b8u);auto count=word(f.memory,engine+0xc18u);
    for(unsigned n=0;n<8 && !count;++n){f.step();count=word(f.memory,engine+0xc18u);}
    need(count>0 && count<=8,"Source event flow has no genuine objective registry");
    std::vector<std::uint32_t> items;for(unsigned n=0;n<count;++n)items.push_back(word(f.memory,engine+0xc1cu+n*16u));
    std::array<Vec3,multiplayer_capacity> starts{};
    for(const auto slot:f.participants)starts[slot]=f.objects.biped_motion(f.match.player(slot).source_current_object).position;
    const auto carried_by=[&](unsigned slot) {
        auto abi=f.world.prepare_source_call(std::array{type==3?2u:1u});abi.gp[0]=f.match.player(slot).source_datum;
        f.world.run_actual(0x1a8fd0u,abi);return abi.gp[0];
    };
    std::uint32_t carried=null_datum;unsigned carrier{};const auto before=f.journal.serial;
    for(const auto item:items)for(const auto slot:f.participants)if(carried==null_datum) {
        for(const auto other:f.participants)f.place(other,starts[other]);
        auto p=f.objects.object_transform(item).position;p.x-=.2f;f.place(slot,p);
        f.controls[slot].yaw=0;f.controls[slot].pitch=0;
        for(unsigned n=0;n<64 && carried==null_datum;++n) {
            f.controls[slot].pickup=n>=2;f.controls[slot].pickup_milliseconds=n>=2?std::uint16_t((n-2)*1000/30):0;
            f.step();const auto result=carried_by(slot);
            if(result!=null_datum && f.objects.object_parent(result)==f.match.player(slot).source_current_object){carried=result;carrier=slot;}
        }
        f.controls[slot].pickup=false;f.controls[slot].pickup_milliseconds=0;
    }
    need(carried!=null_datum,"Ordinary pickup controls did not trigger real objective acquisition");
    const unsigned category=type==1?3:type==3?4:10;
    const auto& pickup=f.journal.require(category,type==3?2:1,before,f.match.player(carrier).source_datum);
    auto state=f.match.state();
    auto pickup_recipient=carrier;
    if(type==3)pickup_recipient=f.participants[0]==carrier?f.participants[1]:f.participants[0];
    if(type==9) {
        const auto teammate=std::ranges::find_if(f.participants,[&](auto slot) {
            return slot!=carrier && f.match.player(slot).source_membership==f.match.player(carrier).source_membership;
        });
        need(teammate!=f.participants.end(),"Assault pickup requires an actual teammate for its authored text audience");
        pickup_recipient=*teammate;
        need(output(assets,pickup,state,carrier).empty(),"Audio-only Assault carrier pickup invented a message");
    }
    require_text(row,"pickup",assets,pickup,state,pickup_recipient);
    if(type==3) {
        // The pickup is authored but its text is suppressed at two local
        // views. This is the source's local census, independent of networking.
        need(output(assets,pickup,state,carrier).empty(),"Oddball pickup ignored the authored carrier exclusion");
        need(output(assets,pickup,state,pickup_recipient,2).empty(),"Split Oddball pickup ignored the original text suppression");
        const auto score=state.players[carrier].score;
        for(unsigned n=0;n<60;++n)f.step();state=f.match.state();
        need(state.players[carrier].score>score,"Actual Oddball hold did not reach its periodic source score owner");
        const auto drop_before=f.journal.serial;f.controls[carrier].fire=true;f.step();f.controls[carrier].fire=false;
        for(unsigned n=0;n<15 && carried_by(carrier)!=null_datum;++n)f.step();
        const auto& drop=f.journal.require(4,3,drop_before,f.match.player(carrier).source_datum);
        need(output(assets,drop,f.match.state(),carrier).empty(),"Un-authored Oddball drop invented a feed message");
        row["odd_ball_periodic_score"]=state.players[carrier].score;return;
    }
    const auto membership=f.match.player(carrier).source_membership;
    need(membership>=0 && membership<8,"Actual objective carrier lacks team membership");
    const auto bank=word(f.memory,0x52d710u),scenario=word(f.memory,0x4ecc20u),markers=word(f.memory,scenario+0x11cu);
    int goal=-1;
    for(unsigned team=0;team<8 && goal<0;++team) {
        if(type==1?int(team)!=membership:int(team)==membership)continue;
        if(std::ranges::none_of(f.participants,[&](auto slot){return f.match.player(slot).source_membership==int(team);}))continue;
        auto mapping=f.world.prepare_source_call(std::array{team});f.world.run_actual(0x15b3b0u,mapping);
        const auto mapped=std::bit_cast<std::int32_t>(mapping.gp[0]);if(mapped>=0 && mapped<9)goal=half(f.memory,bank+0x14u+unsigned(mapped)*2u);
    }
    need(goal>=0 && unsigned(goal)<word(f.memory,scenario+0x118u),"Actual goal selection has no authored marker");
    const auto marker=Bytes(f.memory.view(markers+unsigned(goal)*32u,12));f.place(carrier,{marker.f32(0),marker.f32(4),marker.f32(8)});
    const auto before_goal=f.journal.serial;
    const auto before_score=state.team_scores[unsigned(membership)];
    const auto options=word(f.memory,0x4f3218u);
    const auto budget=type==1?64u:30u*(unsigned(std::max(0,int(half(f.memory,options+0x244u))))+
        unsigned(std::max(10,int(half(f.memory,options+0x246u)))))+120u;
    for(unsigned n=0;n<budget && f.match.state().team_scores[unsigned(membership)]==before_score;++n)f.step();
    state=f.match.state();need(state.team_scores[unsigned(membership)]==before_score+1,"Authored goal did not award its real source point");
    if(type==9) {
        require_text(row,"arming-started",assets,f.journal.require(10,14,before_goal),state,carrier);
        require_text(row,"arming-completed",assets,f.journal.require(10,15,before_goal),state,carrier);
        require_text(row,"planted",assets,f.journal.require(10,13,before_goal),state,carrier);
    }
    require_text(row,"scored",assets,f.journal.require(category,5,before_goal,f.match.player(carrier).source_datum),state,carrier);
    row["source_point_delta"]=state.team_scores[unsigned(membership)]-before_score;
}
void zone_flow(Flow& f,unsigned type,const SourceEventMessageAssetsTu15& assets,Json& row) {
    const auto actor=f.participants[0],enemy=f.participants[1];
    const auto bank=word(f.memory,0x4f63b8u)+0xfcu;const auto before=f.journal.serial;
    if(type==4) {
        const auto initial=*f.match.source_king_state();need(initial.marker_count>=4,"King has no authored active hill");
        const auto contains=[&](unsigned slot) {
            auto abi=f.world.prepare_source_call();abi.gp[0]=f.match.player(slot).source_datum;abi.gp[7]=bank;
            f.world.run_actual(0x2c8640u,abi);return bool(abi.gp[0]&255u);
        };
        bool found{};Vec3 inside;
        for(const float z:{initial.center[2],(initial.minimum_z+initial.maximum_z)*.5f})
            for(const float x:{0.f,1.f,-1.f,2.f,-2.f})for(const float y:{0.f,1.f,-1.f,2.f,-2.f})if(!found) {
                const Vec3 p{initial.center[0]+initial.radius*x/32,initial.center[1]+initial.radius*y/32,z};
                f.place(actor,p);if(contains(actor)){inside=p;found=true;}
            }
        need(found,"Actual King containment rejected every declared interior position");found=false;
        for(const auto& spawn:f.prepared.cache->spawns())if(!found){f.place(enemy,spawn.position);found=!contains(enemy);}
        need(found,"King has no authored outside position");
        for(unsigned n=0;n<60;++n)f.step();
        const auto state=f.match.state();const auto& controlled=f.journal.require(6,state.team_mode?5:1,before);
        require_text(row,"hill-controlled",assets,controlled,state,actor);
        f.place(enemy,inside);for(unsigned n=0;n<32;++n)f.step();
        // The entering contestant is the source cause. The other hill
        // occupant has no unrelated FFA observer row for this event.
        const auto& contested=f.journal.require(6,state.team_mode?6:2,controlled.serial);
        require_text(row,"hill-contested",assets,contested,f.match.state(),enemy);
        return;
    }
    const auto count=half(f.memory,bank+0x114u);need(count>0 && count<=8,"Territories has no authored region");
    const auto markers=word(f.memory,word(f.memory,0x4ecc20u)+0x11cu);
    struct Zone{unsigned index;Vec3 center;float radius,down,up;};std::vector<Zone> zones;
    for(unsigned n=0;n<unsigned(count);++n)if(const auto marker=half(f.memory,bank+0x60u+n*2u);marker>=0) {
        const auto p=Bytes(f.memory.view(markers+unsigned(marker)*32u,12));zones.push_back({n,{p.f32(0),p.f32(4),p.f32(8)},
            scalar(f.memory,bank+n*4u),scalar(f.memory,bank+0x20u+n*4u),scalar(f.memory,bank+0x40u+n*4u)});
    }
    need(!zones.empty(),"Territories active marker bank is empty");const auto zone=zones[0];
    const auto inside=[](const Zone& z,Vec3 p){const auto x=p.x-z.center.x,y=p.y-z.center.y;return x*x+y*y<z.radius*z.radius && p.z-z.center.z<z.up && z.center.z-p.z<z.down;};
    bool found{};Vec3 outside;
    for(const auto& spawn:f.prepared.cache->spawns())if(!found && std::ranges::none_of(zones,[&](const auto& z){return inside(z,spawn.position);})) {outside=spawn.position;found=true;}
    need(found,"Territories has no authored outside position");f.place(actor,outside);f.place(enemy,outside);f.step();f.step();
    const auto capture=[&](unsigned slot,unsigned other,bool retake) {
        const auto previous=f.journal.serial;f.place(other,outside);f.place(slot,zone.center);
        const auto budget=unsigned(half(f.memory,bank+0x112u)+(retake?half(f.memory,bank+0x110u):0))+60u;
        for(unsigned n=0;n<budget && word(f.memory,bank+0x70u+zone.index*4u)!=f.match.player(slot).source_datum;++n)f.step();
        need(word(f.memory,bank+0x70u+zone.index*4u)==f.match.player(slot).source_datum,"Ordinary region capture did not publish its source owner");
        const auto state=f.match.state();const auto& gained=f.journal.require(9,state.team_mode?4:1,previous);
        need(gained.data==zone.index,"Territory event lost the actual region index");
        require_text(row,retake?"territory-retaken":"territory-captured",assets,gained,state,slot);
        if(retake)require_text(row,"territory-lost",assets,f.journal.require(9,state.team_mode?5:2,previous),state,other);
    };
    capture(actor,enemy,false);capture(enemy,actor,true);
}
}
int main(int argc,char** argv)try {
    std::cout<<std::unitbuf;
    need(argc==4 || argc==5,"Usage: multiplayer_event_messages_tu15_tests MCC_MAP ORIGINAL_PRESETS REPORT [PRESET]");
    if(!std::filesystem::is_regular_file(argv[1]))return 77;
    Json fixture;std::ifstream(argv[2])>>fixture;
    Json report={{"passed",false},{"cases",Json::array()},{"scope",
        "All seven original families: actual MCC-backed canonical player/objective birth and ordinary full Match::step trigger packets, then the production localized response consumer. Initial/goal/zone positions are declared B95D0 inputs. Slayer's ordinary delivered aim/fire controls must produce a fatal source hit, actual attribution, complete15F8B0 and both recipient queues. Additional suicide/relationship and Juggernaut cases declare credited-death ABI inputs before original physical cleanup. No packet, damage, score, timer, role, capture, admission or message result is supplied. This is bounded native producer/text integration; other weapons, played approach routes, all presets/locales and installed visual/audio proof remain separate."}};
    const auto save=[&]{std::ofstream out(argv[3]);out<<report.dump(2)<<'\n';need(bool(out),"Cannot retain event-message integration report");};
    const std::map<unsigned,std::string> chosen{{1,"Multi Flag CTF"},{2,"Slayer"},{3,"Oddball"},{4,"King"},{7,"3 on 1"},{8,"3 Plots"},{9,"Multi Bomb"}};
    unsigned count{},failures{};
    for(const auto& preset:fixture.at("variants")) {
        const auto type=preset.at("source_type").get<unsigned>();
        const auto name=preset.at("name").get<std::string>();
        if(!chosen.contains(type) || chosen.at(type)!=name || (argc==5 && name!=argv[4]))continue;
        Json row={{"name",name},{"source_type",type},{"passed",false},{"stage","construct"},{"messages",Json::object()}};
        try {
            Files files(count++);MultiplayerOptions options;options.map=argv[1];options.local_players=(type==7 || type==9)?4:2;
            options.source_object_world=true;options.source_object_savegame_directory=files.path;options.variant=variant(preset);
            if(source_game_variant_get(source_game_variant(options.variant),91))for(unsigned n=0;n<options.local_players;++n)options.selected_teams.push_back(n%2);
            MultiplayerPreparedWorld prepared(options);const auto participants=prepared.local_slots;
            prepared.bind_source_local_views(std::span(participants).first(1));
            Flow f(prepared,participants);const auto assets=load_source_event_message_assets_tu15(*prepared.cache);
            row["loaded_localized_strings"]=assets.strings.size();need(!assets.strings.empty(),"MCC localized event text range did not load");
            for(unsigned n=0;n<4;++n)f.step();
            row["participants"]=Json::array();
            for(const auto slot:f.participants)row["participants"].push_back({{"slot",slot},
                {"source_player",f.match.player(slot).source_datum},{"membership",f.match.player(slot).source_membership},
                {"unit",f.match.player(slot).source_current_object},{"local_view",std::ranges::find(prepared.local_slots,slot)!=prepared.local_slots.end()}});
            const unsigned category=type==1?3:type==2?2:type==3?4:type==4?6:type==7?8:type==8?9:10;
            const auto state=f.match.state();
            const auto activation=std::ranges::find_if(f.journal.events,[&](const auto& event){return event.words[0]==category && event.words[1]==0;});
            need(activation!=f.journal.events.end(),"Ordinary original mode activation did not reach its source event journal");
            const auto recipient=activation->words[2]==null_datum?f.participants[0]:activation->words[2]&0xffffu;
            require_text(row,"mode-started",assets,*activation,state,recipient);
            row["stage"]="mode-specific-ordinary-transition";
            if(type==1 || type==3 || type==9)objective_flow(f,type,assets,row);
            if(type==4 || type==8)zone_flow(f,type,assets,row);
            std::unique_ptr<NetworkProbe> network;
            if(type==1){network=std::make_unique<NetworkProbe>(f,assets,options.variant);network->admit(row);}
            if(type==2 || type==7) {
                unsigned killer=f.participants[0],victim=f.participants[1];
                if(type==7) {
                    const auto mask=Bytes(f.memory.view(word(f.memory,0x4f63b8u)+0xfcu,2)).u16(0);
                    need(std::popcount(mask)==1,"Actual Juggernaut role has no unique holder");victim=std::countr_zero(mask);
                    for(const auto slot:f.participants)if(slot!=victim && f.match.player(slot).source_current_object!=null_datum){killer=slot;break;}
                }
                const auto before=f.journal.serial;
                if(type==2)ordinary_fatal_hit(f,killer,victim,row);else f.death(killer,victim);
                const auto after=f.match.state();
                const auto& killed=f.journal.require(0,0,before,f.match.player(killer).source_datum);
                require_text(row,"killer",assets,killed,after,killer,2);require_text(row,"dead-victim",assets,killed,after,victim,2);
                need(after.players[victim].dead,"Message victim was not in the genuine dead-player snapshot");
                if(type==7) {
                    const auto& assigned=f.journal.require(8,1,before,f.match.player(killer).source_datum);
                    require_text(row,"new-juggernaut",assets,assigned,after,killer);
                    need(output(assets,assigned,after,killer,2).empty(),"Split Juggernaut reassignment ignored its source text suppression");
                }else {
                    f.respawn(victim);const auto suicide_before=f.journal.serial;f.death(victim,victim);
                    require_text(row,"suicide",assets,f.journal.require(0,1,suicide_before),f.match.state(),victim);
                    f.respawn(victim);const auto betrayal_before=f.journal.serial;f.death(killer,victim,1);
                    require_text(row,"declared-relationship-kill",assets,f.journal.require(0,2,betrayal_before),f.match.state(),killer);
                }
            }
            row["stage"]="ordinary-match-over-request";const auto before_end=f.journal.serial;
            f.world.call_actual(0x13b4a0u);for(unsigned n=0;n<8 && !f.match.state().finished;++n)f.step();
            need(f.match.state().finished,"Original match-over request did not complete its normal frame");
            require_text(row,"game-over",assets,f.journal.require(0,19,before_end),f.match.state(),f.participants[0]);
            if(network)network->finish(row);
            row["source_packets"]=f.journal.events.size();row["final_tick"]=f.match.state().tick;row["passed"]=true;row["stage"]="complete";
        }catch(const std::exception& error){row["error"]=error.what();++failures;}
        std::cout<<row.dump()<<'\n';report["cases"].push_back(std::move(row));save();
    }
    need(count>0 && (argc==5 || count==7),"Seven-family event-message cohort was not selected completely");
    report["passed"]=failures==0;save();return failures?1:0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
