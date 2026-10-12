#include "halo2/hud.hpp"
#include <algorithm>
#include <bit>
#include <iostream>

using namespace halo2;
namespace {
void require(bool b,const std::string &m){if(!b)throw FormatError(m);}
bool equal(const std::vector<Vertex> &a,const std::vector<Vertex> &b) {
  if(a.size()!=b.size())return false;
  for(unsigned n=0;n<a.size();++n)
    if(a[n].uv.x!=b[n].uv.x || a[n].uv.y!=b[n].uv.y ||
       a[n].position.x!=b[n].position.x || a[n].position.y!=b[n].position.y)return false;
  return true;
}
}
int main(int argc,char **argv){try{
  require(argc==2 || argc==3,"Supply original maps and optional Vista artwork");
  const auto maps=std::filesystem::path(argv[1]);const Cache cache(maps/"lockout.map");
  auto content=cache.load_content(0,true);
  // Explicit clear-world/contact fixture, shared with the independently
  // verified live SMG damage test. The real match, firing, magazine, vitality,
  // state export and HUD all run unchanged, including actual source assets.
  content.collision.clear();
  auto rules=slayer_rules({"HUD gameplay",0,{{89,5}}});
  rules.weapon=cache.slayer_starting_weapon(5);rules.time_ticks=rules.score_limit=0;
  rules.random_seed=0x12345678;
  MultiplayerMatch match(content,0,cache.spawns(),rules);
  match.join("Shooter");match.join("Target");
  // Successful spawn now retains the original player+194 thirty-tick
  // damage countdown. This ordinary one-hit fixture starts after it expires;
  // shooting a freshly spawned target correctly applies the source0.1 scale.
  std::array<Controls,multiplayer_capacity> warmup{};
  for(unsigned n=0;n<30;++n)match.step(warmup);
  require(match.player(1).source_field_194==0,"Original spawn damage countdown did not expire");
  auto &shooter=const_cast<Unit&>(match.unit(0)),&target=const_cast<Unit&>(match.unit(1));
  shooter.body.position={0,0,0};shooter.yaw=0;target.body.position={2,0,0};target.yaw=0;
  shooter.body.velocity={};target.body.velocity={};
  target.vitality.shield_stun=target.vitality.body_stun=32767;
  auto imported=cache.load_hud(content.player.datum,rules.weapon);
  // SMG loaded rounds drive the bitmap meter; its number widget displays
  // reserve rounds. Retain the original masks, functions and formatting.
  auto numbers=imported;
  std::erase_if(numbers.widgets,[](const auto&w){return !w.is_text || !(w.flags&1) || w.weapon_type!=1;});
  require(!numbers.widgets.empty(),"No source ammo-number widgets");
  Mesh mesh;const auto artwork=argc==3?std::filesystem::path(argv[2]):std::filesystem::path{};
  MultiplayerHud hud(std::move(numbers),maps/"fonts",mesh,2,"Slayer",{},artwork);
  auto state=match.state();hud.animate(state,0,0,0,0,0);hud.animate(state,1,1,0,0,0);
  const auto initial=hud.draw(state,0,0,2,640,480,0,0);
  const auto other_initial=hud.draw(state,1,1,2,640,480,0,0);
  require(!initial.empty() && !other_initial.empty(),"Source ammo counters did not draw");
  require(state.players[0].loaded==60 && state.players[0].reserve==120,"Authored initial SMG ammo");
  auto meter_content=imported;
  std::erase_if(meter_content.widgets,[](const auto &w){return w.name!="ammo_meter_right";});
  require(meter_content.widgets.size()==1,"Missing original loaded-round meter");
  Mesh meter_mesh;MultiplayerHud meter(std::move(meter_content),maps/"fonts",meter_mesh,2,"Slayer",{},artwork);
  meter.animate(state,0,0,0,0,0);meter.animate(state,1,1,0,0,0);
  const auto meter_before=meter_mesh.materials;
  std::array<Controls,multiplayer_capacity> controls{};
  for(unsigned n=0;n<64 && !match.simulation().hits();++n){
    controls[0].fire=match.simulation().shots()==0;match.step(controls);
  }
  state=match.state();
  require(match.simulation().shots()==1 && match.simulation().hits()==1,"Actual one-shot source fixture failed");
  require(state.players[0].loaded==59 && state.players[0].reserve==120,"Shot did not reach HUD snapshot ammo");
  require(std::bit_cast<std::uint32_t>(state.players[1].shield)==0x3f6ea0eau &&
          state.players[1].health==1,"Shot did not reach HUD snapshot vitality");
  hud.animate(state,0,0,0,0,state.tick/30.0);hud.animate(state,1,1,0,0,state.tick/30.0);
  require(equal(initial,hud.draw(state,0,0,2,640,480,0,0)),"Firing changed the source reserve counter without consuming reserve rounds");
  require(equal(other_initial,hud.draw(state,1,1,2,640,480,0,0)),"Other viewport inherited shooter's ammo glyphs");
  meter.animate(state,0,0,0,0,0);meter.animate(state,1,1,0,0,0);
  const auto meter_vertices=meter.draw(state,0,0,2,640,480,0,0);
  require(!meter_vertices.empty(),"Loaded-round source meter vanished");
  const auto meter_material=meter_vertices.front().material;
  require(meter_before[meter_material].forward_program.words!=meter_mesh.materials[meter_material].forward_program.words,
      "Actual firing did not bind the loaded-round bitmap meter");
  const auto other_meter=meter.draw(state,1,1,2,640,480,0,0);
  require(!other_meter.empty() && meter_before[other_meter.front().material].forward_program.words==
      meter_mesh.materials[other_meter.front().material].forward_program.words,
      "Other viewport inherited shooter's loaded-round meter");
  controls={};
  // A reload pressed during the source firing state is correctly rejected.
  // Release the trigger and wait for the genuine admitted state first.
  for(unsigned n=0;n<64 && !smg_reload_admitted(match.unit(0).weapon.smg);++n)match.step(controls);
  require(smg_reload_admitted(match.unit(0).weapon.smg),"Source firing action never admitted reload");
  controls[0].reload=true;match.step(controls);controls[0].reload=false;
  for(unsigned n=0;n<120 && match.unit(0).weapon.loaded!=60;++n)match.step(controls);
  state=match.state();
  require(state.players[0].loaded==60 && state.players[0].reserve==119,"Original reload transfer did not reach HUD: "+
      std::to_string(state.players[0].loaded)+"/"+std::to_string(state.players[0].reserve)+
      " dead="+std::to_string(state.players[0].dead));
  hud.animate(state,0,0,0,0,state.tick/30.0);
  require(!equal(initial,hud.draw(state,0,0,2,640,480,0,0)),"Reload reserve change did not change source number glyphs");
  // Run the actual shield meter over the damage snapshot; its history must
  // retain the source half-second hold and then expose the reduced amount.
  std::erase_if(imported.widgets,[](const auto&w){return w.name!="shield_meter";});
  Mesh shields;MultiplayerHud shield(std::move(imported),maps/"fonts",shields,2,"Slayer",{},artwork);
  auto shield_state=state;shield_state.players[1].shield=1;shield.animate(shield_state,1,1,0,0,0);
  const auto before=shields.materials;
  shield_state.players[1].shield=std::bit_cast<float>(0x3f6ea0eau);++shield_state.tick;
  shield.animate(shield_state,1,1,0,0,shield_state.tick/30.0);
  require(!shield.draw(shield_state,1,1,2,640,480,0,0).empty(),"Damaged source shield did not draw");
  unsigned changed=0;for(unsigned n=0;n<before.size();++n)
    changed+=before[n].forward_program.words!=shields.materials[n].forward_program.words;
  require(changed>0,"Actual damage did not bind source shield shader inputs");
  // Continue genuine firing into shield depletion and body death, rather
  // than presenting a fabricated health value to the HUD.
  controls[0].fire=true;
  for(unsigned n=0;n<300 && !match.state().players[1].dead;++n)match.step(controls);
  state=match.state();
  require(state.players[1].dead && state.players[1].health<=0 && state.players[0].kills==1,
      "Actual SMG damage did not reach body death and attribution: health="+
      std::to_string(state.players[1].health)+" shield="+std::to_string(state.players[1].shield)+
      " dead="+std::to_string(state.players[1].dead)+" kills="+std::to_string(state.players[0].kills)+
      " shots="+std::to_string(match.simulation().shots())+" hits="+std::to_string(match.simulation().hits()));
  hud.animate(state,1,1,0,0,state.tick/30.0);
  require(hud.draw(state,1,1,2,640,480,0,0).empty(),"Dead player retained live ammo counters");
  shield.animate(state,1,1,0,0,state.tick/30.0);
  require(shield.draw(state,1,1,2,640,480,0,0).empty(),"Dead player retained live shield status");
  controls={};
  for(unsigned n=0;n<300 && match.state().players[1].dead;++n)match.step(controls);
  state=match.state();
  require(!state.players[1].dead && state.players[1].health==1 && state.players[1].shield==1 &&
      state.players[1].loaded==60 && state.players[1].reserve==120,"Actual respawn did not restore HUD state");
  hud.animate(state,1,1,0,0,state.tick/30.0);
  shield.animate(state,1,1,0,0,state.tick/30.0);
  require(!hud.draw(state,1,1,2,640,480,0,0).empty() &&
      !shield.draw(state,1,1,2,640,480,0,0).empty(),"Respawn did not restore source HUD widgets");
  std::cout<<"Production match -> snapshot -> HUD: actual shot/shield hit, reload, ammo glyphs, body death/respawn and independent local viewports PASS\n";
  return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
