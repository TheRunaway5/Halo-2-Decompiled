#include "halo2/hud.hpp"
#include "halo2/math.hpp"
#include "halo2/native_weapon_inventory.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/source_device_world.hpp"
#include "halo2/action_prompt_bindings.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <set>

using namespace halo2;
namespace {
void check(bool value,const std::string &why) {
  if(!value)throw FormatError(why);
}
std::string substitute(std::string text,bool gamepad,std::array<std::uint32_t,2> buttons={0xe102,0xe103}) {
  const auto glyph=[](std::uint32_t c){return std::string("\xee\x84")+char(0x80+c-0xe100);};
  for(const auto &[token,label]:std::array<std::pair<std::string,std::string>,2>{{
      {"\xee\x91\x88",gamepad?glyph(buttons[0]):"\xee\x84\xb2" "E"},
      {"\xee\x91\x89",gamepad?glyph(buttons[1]):"\xee\x84\xb2" "X"}}})
    for(std::size_t p=0;(p=text.find(token,p))!=std::string::npos;p+=label.size())
      text.replace(p,token.size(),label);
  return text;
}
unsigned message_selector(std::uint16_t flags) {
  return flags&3 ? unsigned(!(flags&1))+(flags&12?2:0) : 4;
}
bool control_is_placed(const Cache &cache,std::uint32_t datum) {
  const auto &tags=cache.tags();
  const auto scenario=std::find_if(tags.begin(),tags.end(),[](const auto &tag){return tag.group=="scnr";});
  const auto metadata=cache.load_source_tag_metadata(scenario->datum);
  const Bytes data(metadata.bytes_at(metadata.source_address(),metadata.size()));
  for(unsigned i=0;i<data.u32(0xb8);++i) {
    const Bytes row(metadata.bytes_at(data.u32(0xbc)+i*0x44,0x44));
    const auto index=std::int16_t(row.u16(0));if(index<0)continue;
    check(unsigned(index)<data.u32(0xc0),"Control placement has invalid palette index");
    const Bytes palette(metadata.bytes_at(data.u32(0xc4)+unsigned(index)*0x28,0x28));
    if(palette.u32(4)==datum)return true;
  }
  return false;
}
unsigned control_poses(const Cache &cache) {
  const auto &tags=cache.tags();
  const auto scenario=std::find_if(tags.begin(),tags.end(),[](const auto &tag){return tag.group=="scnr";});
  const auto metadata=cache.load_source_tag_metadata(scenario->datum);
  const Bytes data(metadata.bytes_at(metadata.source_address(),metadata.size()));
  unsigned count=0;
  for(unsigned i=0;i<data.u32(0xb8);++i) {
    const Bytes row(metadata.bytes_at(data.u32(0xbc)+i*0x44,0x44));
    const auto index=std::int16_t(row.u16(0));if(index<0)continue;
    check(unsigned(index)<data.u32(0xc0),"Control placement has invalid palette index");
    const Bytes palette(metadata.bytes_at(data.u32(0xc4)+unsigned(index)*0x28,0x28));
    ScenarioObject object;object.type=8;object.placement=int(i);object.definition=palette.u32(4);
    const auto point=[&](unsigned at){return Vec3{row.f32(at),row.f32(at+4),row.f32(at+8)};};
    object.position=point(8);object.rotation=point(0x14);object.scale=row.f32(0x20);
    if(object.scale<=0)object.scale=1;
    const auto definition=load_source_device_world_definition(cache,object);
    check(definition && definition->control,"Placed control lost its device definition");
    // A model-less authored control has no visible target/marker geometry.
    if(definition->control_nodes.empty())continue;
    SourceDeviceWorldOwner owner;owner.root.position=object.position;owner.root.scale=object.scale;
    const auto basis=authored_basis(object.rotation);owner.root.basis={basis.forward,basis.left,basis.up};
    for(const float phase:{0.f,.5f,1.f}) {
      owner.motion.position=phase;owner.motion.power=1;
      const auto nodes=source_control_world_nodes(*definition,owner);
      check(nodes.size()==definition->control_nodes.size(),"Control pose changed its model node bank");
      const auto sphere=source_shadow_sphere(definition->control_bounds,nodes.front(),owner.root.scale);
      check(std::isfinite(sphere.radius) && sphere.radius>=0,"Control target bounds are invalid");
      const auto markers=pose_model_markers(definition->control_markers,nodes,definition->control_permutations,{},1);
      const auto forward=markers.empty()?nodes[0].basis[0]:markers[0].world.basis[0];
      // Isolate source geometry admission from campaign scripts and group
      // availability: the supplied current owner is powered and unused.
      SourceDeviceGroup position{phase,0},power{1,0};
      SourceControlTargetQuery query;query.unit_forward=forward*-1;
      query.unit_root=query.unit_center=query.unit_bound_center=sphere.center+forward*.1f;
      query.unit_bound_radius=.5f;
      SourceControlTargetCandidate candidate;candidate.object=0xface0001;candidate.root=owner.root.position;
      candidate.bound_center=sphere.center;candidate.bound_radius=sphere.radius;
      candidate.marker_count=std::uint16_t(markers.size());candidate.marker_forward=forward;
      candidate.position=&position;candidate.power=&power;
      candidate.control_flags=source_control_initial_flags(definition->placement.machine_flags);
      SourcePlayerActionTarget target;
      check(source_control_target_candidate(query,candidate,target,{}),"Current authored control pose cannot be targeted");
      check(target.action==3 && target.object==candidate.object,"Control geometry selected the wrong action");
      if(markers.empty()) {
        // Original B8D30 returns zero for a missing named use marker even
        // though it writes node zero. 1078F0 then accepts either approach.
        query.unit_forward=forward;
        query.unit_root=query.unit_center=query.unit_bound_center=sphere.center-forward*.1f;
        target={};
        check(source_control_target_candidate(query,candidate,target,{}),
            "Missing named control marker incorrectly enforced a side gate");
      }
    }
    ++count;
  }
  return count;
}
std::uint32_t weapon_named(const LevelContent &content,std::string_view name) {
  for(const auto &[id,w]:content.weapons)if(w.name.ends_with(name))return id;
  throw FormatError("Required authored weapon fixture missing: "+std::string(name));
}
void reset_item_rows(LevelContent &content,std::span<const std::uint32_t> definitions,Vec3 at) {
  content.netgame_equipment.clear();
  for(const auto definition:definitions) {
    NetgameEquipment row;row.modes={2,0,0,0};row.respawn_seconds=300;
    row.position=at;row.position.x+=float(content.netgame_equipment.size())*20;
    row.items.push_back({1,0x77656170u,definition,0});content.netgame_equipment.push_back(row);
  }
}
const NativeWeaponPickup &world_item(const MultiplayerMatch &match,std::uint32_t definition) {
  const auto rows=match.weapon_pickups();
  const auto found=std::find_if(rows.begin(),rows.end(),[&](const auto &row){return row.weapon && row.definition==definition;});
  check(found!=rows.end(),"Authored usable weapon did not become a native world item");return *found;
}
void approach(MultiplayerMatch &match,unsigned player,const NativeWeaponPickup &item) {
  auto &u=const_cast<Unit&>(match.unit(player));u.body.position=item.position;u.body.velocity={};
}
void assert_message(MultiplayerHud &hud,const HudContent &content,const MultiplayerState &state,
                    unsigned player,std::uint32_t definition,const std::string &why) {
  const auto &prompt=state.players[player].action_prompt;
  check(prompt.definition==definition && (prompt.target.action==1 || prompt.target.action==7),why+": lost weapon context");
  const auto flags=std::uint16_t(prompt.target.index);
  check(flags!=0,why+": no available action");
  const auto message=content.action_messages.weapons.at(definition).at(message_selector(flags));
  check(message!=0 && content.strings.contains(message),why+": selected action has no authored message");
  for(const bool gamepad:{false,true}) {
    hud.prompt_gamepad(0,gamepad);
    check(hud.action_text(state,player,0)==substitute(content.strings.at(message),gamepad),why+": displayed a different weapon's message");
    check(!hud.draw(state,player,0,1,1280,800,0,0).empty(),why+": action context did not draw a button prompt");
  }
}
void transfer_case(const LevelContent &original,std::span<const Spawn> spawns,MultiplayerHud &hud,
    const HudContent &messages,std::uint32_t target,bool swap,bool dual) {
  auto content=original;content.collision.clear();
  const auto smg=weapon_named(content,"\\smg"),br=weapon_named(content,"\\battle_rifle");
  const auto filler=weapon_named(content,dual?(target==weapon_named(content,"\\plasma_rifle")?"\\magnum":"\\plasma_rifle"):
      target==weapon_named(content,"\\needler")?"\\magnum":"\\needler");
  SlayerRules rules;rules.weapon=target==smg?br:smg;rules.time_ticks=rules.score_limit=0;
  if(dual)rules.weapon=target==smg?weapon_named(content,"\\magnum"):smg;
  content.multiplayer_starting_equipment_flags=0;
  const std::array<std::uint32_t,2> definitions{swap||dual?filler:target,target};
  reset_item_rows(content,std::span(definitions).first(swap||dual?2:1),spawns.front().position);
  MultiplayerMatch match(content,0,spawns,rules);match.configure_weapon_pickups();
  const auto id=match.join("All weapon prompt test");std::array<Controls,multiplayer_capacity> input{};
  for(unsigned tick=0;tick<30;++tick)match.step(input);
  const auto take=[&](std::uint32_t definition,bool secondary) {
    const auto &item=world_item(match,definition);const auto handle=item.handle;
    approach(match,id,item);const auto state=match.state();
    const auto why=content.weapons.at(definition).name+(secondary?" dual":swap?" swap":" pickup");
    assert_message(hud,messages,state,id,definition,why);
    const auto flags=state.players[id].action_prompt.target.index;
    check(bool(flags&(secondary?4:3)),why+": advertised action is unavailable");
    const auto before=match.unit(id).weapon.source_datum;
    input[id].pickup=!secondary;input[id].dual_wield=secondary;
    input[id].switch_weapon=secondary; // A completed Y hold and release can arrive in one native tick.
    input[id].pickup_milliseconds=input[id].dual_wield_milliseconds=233;
    match.step(input);
    const auto *held=native_hand_weapon(match.unit(id),secondary?1:0);
    check(held && held->source_datum==handle,why+": displayed button did not transfer the selected world item");
    if(secondary)check(match.unit(id).weapon.source_datum==before,why+": offhand action replaced the primary item");
    check(match.action_prompt(id).target.object!=handle,why+": consumed item retained its prompt");
    input[id]={};input[id].switch_weapon=secondary;match.step(input);
    if(secondary)check(match.unit(id).weapon.source_datum==before,why+": consumed dual release switched the primary weapon");
    input[id]={};
  };
  if(swap || dual)take(filler,false);
  take(target,dual);
}
unsigned activate_controls(const LevelContent &original,std::span<const Spawn> spawns,
    MultiplayerHud &hud,const HudContent &messages) {
  auto content=original;content.collision.clear();content.netgame_equipment.clear();
  SlayerRules rules;rules.weapon=weapon_named(content,"\\smg");rules.time_ticks=rules.score_limit=0;
  MultiplayerMatch inventory(content,0,spawns,rules);inventory.configure_weapon_pickups();
  std::vector<std::uint32_t> handles;
  for(const auto &[handle,owner]:inventory.simulation().devices().owners())
    if(const auto &d=*content.devices.at(owner.placement);d.control && d.control->action_string && !d.control_nodes.empty())
      handles.push_back(handle);
  for(const auto handle:handles) {
    MultiplayerMatch match(content,0,spawns,rules);match.configure_weapon_pickups();
    const auto id=match.join("All control prompt test");std::array<Controls,multiplayer_capacity> input{};
    for(unsigned tick=0;tick<30;++tick)match.step(input);
    const auto &owner=*match.simulation().devices().find(handle);
    const auto &definition=*content.devices.at(owner.placement);
    const auto nodes=source_control_world_nodes(definition,owner);
    const auto sphere=source_shadow_sphere(definition.control_bounds,nodes.front(),owner.root.scale);
    const auto markers=pose_model_markers(definition.control_markers,nodes,definition.control_permutations,{},1);
    const auto forward=markers.empty()?nodes[0].basis[0]:markers[0].world.basis[0];
    auto &unit=const_cast<Unit&>(match.unit(id));
    unit.body.position=sphere.center+forward*.2f-Vec3{0,0,unit.definition.camera_height};unit.body.velocity={};
    unit.yaw=std::atan2(-forward.y,-forward.x);unit.pitch=std::atan2(-forward.z,std::hypot(forward.x,forward.y));
    const auto state=match.state();const auto &prompt=state.players[id].action_prompt;
    check(prompt.target.action==3 && prompt.target.object==handle && prompt.definition==definition.control->datum,
        "Native control idle prompt did not select its actual device owner");
    const auto buttons=native_action_prompt_controller_buttons({});hud.prompt_gamepad_buttons(0,buttons);
    for(const bool gamepad:{false,true}) {
      hud.prompt_gamepad(0,gamepad);
      check(hud.action_text(state,id,0)==substitute(messages.strings.at(definition.control->action_string),gamepad,buttons),
          "Native control displayed a different device's action text");
      check(!hud.draw(state,id,0,1,1280,800,0,0).empty(),"Native control prompt produced no button geometry");
    }
    const auto before=match.simulation().devices().position_group(owner);
    input[id].yaw=unit.yaw;input[id].pitch=unit.pitch;input[id].pickup=true;input[id].pickup_milliseconds=233;
    match.step(input);
    const auto &after=match.simulation().devices().position_group(*match.simulation().devices().find(handle));
    check(after.value!=before.value,"Displayed native control button did not change its actual device group");
    if(before.flags&1)check(match.action_prompt(id).target.object!=handle,"Used one-shot control retained its prompt");
  }
  return unsigned(handles.size());
}
}
int main(int argc,char **argv) {try {
  check(argc==3,"Supply MAPS VISTA_UI");const std::filesystem::path maps=argv[1],artwork=argv[2];
  if(!std::filesystem::is_directory(maps) || !std::filesystem::exists(artwork/"fonts/conduit-13"))return 77;
  std::vector<std::filesystem::path> paths;
  for(const auto &entry:std::filesystem::directory_iterator(maps))
    if(entry.path().extension()==".map" && entry.path().filename()!="shared.map" &&
        entry.path().filename()!="single_player_shared.map" && entry.path().filename()!="mainmenu.map")paths.push_back(entry.path());
  std::sort(paths.begin(),paths.end());
  check(!paths.empty(),"No gameplay maps supplied to the prompt catalog");
  PcKeyboardSettings keyboard;keyboard.bindings[unsigned(PcAction::Pickup)]=SDL_SCANCODE_F;
  keyboard.bindings[unsigned(PcAction::DualPickup)]=SDL_SCANCODE_SPACE;
  check(native_action_prompt_keyboard_buttons(keyboard)==std::array<std::string,2>{"F","Space"},
      "Prompt labels ignored configured keyboard pickup bindings");
  for(unsigned layout=0;layout<4;++layout)for(unsigned sticks=0;sticks<4;++sticks) {
    ControllerSettings settings;settings.buttons=layout;settings.sticks=sticks;
    check(native_action_prompt_controller_buttons(settings)==std::array<std::uint32_t,2>{0xe102,0xe103},
        "Controller prompt differs from the active adapter's X/Y bindings");
  }
  unsigned map_count=0,weapon_messages=0,control_messages=0,live_cases=0,posed_controls=0,other_messages=0,live_controls=0;
  std::set<std::string> weapon_names,control_names,usable_names;
  for(const auto &path:paths) {
    Cache cache(path);const auto &tags=cache.tags();
    posed_controls+=control_poses(cache);
    auto biped=std::find_if(tags.begin(),tags.end(),[&](const auto &tag) {
      if(tag.group!="bipd" || !tag.address)return false;
      const auto metadata=cache.load_source_tag_metadata(tag.datum);
      return Bytes(metadata.bytes_at(metadata.source_address(),metadata.size())).u32(0x1a0)>0;
    });
    if(biped==tags.end())biped=std::find_if(tags.begin(),tags.end(),[](const auto &tag){return tag.group=="bipd" && tag.address;});
    auto weapon=std::find_if(tags.begin(),tags.end(),[](const auto &tag){return tag.group=="weap" && tag.address && tag.name.ends_with("\\smg");});
    if(weapon==tags.end())weapon=std::find_if(tags.begin(),tags.end(),[](const auto &tag){return tag.group=="weap" && tag.address;});
    check(biped!=tags.end() && weapon!=tags.end(),path.filename().string()+": no source unit/weapon HUD fixture");
    auto messages=cache.load_hud(biped->datum,weapon->datum);messages.widgets.clear();
    Mesh mesh;MultiplayerHud hud(messages,maps/"fonts",mesh,4,{},std::nullopt,artwork);
    MultiplayerState state;auto &player=state.players[0];player.connected=true;
    const auto other_message=[&](unsigned action,int index,std::uint32_t definition,std::uint32_t sid) {
      if(!sid || sid==null_datum || !messages.strings.contains(sid) || messages.strings.at(sid).empty())return;
      player.action_prompt={{std::int16_t(action),std::int16_t(index),0xface0001},definition,0};
      for(const bool gamepad:{false,true}) {
        hud.prompt_gamepad(0,gamepad);
        check(hud.action_text(state,0,0)==substitute(messages.strings.at(sid),gamepad),"Authored trade/seat/flip action text was lost");
        check(!hud.draw(state,0,0,1,1280,800,0,0).empty(),"Authored trade/seat/flip action produced no prompt geometry");
      }
      ++other_messages;
    };
    for(const auto &tag:tags)if(tag.address && tag.group=="weap") {
      const auto &ids=messages.action_messages.weapons.at(tag.datum);
      const auto metadata=cache.load_source_tag_metadata(tag.datum);
      const Bytes definition(metadata.bytes_at(metadata.source_address(),metadata.size()));
      other_message(2,0,tag.datum,definition.u32(0xf0));
      for(unsigned selector=0;selector<ids.size();++selector) {
        // Several non-dual weapons retain unused IDs outside their unic range.
        // The source capability check never selects those three messages.
        if(selector>=2 && !(definition.u32(0x12c)&(1u<<22)))continue;
        if(!ids[selector])continue;
        check(messages.strings.contains(ids[selector]),path.filename().string()+": missing nonzero authored weapon action string");
        const auto expected=messages.strings.at(ids[selector]);if(expected.empty())continue;
        constexpr std::array<std::int16_t,5> flags{1,2,5,6,4};
        player.action_prompt={{1,flags[selector],0xface0001},tag.datum,0};
        for(const bool gamepad:{false,true}) {
          hud.prompt_gamepad(0,gamepad);
          check(hud.action_text(state,0,0)==substitute(expected,gamepad),tag.name+": authored action selector was not rendered");
          check(!hud.draw(state,0,0,1,1280,800,0,0).empty(),tag.name+": authored action selector produced no geometry");
        }
        ++weapon_messages;weapon_names.insert(tag.name);
      }
    }
    for(const auto &tag:tags)if(tag.address && (tag.group=="vehi" || tag.group=="bipd")) {
      const auto metadata=cache.load_source_tag_metadata(tag.datum);
      const Bytes definition(metadata.bytes_at(metadata.source_address(),metadata.size()));
      for(unsigned seat=0;seat<definition.u32(0x1c8);++seat) {
        const Bytes row(metadata.bytes_at(definition.u32(0x1cc)+seat*0xb0,0xb0));
        for(unsigned kind=4;kind<=6;++kind)other_message(kind,int(seat),tag.datum,row.u32(0x84));
      }
      if(tag.group=="vehi")other_message(8,0,tag.datum,definition.u32(0x21c));
    }
    for(const auto &tag:tags)if(tag.address && tag.group=="ctrl") {
      const auto control=cache.load_source_control_definition(tag.datum);
      if(!control.action_string)continue;
      if(!messages.strings.contains(control.action_string)) {
        check(!control_is_placed(cache,tag.datum),tag.name+": placed control has no authored action string");
        std::cout<<"SOURCE "<<path.filename().string()<<": unplaced control has no locale entry: "<<tag.name<<'\n';
        continue;
      }
      const auto expected=messages.strings.at(control.action_string);if(expected.empty())continue;
      player.action_prompt={{3,0,0xface0001},tag.datum,control.action_string};
      for(const bool gamepad:{false,true}) {
        hud.prompt_gamepad(0,gamepad);
        check(hud.action_text(state,0,0)==substitute(expected,gamepad),tag.name+": lost authored control action text");
        check(!hud.draw(state,0,0,1,1280,800,0,0).empty(),tag.name+": control prompt produced no geometry");
      }
      ++control_messages;control_names.insert(tag.name);
    }
    ++map_count;
    const auto scenario=std::find_if(tags.begin(),tags.end(),[](const auto &tag){return tag.group=="scnr";});
    const auto metadata=cache.load_source_tag_metadata(scenario->datum);
    const Bytes scenario_data(metadata.bytes_at(metadata.source_address(),metadata.size()));
    if(scenario_data.u16(0x10)!=1)continue;
    auto content=cache.load_content(0,true);const auto spawns=cache.spawns();
    check(!spawns.empty(),"Multiplayer map lacks source player starts");
    unsigned usable=0;
    for(const auto &[id,w]:content.weapons) {
      if(w.first_person_models.empty() || w.first_person_models.front().model==null_datum ||
          w.first_person_models.front().animation==null_datum)continue;
      // No message means the original does not advertise this item as a pickup.
      if(!messages.action_messages.weapons.at(id)[0])continue;
      transfer_case(content,spawns,hud,messages,id,false,false);++live_cases;
      transfer_case(content,spawns,hud,messages,id,true,false);++live_cases;
      if(w.dual_wieldable){transfer_case(content,spawns,hud,messages,id,false,true);++live_cases;}
      ++usable;usable_names.insert(w.name);
    }
    check(usable>=14,path.filename().string()+": weapon coverage unexpectedly shrank");
    live_controls+=activate_controls(content,spawns,hud,messages);
    std::cout<<"PASS "<<path.filename().string()<<": "<<usable<<" usable weapons, pickup/swap/compatible dual actions\n";
  }
  check(map_count==paths.size(),"Catalog omitted available map HUDs");
  check(usable_names.size()>=14 && control_names.size()>1,"Coverage collapsed to the named examples");
  std::cout<<"PASS "<<map_count<<" map HUDs; "<<weapon_messages<<" weapon action strings across "<<weapon_names.size()
      <<" definitions; "<<control_messages<<" control action strings across "<<control_names.size()
      <<" definitions; "<<posed_controls<<" placed control poses; "<<other_messages<<" trade/seat/flip message routes; "
      <<live_cases<<" native transfer cases across "<<usable_names.size()<<" usable weapon definitions; "<<live_controls<<" native control activations\n";
  for(const auto &name:usable_names)std::cout<<"NATIVE_WEAPON "<<name<<'\n';
  return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
