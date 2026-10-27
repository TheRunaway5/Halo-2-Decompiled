#include "halo2/multiplayer_network.hpp"
#include <iostream>
using namespace halo2;
namespace {
void check(bool value,const char *message) {if(!value)throw FormatError(message);}
}
int main(int argc,char **argv) {
  try {
    if(argc!=2)throw FormatError("Supply actual local Lockout cache");
    if(!std::filesystem::exists(argv[1]))return 77;
    Cache cache(argv[1]);auto content=cache.load_content(0,true);content.kill_triggers.clear();
    auto rules=slayer_rules({std::string(32,'X'),0,{{0x59,5}}});rules.weapon=cache.slayer_starting_weapon(5);
    rules.score_limit=rules.time_ticks=0;rules.respawn_ticks=30;
    const auto definition=cache.load_first_person_binding(rules.weapon,2);
    MultiplayerMatch match(content,0,cache.spawns(),rules);LanHost host(match,0,0x169d8a);
    std::unique_ptr<LanClient> client;std::array<Controls,16> controls{};
    std::array<FirstPersonViewBinding,4> views;
    const auto exchange=[&] {
      client->send(std::array<Controls,4>{});host.receive(controls);host.broadcast();client->receive();
    };
    const auto connect=[&] {
      client=std::make_unique<LanClient>(host.join_address(),0x169d8a,4);
      for(unsigned i=0;i<64 && !client->ready();++i)exchange();
      check(client->ready() && client->slots().size()==4,"Four-view native UDP join");
      for(unsigned i=0;i<4;++i)check(client->slots()[i]==i,"Assigned view slot order");
    };
    const auto observe=[&] {
      for(unsigned i=0;i<4;++i) {
        const auto &p=client->state().players[i];const auto &authoritative=match.player(i);
        check(p.source_player==authoritative.source_datum && p.source_unit==authoritative.source_current_object &&
            p.source_weapon==(p.source_unit==null_datum?null_datum:match.unit(i).weapon.source_datum),"Full UDP identity round trip");
        observe_multiplayer_first_person(views[i],p,definition);
        check(views[i].unit==p.source_unit && views[i].hands[0].weapon==p.source_weapon,"Original full current binding");
        if(p.source_unit!=null_datum) {
          const auto &h=views[i].hands[0];check((h.flags&6)==6 && h.graph==definition.graph &&
              h.weapon_node_count==definition.weapon_nodes.size() && h.arms_node_count==definition.arms_nodes.size(),"Authored original node maps");
        }
      }
    };
    connect();observe();
    const auto first_player=match.player(0).source_datum,first_unit=match.unit(0).source_datum;
    const auto first_weapon=match.unit(0).weapon.source_datum;
    for(unsigned i=4;i<16;++i)check(match.join("Capacity")==i,"Full native lobby capacity");
    // Four local identities,16 connected players and32 variant bytes must fit
    // the1400-byte transport budget without dropping the authoritative snapshot.
    match.step(controls);exchange();check(client->state().tick==match.state().tick,"Maximum-size UDP snapshot");observe();
    for(const auto &p:client->state().players)check(p.connected,"Maximum-size connected slots");
    views[0].hands[0].frame=17;views[1].hands[0].frame=29;
    match.step(controls);exchange();observe();check(views[0].hands[0].frame==17 && views[1].hands[0].frame==29,"Unchanged identity retains independent channels");
    // Supply the already-proven dead-bit prefix boundary. Actual native cleanup
    // and source respawn schedule run; no retail live-contact claim is made.
    auto &victim=const_cast<Unit &>(match.unit(0));victim.vitality.dead=true;victim.last_damager=-2;
    match.step(controls);exchange();observe();check(views[0].unit==null_datum && views[0].hands[0].weapon==null_datum,"Death clears current remote binding");
    check(views[0].hands[0].frame==17 && views[1].hands[0].frame==29,"Source removal retains channel without affecting other views");
    for(unsigned t=0;t<200 && match.player(0).source_current_object==null_datum;++t)match.step(controls);
    exchange();observe();check(match.unit(0).source_datum!=first_unit && match.unit(0).weapon.source_datum!=first_weapon &&
        match.player(0).source_datum==first_player && views[0].hands[0].frame==0,"Respawn rebuilds new full unit/weapon with same player salt");
    check(match.unit_from_datum(first_unit) && match.unit_from_datum(first_unit)->source_player==null_datum,"Retained source corpse lookup");
    client.reset();host.receive(controls);
    for(unsigned i=0;i<4;++i) {auto p=match.state().players[i];observe_multiplayer_first_person(views[i],p,definition);check(views[i].unit==null_datum,"Disconnected view release");}
    connect();observe();check(match.player(0).source_datum!=first_player && !match.player_from_datum(first_player),"Reconnect invalidates departed full player salt");
    auto malformed=client->state().players[0];malformed.source_unit=null_datum;bool rejected=false;
    try {observe_multiplayer_first_person(views[0],malformed,definition);}catch(const FormatError &) {rejected=true;}
    check(rejected,"Inconsistent current unit/weapon must reject");
    std::cout<<"Four-view source bindings passed actual UDP join/death/respawn/leave/reconnect, independent retained channels and maximum1400-byte snapshot\n";
  }catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}
