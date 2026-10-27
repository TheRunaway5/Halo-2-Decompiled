#include "halo2/multiplayer_network.hpp"
#include <iostream>

using namespace halo2;
namespace {
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
}
int main(int argc,char **argv) {
  try {
    if(argc!=4)throw std::runtime_error("Supply original MAP SHARED_MAPS UPDATE_XBE");
    if(!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[3]))return 77;
    const Cache cache(argv[1],argv[2],OriginalData::load_xbe(argv[3]));
    const auto content=cache.load_content(0,true);
    UiVariant variant{"Slayer",0,{}};variant.values.set(0x59,5);
    auto rules=slayer_rules(variant);rules.weapon=cache.slayer_starting_weapon(rules.starting_weapon);
    check(content.weapons.at(rules.weapon).smg.has_value(),"Actual stock SMG source manager required");
    MultiplayerMatch match(content,0,cache.spawns(),rules);
    match.join("Local Host");LanHost host(match,0,789);
    LanClient client(host.join_address(),789,1);
    std::array<Controls,16> controls{};std::array<Controls,1> local{};
    for(unsigned i=0;i<20 && !client.ready();++i) {
      client.send(local);host.receive(controls);host.broadcast();client.receive();
    }
    check(client.ready() && client.error().empty(),"Actual UDP scope client joined");
    const unsigned id=client.slots()[0];
    const auto step=[&] {
      client.send(local);host.receive(controls);match.step(controls);host.broadcast();client.receive();
      check(client.state().tick==match.state().tick,"Complete authoritative scope snapshot reaches UDP client");
      const auto &a=client.state().players[id];const auto &u=match.unit(id);
      check(a.source_zoom.current==u.source_zoom.current && a.source_zoom.requested==u.source_zoom.requested &&
          a.source_zoom.motion==u.source_zoom.motion && a.zoom_reset_serial==u.zoom_reset_serial &&
          a.source_action_word==u.source_action_word && a.source_zoom_magazine_mode==u.weapon.smg.magazine_mode &&
          a.source_hud_zoom_mask==match.state().players[id].source_hud_zoom_mask,
          "Actual source scope/action/magazine fields survive native UDP transport");
    };
    local[0].yaw=client.state().players[id].yaw;
    local[0].zoom=0;step();
    check(match.unit(id).source_zoom.current==0 && match.unit(0).source_zoom.current==-1,
        "Remote source scope is independent from local host");
    const auto loaded=match.unit(id).weapon.loaded;
    local[0].fire=true;
    for(unsigned i=0;i<5 && match.unit(id).weapon.loaded==loaded;++i)step();
    check(match.unit(id).weapon.loaded<loaded && match.unit(id).source_zoom.current==-1 &&
        match.unit(id).zoom_reset_serial>0,"Actual remote prepared shot resets binocular zoom");
    // Host repeats its cached numeric request until a new native input arrives.
    // A source reset must stay reset while that request acknowledges old state.
    for(unsigned i=0;i<8;++i) {
      host.receive(controls);match.step(controls);host.broadcast();client.receive();
      check(match.unit(id).source_zoom.current==-1 && match.unit(id).source_zoom.requested==-1,
          "Cached pre-reset LAN input reintroduced binocular zoom");
    }
    local[0].fire=false;local[0].zoom=-1;
    local[0].zoom_reset_serial=client.state().players[id].zoom_reset_serial;step();
    local[0].zoom=0;step();
    check(match.unit(id).source_zoom.current==0,"Acknowledged remote RMB request can scope again");
    local[0].reload=true;step();
    check(match.unit(id).source_zoom.current==-1 && (match.unit(id).source_action_word&1u) &&
        match.unit(id).weapon.smg.magazine_mode==1,"Real remote manual reload resets and blocks scope");
    local[0].reload=false;local[0].zoom_reset_serial=client.state().players[id].zoom_reset_serial;
    step();check(match.unit(id).source_zoom.current==-1,"Real active magazine denies remote numeric scope request");
    local[0].zoom=-1;bool observed_completion{},observed_release{};
    for(unsigned i=0;i<100;++i) {
      const auto before=match.unit(id).weapon.smg.magazine_mode;
      const auto action=match.unit(id).source_action_word;
      step();const auto &u=match.unit(id);
      if(before==6 && u.weapon.smg.magazine_mode==0) {
        check((u.source_action_word&1u)!=0,"Biped action cleared prematurely during its child weapon update");
        observed_completion=true;
      }
      if(before==0 && (action&1u) && !(u.source_action_word&1u))observed_release=true;
      if(observed_completion && observed_release)break;
    }
    check(observed_completion && observed_release,"Original child/biped reload release ordering was not observed");
    local[0].zoom=0;step();check(match.unit(id).source_zoom.current==0,"Remote scope admitted after actual action release");
    std::cout<<"PASS actual original Lockout/1.5 SMG over native UDP: independent zoom, prepared-shot reset, stale-request protection, reload admission and biped-before-child release order\n";
    std::cout<<"LIMIT supplied native numeric controls, not a second-machine or rendered LAN keyboard test; no Xbox wire-protocol parity claim\n";
  }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
