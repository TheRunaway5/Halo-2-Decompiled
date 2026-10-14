#include "halo2/hud.hpp"
#include "halo2/native_weapon_inventory.hpp"
#include <algorithm>
#include <iostream>
using namespace halo2;
namespace {
void require(bool value,const std::string &message){if(!value)throw FormatError(message);}
bool same_glyphs(const std::vector<Vertex> &a,const std::vector<Vertex> &b) {
  if(a.size()!=b.size())return false;
  for(unsigned i=0;i<a.size();++i)if(a[i].uv.x!=b[i].uv.x || a[i].uv.y!=b[i].uv.y)return false;
  return true;
}
}
int main(int argc,char **argv)try {
  require(argc==4,"Supply MAP SHARED XBE");
  Cache cache(argv[1],argv[2],OriginalData::load_xbe(argv[3]));
  const auto authored=cache.load_content(0,true);const auto spawns=cache.spawns();
  const auto smg=std::find_if(authored.weapons.begin(),authored.weapons.end(),[](const auto &w){return w.second.smg.has_value();});
  require(smg!=authored.weapons.end(),"Missing actual SMG definition");
  unsigned total_ticks=0;
  for(unsigned representation:{2u,3u}) {
    auto level=authored;level.netgame_equipment.clear();
    const auto spawn=spawns.at(0);NetgameEquipment pickup;
    pickup.modes={2,0,0,0};pickup.respawn_seconds=1000;pickup.position=spawn.position+Vec3{.2f,0,.4f};
    pickup.items.push_back({1,0x77656170u,smg->first,0});level.netgame_equipment.push_back(pickup);
    auto rules=slayer_rules({"Weapon HUD",0,{{0x59,5}}});rules.weapon=smg->first;rules.score_limit=rules.time_ticks=0;
    MultiplayerMatch match(level,0,spawns,rules);match.configure_weapon_pickups();
    std::optional<SourcePlayerConfiguration> profile;
    if(representation==3){profile=source_default_player_configuration();(*profile)[0x44]=1;}
    const auto player=match.join("Shared reserve HUD",-1,profile);
    const auto binding=cache.load_first_person_binding(smg->first,int(representation));
    match.configure_replicated_first_person(binding);
    const std::array<unsigned,1> views{player};const std::array definitions{binding};
    match.configure_local_views(views,definitions);
    std::array<Controls,multiplayer_capacity> controls{};
    auto step=[&](){match.step(controls);++total_ticks;};
    match.debug_request({DebugGameRequest::Teleport,player},&spawn);step();
    match.debug_request({DebugGameRequest::Teleport,player},&spawn);
    controls[player].dual_wield=true;controls[player].dual_wield_milliseconds=233;step();
    controls={};for(unsigned n=0;n<120;++n)step();
    for(unsigned hand=0;hand<2;++hand) {
      const auto *weapon=native_hand_weapon(match.unit(player),hand);
      require(weapon && weapon->definition==smg->first && weapon->loaded==60 && weapon->reserve==120,
          "Actual dual pickup did not preserve authored stores");
    }
    const auto content=cache.load_hud(authored.player_representations.at(representation).biped.datum,smg->first);
    require(content.weapon_magazine_count==1 && content.clip_capacity==60 && content.reserve_capacity==180,
        "Source HUD lost authored magazine dimensions");
    auto numbers=content;
    std::erase_if(numbers.widgets,[](const auto &w){return !w.is_text || !(w.flags&1) || w.weapon_type!=1 ||
        std::find(w.inputs.begin(),w.inputs.end(),51)==w.inputs.end();});
    require(!numbers.widgets.empty(),"Missing actual reserve number widgets");
    Mesh mesh;const auto fonts=std::filesystem::path(argv[2])/"fonts";
    MultiplayerHud primary(numbers,fonts,mesh,1,"Slayer",{}, {},0);
    MultiplayerHud secondary(numbers,fonts,mesh,1,"Slayer",{}, {},1);
    auto state=match.state();
    for(unsigned hand=0;hand<2;++hand) {
      const auto projected=multiplayer_hud_weapon_state(state.players[player],content,hand);
      require(projected.loaded==60 && projected.reserve==240 && projected.capacity==60 && projected.maximum_reserve==180,
          "Initial HUD did not sum the actual two reserve stores");
    }
    primary.animate(state,player,0,0,0,state.tick/30.0);secondary.animate(state,player,0,0,0,state.tick/30.0);
    const auto initial=primary.draw(state,player,0,1,640,480,0,0);
    const auto initial_other=secondary.draw(state,player,0,1,640,480,0,0);
    require(!initial.empty() && same_glyphs(initial,initial_other),"Dual shared counters did not render equal240 glyphs");
    for(unsigned hand=0;hand<2;++hand) {
      const int target=hand?56:57;
      for(unsigned n=0;n<180 && native_hand_weapon(match.unit(player),hand)->loaded>target;++n) {
        controls[player].fire=hand==0;controls[player].fire_secondary=hand==1;step();
      }
      controls={};
      require(native_hand_weapon(match.unit(player),hand)->loaded==target,"Actual firing did not consume the intended rounds");
      for(unsigned n=0;n<90;++n)step();
    }
    state=match.state();
    require(state.players[player].loaded==57 && state.players[player].secondary_weapon.loaded==56,
        "Actual shot counts did not reach both snapshot stores");
    for(unsigned hand=0;hand<2;++hand)require(multiplayer_hud_weapon_state(state.players[player],content,hand).reserve==240,
        "Firing changed shared reserve without a magazine transfer");
    require(smg_reload_admitted(native_hand_weapon(match.unit(player),0)->smg) &&
        smg_reload_admitted(native_hand_weapon(match.unit(player),1)->smg),"Real dual action did not admit reload");
    controls[player].reload=true;step();controls={};
    for(unsigned n=0;n<180;++n)step();
    state=match.state();const auto &p=state.players[player];
    require(p.loaded==60 && p.reserve==117 && p.secondary_weapon.loaded==60 && p.secondary_weapon.reserve==116,
        "Reload did not retain each weapon's individual reserve store");
    require(p.ammunition_available==293 && p.secondary_weapon.ammunition_available==293,
        "Shared loaded-plus-reserve did not reach both real snapshots");
    for(unsigned hand=0;hand<2;++hand) {
      const auto projected=multiplayer_hud_weapon_state(p,content,hand);
      require(projected.loaded==60 && projected.reserve==233 && weapon_hud_input(&projected,51,0)==233 &&
          weapon_hud_input(&projected,54,0)==1,"HUD did not consume source shared reserve and loaded fraction");
    }
    primary.animate(state,player,0,0,0,state.tick/30.0);secondary.animate(state,player,0,0,0,state.tick/30.0);
    const auto after=primary.draw(state,player,0,1,640,480,0,0);
    const auto other_after=secondary.draw(state,player,0,1,640,480,0,0);
    require(!after.empty() && !same_glyphs(initial,after) && same_glyphs(after,other_after),
        "Both actual reserve number widgets did not change240 to233 together");
  }
  unsigned battery_cases=0;
  for(unsigned representation:{2u,3u})for(const std::string name:{"plasma_rifle","brute_plasma_rifle","beam_rifle"}) {
    const auto weapon=std::find_if(authored.weapons.begin(),authored.weapons.end(),[&](const auto &w){return w.second.name.ends_with("\\"+name);});
    require(weapon!=authored.weapons.end() && weapon->second.energy,"Missing actual battery weapon: "+name);
    auto level=authored;level.netgame_equipment.clear();const auto spawn=spawns.at(0);NetgameEquipment pickup;
    pickup.modes={2,0,0,0};pickup.position=spawn.position+Vec3{.2f,0,.4f};
    pickup.items.push_back({1,0x77656170u,weapon->first,0});level.netgame_equipment.push_back(pickup);
    auto rules=slayer_rules({"Battery HUD",0,{{0x59,5}}});rules.weapon=smg->first;rules.score_limit=rules.time_ticks=0;
    MultiplayerMatch match(level,0,spawns,rules);match.configure_weapon_pickups();
    std::optional<SourcePlayerConfiguration> profile;
    if(representation==3){profile=source_default_player_configuration();(*profile)[0x44]=1;}
    const auto player=match.join("Actual battery HUD",-1,profile);
    std::array<Controls,multiplayer_capacity> controls{};auto step=[&](){match.step(controls);++total_ticks;};
    match.debug_request({DebugGameRequest::Teleport,player},&spawn);step();
    match.debug_request({DebugGameRequest::Teleport,player},&spawn);
    controls[player].pickup=true;controls[player].pickup_milliseconds=233;step();controls={};
    require(match.unit(player).weapon.definition==weapon->first,"Actual battery pickup failed");
    for(unsigned n=0;n<120;++n)step();
    const auto content=cache.load_hud(authored.player_representations.at(representation).biped.datum,weapon->first);
    auto state=match.state();auto projected=multiplayer_hud_weapon_state(state.players[player],content,0);
    require(content.clip_capacity==0 && content.reserve_capacity==0 && projected.loaded==0 && projected.reserve==0 &&
        !(weapon_hud_mask(&projected,1)&0x80),"Fresh battery state "+name+" mags="+std::to_string(content.weapon_magazine_count)+" loaded="+std::to_string(projected.loaded)+" reserve="+std::to_string(projected.reserve)+" age="+std::to_string(projected.age)+" mask="+std::to_string(weapon_hud_mask(&projected,1)));
    for(unsigned n=0;n<100 && match.unit(player).weapon.shader.power_fraction==0;++n){controls[player].fire=true;step();}
    state=match.state();projected=multiplayer_hud_weapon_state(state.players[player],content,0);
    require(projected.age>0 && projected.age<1 && projected.age==state.players[player].weapon_shader.power_fraction &&
        projected.heat==state.players[player].weapon_shader.heat && weapon_hud_input(&projected,50,0)<100 &&
        weapon_hud_input(&projected,56,0)<1 && !(weapon_hud_mask(&projected,1)&0x80),
        "Actual battery consumption did not reach HUD age,heat,percentage and availability");
    ++battery_cases;
  }
  std::cout<<"PASS both authored bipeds: actual dual pickup,3/4 shots,independent reload stores,shared240->233 snapshots and source reserve glyphs; "
      <<total_ticks<<" production ticks; "<<battery_cases<<" actual battery pickup/fire HUD cases\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
