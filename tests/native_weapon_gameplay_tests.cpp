#include "halo2/multiplayer.hpp"
#include "halo2/native_weapon_inventory.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/multiplayer_kbm.hpp"
#include <thread>
#include <cmath>
#include <bit>
#include <iostream>
using namespace halo2;
namespace {
void check(bool ok,const char *why){if(!ok)throw FormatError(why);}
LevelContent arena(bool dual) {
  LevelContent c;c.player.datum=1;c.player.height=1;c.player.radius=.2f;c.player.camera_height=.8f;
  c.pickup_hold_seconds=.2333f;
  c.player.bounding_radius=.44f;c.player.crouch_height=.6f;c.player.crouch_camera_height=.5f;c.player.vitality.health=100;
  c.multiplayer_starting_equipment_flags=0;
  WeaponDefinition w;w.datum=3;w.capacity=6;w.initial_rounds=18;w.reserve=12;
  w.reload_rounds=6;w.reload_time=.2f;w.rounds_per_shot=1;w.rounds_per_second=30;w.burst=1;
  w.dual_wieldable=true;w.first_person_models.push_back({8,9});c.weapons.emplace(3,w);
  w.datum=4;w.dual_wieldable=false;c.weapons.emplace(4,w);
  NetgameEquipment row;row.modes={2,0,0,0};row.respawn_seconds=2;row.position={.5f,0,.4f};
  row.items.push_back({1,0x77656170u,dual?3u:4u,0});c.netgame_equipment.push_back(row);return c;
}
SlayerRules rules(){SlayerRules r;r.weapon=3;r.score_limit=0;r.time_ticks=0;return r;}
std::vector<Spawn> starts(){Spawn s;s.player_type=0;return {s};}
void pickup_hold_boundary() {
  auto c=arena(true);auto spawns=starts();MultiplayerMatch m(c,0,spawns,rules());m.configure_weapon_pickups();
  const auto id=m.join("Held pickup owner");std::array<Controls,16> input{};for(auto &v:input)v.pickup_milliseconds=v.dual_wield_milliseconds=233;m.step(input);
  input[id].dual_wield=true;input[id].dual_wield_milliseconds=232;m.step(input);
  check(!native_hand_weapon(m.unit(id),1),"Original pickup must reject 232ms hold");
  input[id].dual_wield_milliseconds=233;m.step(input);
  check(native_hand_weapon(m.unit(id),1),"Original pickup must admit 233ms hold");
  check(m.unit(id).source_player_actions.flags&0x20u,"Successful secondary pickup must consume until release");
  // Drop while still held: the same hold must not immediately reacquire it.
  input[id].grenade=true;m.step(input);input[id].grenade=false;m.step(input);
  check(!native_hand_weapon(m.unit(id),1),"Consumed hold reacquired a dropped offhand");
  input[id].dual_wield=false;input[id].dual_wield_milliseconds=0;m.step(input);
  check(!(m.unit(id).source_player_actions.flags&0x20u),"Release must clear pickup consumption");
}
void zero_rate_latched_fire() {
  auto c=arena(false);auto &w=c.weapons.at(3);
  // Actual Lockout shotgun fields, independently admitted by unchanged
  // 1062B0/108010 in shotgun-firing-gate-20261011.json. This is a native
  // integration regression, not a complete original shotgun update trace.
  w.rounds_per_second=0;w.fire_recovery=1;w.trigger_behavior=1;
  auto spawns=starts();MultiplayerMatch m(c,0,spawns,rules());
  const auto id=m.join("Latched zero-rate owner");std::array<Controls,16> input{};
  m.step(input);input[id].fire=true;m.step(input);
  check(m.unit(id).weapon.loaded==5,"Zero authored firing rate disabled an admitted shotgun shot");
  for(unsigned n=0;n<65;++n)m.step(input);
  check(m.unit(id).weapon.loaded==5,"Latched shotgun repeated while trigger remained held");
  input[id].fire=false;m.step(input);input[id].fire=true;m.step(input);
  check(m.unit(id).weapon.loaded==4,"Latched shotgun did not rearm after trigger release");
}
void dual_gameplay() {
  auto c=arena(true);auto spawns=starts();MultiplayerMatch m(c,0,spawns,rules());m.configure_weapon_pickups();
  const auto id=m.join("Dual owner");
  // A synthetic residency definition isolates publication order: the actual
  // pickup still owns transfer/ready, and binding must precede its activity.
  FirstPersonBindingDefinition fp;fp.representation=2;fp.weapon_definition=3;
  fp.arms_model=7;fp.weapon_model=8;fp.graph=9;fp.record_present=true;fp.idle_index=0;
  fp.arms_nodes=fp.weapon_nodes=fp.graph_nodes={17};
  fp.clock_clips={{0x0400000c,0,10,0,{}},{0x05000024,1,8,0,{}}};fp.dual_clock_clips=fp.clock_clips;
  const std::array<unsigned,1> views{id};m.configure_local_views(views,fp);
  std::array<Controls,16> input{};for(auto &v:input)v.pickup_milliseconds=v.dual_wield_milliseconds=233;m.step(input);
  check(m.state().world_weapons.size()==1,"Map weapon did not spawn");
  const auto pickup=m.state().world_weapons[0].handle,primary=m.unit(id).weapon.source_datum;
  input[id].dual_wield=true;m.step(input);input[id].dual_wield=false;
  const auto *offhand=native_hand_weapon(m.unit(id),1);
  check(offhand && offhand->source_datum==pickup && pickup!=primary,"Dual pickup lost item identity");
  check(m.first_person_view(0).hands[1].weapon==pickup && m.first_person_view(0).hands[1].activity==0x05000024,
      "Ready activity was lost before the newly picked hand acquired its binding");
  check(m.first_person_view(0).hands[0].activity==0x0400000c,
      "New offhand ready activity restarted the existing primary hand");
  check(m.state().world_weapons.empty(),"Picked weapon still rendered in the world");
  check(!m.weapon_pickups()[0].weapon,"World and inventory retained duplicate weapon clocks");
  input[id].fire=true;m.step(input);input[id].fire=false;
  check(m.unit(id).weapon.loaded==5 && native_hand_weapon(m.unit(id),1)->loaded==6,"Primary fire shared offhand ammo");
  input[id].fire_secondary=true;m.step(input);input[id].fire_secondary=false;
  check(m.unit(id).weapon.loaded==5 && native_hand_weapon(m.unit(id),1)->loaded==5,"Secondary fire did not own independent ammo");
  check(m.state().players[id].motion_sensor_control==0x20,
      "Actual offhand trigger did not publish original radar bit5 independently");
  input[id].reload=true;m.step(input);input[id].reload=false;
  check(m.unit(id).weapon.reload>0 && native_hand_weapon(m.unit(id),1)->reload>0,"Both hand reload requests were not delivered");
  for(unsigned tick=0;tick<10;++tick)m.step(input);
  check(m.unit(id).weapon.loaded==6 && native_hand_weapon(m.unit(id),1)->loaded==6,"Independent magazines did not reload");
  check(m.unit(id).weapon.reserve==11 && native_hand_weapon(m.unit(id),1)->reserve==11,"Reload reserve accounting differs");
  const auto state=m.state();check(state.players[id].secondary_weapon.datum==pickup &&
      state.players[id].secondary_weapon.loaded==6,"Secondary weapon was not published");
  input[id].grenade=true;m.step(input);input[id].grenade=false;
  check(!native_hand_weapon(m.unit(id),1) && m.state().world_weapons.size()==1,"Grenade did not release the offhand");
  check(!m.state().players[id].motion_sensor_control,
      "Grenade request was misrepresented as an offhand trigger on radar");
  check(m.state().world_weapons[0].handle==pickup && m.state().world_weapons[0].loaded==6 &&
      m.state().world_weapons[0].reserve==11,"Drop recreated the weapon or replenished ammunition");
  for(unsigned tick=0;tick<90;++tick)m.step(input);
  check(m.state().world_weapons.size()==2,"Picked row did not respawn independently of dropped item");
}
void pickup_switch() {
  auto c=arena(false);auto spawns=starts();MultiplayerMatch m(c,0,spawns,rules());m.configure_weapon_pickups();
  const auto id=m.join("Carry owner");std::array<Controls,16> input{};for(auto &v:input)v.pickup_milliseconds=v.dual_wield_milliseconds=233;m.step(input);
  const auto original=m.unit(id).weapon.source_datum,target=m.state().world_weapons[0].handle;
  input[id].dual_wield=true;m.step(input);input[id].dual_wield=false;
  check(!native_hand_weapon(m.unit(id),1),"Non-dual weapon admitted in secondary hand");
  input[id].pickup=true;m.step(input);input[id].pickup=false;
  check(m.unit(id).weapon.definition==4 && m.unit(id).weapon.source_datum==target,"Pickup did not equip the same world item");
  input[id].fire=true;m.step(input);input[id].fire=false;
  check(m.unit(id).weapon.loaded==5,"Picked weapon did not fire");
  input[id].switch_weapon=true;m.step(input);input[id].switch_weapon=false;
  check(m.unit(id).weapon.source_datum==original && m.unit(id).weapon.loaded==6,"Switch lost inactive weapon state");
  input[id].switch_weapon=true;m.step(input);
  check(m.unit(id).weapon.source_datum==target && m.unit(id).weapon.loaded==5,"Switch replenished ammunition or changed identity");
}
void mixed_dual_gameplay() {
  auto c=arena(true);auto &other=c.weapons.at(4);
  other.dual_wieldable=true;other.capacity=9;other.initial_rounds=27;other.reserve=18;
  c.netgame_equipment[0].items[0].definition=4;
  auto spawns=starts();MultiplayerMatch m(c,0,spawns,rules());m.configure_weapon_pickups();
  const auto id=m.join("Mixed dual owner");std::array<Controls,16> input{};for(auto &v:input)v.pickup_milliseconds=v.dual_wield_milliseconds=233;
  m.step(input);input[id].dual_wield=true;m.step(input);input[id].dual_wield=false;
  check(native_hand_weapon(m.unit(id),1) && native_hand_weapon(m.unit(id),1)->definition==4,
      "Different compatible weapon did not enter the secondary hand");
  input[id].fire=input[id].fire_secondary=true;m.step(input);
  input[id].fire=input[id].fire_secondary=false;
  check(m.unit(id).weapon.loaded==5 && native_hand_weapon(m.unit(id),1)->loaded==8,
      "Mixed hands used one weapon's magazine definition");
  input[id].reload=true;m.step(input);input[id].reload=false;
  for(unsigned tick=0;tick<10;++tick)m.step(input);
  check(m.unit(id).weapon.loaded==6 && native_hand_weapon(m.unit(id),1)->loaded==9 &&
      m.unit(id).weapon.reserve==11 && native_hand_weapon(m.unit(id),1)->reserve==17,
      "Mixed reload did not retain independent capacities and reserve ammunition");
  const auto state=m.state();
  check(state.players[id].weapon==3 && state.players[id].secondary_weapon.definition==4,
      "Mixed weapon definitions were not published independently");
}
void pickup_bounds() {
  for(int side:{-1,0,1}) {
    auto c=arena(true);c.player.bounding_offset={};c.netgame_equipment[0].position={.44f+.3f,0,0};
    if(side<0)c.netgame_equipment[0].position.x=std::nextafter(c.netgame_equipment[0].position.x,0.f);
    if(side>0)c.netgame_equipment[0].position.x=std::nextafter(c.netgame_equipment[0].position.x,1.f);
    auto spawns=starts();spawns[0].facing=3.14159265f;
    MultiplayerMatch m(c,0,spawns,rules());m.configure_weapon_pickups();const auto id=m.join("Bounds owner");
    std::array<Controls,16> input{};for(auto &v:input)v.pickup_milliseconds=v.dual_wield_milliseconds=233;input[id].yaw=spawns[0].facing;input[id].dual_wield=true;m.step(input);
    check(bool(native_hand_weapon(m.unit(id),1))==(side<0),"Pickup must use strict source bound distance and ignore view direction");
  }
}
// Declared native arena controls for the original152080 restrictions. These
// verify live routing; original executable comparisons use separate raw banks.
void player_carry_admission() {
  for(const auto flags:std::array<std::array<std::uint32_t,2>,3>{{{0,0},{16,0},{0,16}}}) {
    auto c=arena(false);c.weapons.at(3).inventory_flags=flags[0];
    auto third=c.weapons.at(4);third.datum=5;third.inventory_flags=flags[1];c.weapons.emplace(5,third);
    auto row=c.netgame_equipment[0];row.items[0].definition=5;c.netgame_equipment.push_back(row);
    auto spawns=starts();MultiplayerMatch m(c,0,spawns,rules());m.configure_weapon_pickups();
    const auto id=m.join("Player carry restriction owner");std::array<Controls,16> input{};
    input[id].pickup_milliseconds=233;m.step(input);m.step(input); // Source row1 admits on tick1.
    check(m.state().world_weapons.size()==2,"Carry restriction targets did not spawn");
    const auto first=m.state().world_weapons[0].handle,second=m.state().world_weapons[1].handle;
    input[id].pickup=true;m.step(input);check(m.unit(id).weapon.source_datum==first,"First carry restriction target was not equipped");
    input[id].pickup=false;m.step(input);input[id].pickup=true;m.step(input);
    check(m.unit(id).weapon.source_datum==second,"Second carry restriction target was not equipped");
    const auto &inventory=m.source_inventory(id);unsigned occupied=0;bool first_carried=false;
    for(auto datum:inventory.slots){occupied+=datum!=null_datum;first_carried|=datum==first;}
    const bool exempt=flags[0]||flags[1];
    check(occupied==(exempt?3u:2u),"Live player carry limit ignored original flag4 exemption");
    check(first_carried==exempt,"Live pickup replaced an item when original player restriction allows another carry");
  }
  auto c=arena(false);c.weapons.at(3).inventory_flags=8;
  auto spawns=starts();MultiplayerMatch m(c,0,spawns,rules());m.configure_weapon_pickups();const auto id=m.join("Restricted hand owner");
  const auto original=m.unit(id).weapon.source_datum;std::array<Controls,16> input{};
  input[id].pickup=true;input[id].pickup_milliseconds=233;m.step(input);
  check(m.unit(id).weapon.source_datum==original && m.state().world_weapons.size()==1,
      "Live pickup ignored original flag3 restriction on the current hand");
}
void native_requests() {
  MultiplayerKbm input(42);
  const auto key=[&](SDL_Scancode scan,bool down) {
    SDL_Event event{};event.type=down?SDL_KEYDOWN:SDL_KEYUP;event.key.windowID=42;
    event.key.keysym.scancode=scan;input.handle_event(event);
  };
  key(SDL_SCANCODE_E,true);key(SDL_SCANCODE_E,false);
  key(SDL_SCANCODE_X,true);key(SDL_SCANCODE_X,false);
  key(SDL_SCANCODE_TAB,true);key(SDL_SCANCODE_TAB,false);
  const auto c=input.consume().controls;
  check(c.pickup && c.dual_wield && c.switch_weapon,"SDL short pickup/dual/switch taps were lost");
  const auto released=input.consume().controls;
  check(!released.pickup && !released.dual_wield && !released.switch_weapon,"Consumed item requests repeated");
  key(SDL_SCANCODE_X,true);
  SDL_Event focus{};focus.type=SDL_WINDOWEVENT;focus.window.windowID=42;focus.window.event=SDL_WINDOWEVENT_FOCUS_LOST;
  input.handle_event(focus);check(!input.consume().controls.dual_wield,"Focus loss retained a dual pickup request");
}
void ammunition_inventory_sum() {
  Unit u;auto &i=u.source_inventory;i.current={0,1};i.slots={10,11,12,13};
  u.weapon.definition=3;u.weapon.source_datum=10;u.weapon.loaded=7;u.weapon.reserve=12;
  for(unsigned n=1;n<4;++n){WeaponState w;w.definition=n==3?4:3;w.source_datum=10+n;
    w.loaded=99;w.reserve=int(n)*3;i.inactive_weapons[n]=w;}
  check(native_weapon_ammunition_available(u,u.weapon)==28,"104CF0 must include inactive matching reserve and exclude other loaded rounds");
  check(native_weapon_ammunition_available(u,*native_hand_weapon(u,1))==120,"104CF0 secondary total must use its own loaded rounds");
}
void network_inventory() {
  auto content=arena(true);content.weapons.at(3).projectile=9;
  content.weapons.at(3).projectile_speed=3;
  auto spawns=starts();MultiplayerMatch match(content,0,spawns,rules());
  match.configure_weapon_pickups();LanHost host(match,0,12345);
  LanClient client("127.0.0.1:"+std::to_string(host.port()),12345,1);
  std::array<Controls,1> local{};std::array<Controls,16> commands{};
  for(unsigned tick=0;tick<40 && !client.ready();++tick) {
    client.send(local);host.receive(commands);host.broadcast();client.receive();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  check(client.ready(),"Inventory UDP handshake failed");
  const auto id=client.slots()[0];match.step(commands);host.broadcast();client.receive();
  // Explicit native transport controls, not claimed original gameplay states.
  auto &authority=const_cast<Unit&>(match.unit(id));
  authority.weapon.shader={2,0x20,-.5f,.75f};host.broadcast();client.receive();
  check(client.state().players[id].weapon_shader.item_flags==2 &&
      client.state().players[id].weapon_shader.weapon_flags==0x20 &&
      client.state().players[id].weapon_shader.power_fraction==-.5f &&
      client.state().players[id].weapon_shader.heat==.75f,
      "Authoritative primary shader fields changed over UDP");
  check(client.state().world_weapons.size()==1,"Authoritative world weapon did not reach client");
  const auto item=client.state().world_weapons[0].handle;
  local[0].dual_wield=true;local[0].dual_wield_milliseconds=232;client.send(local);
  local[0].dual_wield=false;local[0].dual_wield_milliseconds=0;client.send(local);
  host.receive(commands);match.step(commands);
  check(!native_hand_weapon(match.unit(id),1),"UDP completion promoted a 232ms hold past the source threshold");
  host.receive(commands);match.step(commands);
  local[0].dual_wield=true;local[0].dual_wield_milliseconds=233;client.send(local);
  local[0].dual_wield=false;local[0].dual_wield_milliseconds=0;client.send(local);
  client.send(local); // A later neutral packet must not erase the completed hold.
  host.receive(commands);check(commands[id].dual_wield,"Coalesced dual pickup tap was lost");
  check(commands[id].dual_wield_milliseconds==233,"Coalesced UDP release lost its completed hold duration");
  match.step(commands);host.broadcast();client.receive();
  check(client.state().players[id].secondary_weapon.datum==item && client.state().world_weapons.empty(),
      "Dual ownership/world removal snapshot differs over UDP");
  check(client.state().players[id].ammunition_available==native_weapon_ammunition_available(authority,authority.weapon) &&
      client.state().players[id].secondary_weapon.ammunition_available==native_weapon_ammunition_available(authority,*native_hand_weapon(authority,1)),
      "Authoritative primary/secondary shared ammunition totals changed over UDP");
  native_hand_weapon(authority,1)->ready_clock=false; // Explicit transport-only raw-state control.
  native_hand_weapon(authority,1)->shader={3,0x40,std::bit_cast<float>(0x7fc01234u),std::bit_cast<float>(0x7fc05678u)};
  host.broadcast();client.receive();
  check(client.state().players[id].secondary_weapon.shader.item_flags==3 &&
      client.state().players[id].secondary_weapon.shader.weapon_flags==0x40 &&
      std::bit_cast<std::uint32_t>(client.state().players[id].secondary_weapon.shader.power_fraction)==0x7fc01234u &&
      std::bit_cast<std::uint32_t>(client.state().players[id].secondary_weapon.shader.heat)==0x7fc05678u,
      "Independent offhand shader words changed over UDP");
  host.receive(commands);check(!commands[id].dual_wield,"Dual pickup pulse was delivered twice");
  local[0].fire_secondary=true;client.send(local);local[0].fire_secondary=false;client.send(local);
  host.receive(commands);match.step(commands);host.broadcast();client.receive();
  check(client.state().players[id].loaded==6 && client.state().players[id].secondary_weapon.loaded==5,
      "Coalesced offhand trigger did not preserve independent ammunition over UDP");
  check(client.state().players[id].motion_sensor_control==0x20,
      "Authoritative offhand radar trigger did not survive coalesced UDP delivery");
  const auto actual=match.state().weapon_projectiles;
  const auto replica=client.state().weapon_projectiles;
  check(!actual.empty() && actual.size()==replica.size(),"Actual fired projectile bank was lost over UDP");
  for(unsigned i=0;i<actual.size();++i) {
    check(actual[i].datum==replica[i].datum && actual[i].definition==replica[i].definition,
        "Projectile full identity changed over UDP");
    check(actual[i].position.x==replica[i].position.x && actual[i].velocity.x==replica[i].velocity.x &&
        actual[i].forward.x==replica[i].forward.x && actual[i].up.y==replica[i].up.y &&
        actual[i].traveled==replica[i].traveled,
        "Projectile world pose/velocity changed over UDP");
  }
  // Retain the offhand's exact raw shader words through actual grenade drop
  // and the world UDP bank, including a NaN payload used as a wire control.
  commands={};commands[id].grenade=true;match.step(commands);
  host.broadcast();client.receive();
  const auto dropped=std::find_if(client.state().world_weapons.begin(),client.state().world_weapons.end(),
      [&](const auto &w){return w.handle==item;});
  check(dropped!=client.state().world_weapons.end(),"Dropped offhand owner was lost from the world UDP bank");
  check(dropped->shader.item_flags==2 && dropped->shader.weapon_flags==0x40 &&
      std::bit_cast<std::uint32_t>(dropped->shader.power_fraction)==0x7fc01234u &&
      std::bit_cast<std::uint32_t>(dropped->shader.heat)==0x7fc05678u,
      "Dropped world weapon changed its independent raw shader words over UDP");
}

}
int main(){try{ammunition_inventory_sum();zero_rate_latched_fire();pickup_hold_boundary();dual_gameplay();mixed_dual_gameplay();pickup_switch();pickup_bounds();player_carry_admission();native_requests();network_inventory();std::cout<<"PASS map pickup, rejection, stable datums, same/mixed weapon hand fire/reload/ammo, switch, drop and row respawn\n";}
catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
