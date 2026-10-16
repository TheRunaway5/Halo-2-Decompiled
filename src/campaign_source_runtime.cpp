#include "halo2/campaign_source_runtime.hpp"
#include "halo2/native_weapon_hud.hpp"
#include "halo2/native_weapon_inventory.hpp"
#include "halo2/source_scene_physics.hpp"
#include "halo2/math.hpp"
#include "halo2/original_data.hpp"
#include "halo2/player_command.hpp"
#include "halo2/source_script.hpp"
#include <algorithm>
#include <bit>
#include <cstring>
#include <set>
#include <unistd.h>
namespace halo2 {
namespace {
template<class T>T read(const SourceRuntimeMemory &memory,NativeRuntimeAddress body,unsigned offset) {
  static_assert(std::endian::native==std::endian::little);
  T value;memory.read(body+offset,std::as_writable_bytes(std::span(&value,1)));return value;
}
}
CampaignSourceRuntime::CampaignSourceRuntime(const Cache &cache,const std::filesystem::path &xbe,
    std::size_t bsp,unsigned difficulty,std::uint32_t seed) {
  if(cache.is_mcc() || cache.original_data()->image!=OriginalData::Image::TitleUpdate15 || difficulty>3)
    throw FormatError("Campaign source ownership requires an authenticated TU1.5 Xbox cache/session");
  auto name=(std::filesystem::temp_directory_path()/"halo2-campaign-XXXXXX").string();
  if(!::mkdtemp(name.data()))throw FormatError("Campaign source storage unavailable");
  files_=name;
  try {
    objects_=std::make_unique<MultiplayerObjectRuntime>(xbe,cache,bsp,files_);
    auto &world=objects_->tu15_world();world.initialize_map();
    world.configure_campaign_session(std::int16_t(difficulty),1,30,seed);
  } catch(...) {std::error_code ec;std::filesystem::remove_all(files_,ec);throw;}
}
CampaignSourceRuntime::~CampaignSourceRuntime() {
  objects_.reset();std::error_code ec;std::filesystem::remove_all(files_,ec);
}
void CampaignSourceRuntime::set_difficulty(unsigned value) {
  if(value>3)throw FormatError("Invalid campaign source difficulty");
  auto &memory=objects_->memory();
  const auto options=Bytes(memory.view(0x4f3218u,4)).u32(0);
  const std::array bytes{std::byte(value),std::byte{0}};
  memory.write(options+0x132u,bytes);
}
void CampaignSourceRuntime::set_team_allegiance(std::int16_t a,std::int16_t b,bool allied) {
  if(a< -1 || b< -1 || a>=16 || b>=16)throw FormatError("Campaign allegiance team outside original table");
  auto &world=objects_->tu15_world();
  if(allied) {
    auto input=world.prepare_source_call();input.gp[1]=std::uint32_t(std::int32_t(a));
    input.gp[0]=std::uint32_t(std::int32_t(b));world.run_actual(0x27e000u,input);
  } else {
    const std::array arguments{std::uint32_t(std::int32_t(b))};
    auto input=world.prepare_source_call(arguments);input.gp[3]=std::uint32_t(std::int32_t(a));
    world.run_actual(0x1e7a20u,input);
  }
}
void CampaignSourceRuntime::switch_bsp(std::size_t bsp) {objects_->tu15_world().switch_campaign_bsp(bsp);}
void CampaignSourceRuntime::set_kill_volume_enabled(std::int16_t trigger,bool enabled) {
  objects_->tu15_world().set_kill_volume_enabled(trigger,enabled);
}
std::optional<unsigned> CampaignSourceRuntime::random_choice(unsigned count,std::uint32_t visited) {
  auto &memory=objects_->memory();const auto random=Bytes(memory.view(0x4f3cd8u,4)).u32(0);
  auto state=Bytes(memory.view(random,4)).u32(0);
  const auto result=source_hs_begin_random(state,count,visited);
  memory.write(random,std::as_bytes(std::span(&state,1)));return result;
}
NativeObjectLifetimes CampaignSourceRuntime::lifetimes() {
  return NativeObjectLifetimes(objects_->memory(),objects_->object_pool());
}
CampaignSourceRuntime::Checkpoint CampaignSourceRuntime::checkpoint(const NativeObjectLifetimes &lifetimes)const {
  return {objects_->authority_checkpoint(),lifetimes.source_authority_checkpoint(),player_,placements_};
}
void CampaignSourceRuntime::restore(const Checkpoint &saved,NativeObjectLifetimes &lifetimes) {
  auto placements=saved.placements;
  auto world=objects_->prepare_authority(saved.objects);
  auto pool=lifetimes.prepare_source_authority(saved.lifetimes,world.replacement_memory());
  if(!objects_->commit_authority(std::move(world)) || !lifetimes.commit_source_authority(std::move(pool)))
    throw FormatError("Campaign source checkpoint did not publish its coupled world/pool backing");
  player_=saved.player;placements_.swap(placements);
}
SourceSceneObjectOwner CampaignSourceRuntime::scene_owner(const LevelContent &content) {
  return {[this,&content](const ScenarioObject &placement,SourceScenarioBirth birth) {
    if(!placement.model_attachment) {
      const auto object=objects_->create_scenario_object(placement,birth);
      if(object!=null_datum)placements_[{placement.type,placement.placement}]=object;
      return object;
    }
    const auto resolve=[&](auto &&self,std::size_t index,unsigned depth)->std::uint32_t {
      if(depth>8)throw FormatError("Campaign vehicle child hierarchy is cyclic");
      const auto &p=content.objects.at(index);
      if(p.model_attachment)return objects_->child_object(self(self,p.model_attachment->parent,depth+1),p.definition);
      const auto found=placements_.find({p.type,p.placement});
      if(found!=placements_.end())return found->second;
      throw FormatError("Campaign model child has no retained authored parent");
    };
    const auto parent=resolve(resolve,placement.model_attachment->parent,0);
    const auto child=objects_->child_object(parent,placement.definition);
    if(child==null_datum)throw FormatError("Campaign source constructor did not publish its model child");
    return child;
  },[this](auto object){objects_->destroy_object(object);}};
}
void CampaignSourceRuntime::respawn(Simulation &simulation,Spawn spawn) {
  if(player_==null_datum) {
    DatumPool players(objects_->memory(),objects_->player_pool());player_=players.allocate_at(0);
    if(player_==null_datum)throw FormatError("Campaign player pool exhausted");
    auto configuration=source_default_player_configuration();
    //14E7B0 consumes the actual campaign starting-profile representation and
    //team before the new-player continuation. This also admits Arbiter's
    //representation1 to the original HUD appearance query.
    constexpr std::uint32_t profile=0x6005b000u;
    auto &world=objects_->tu15_world();
    world.memory().write(profile,std::as_bytes(std::span(configuration)));
    auto input=world.prepare_source_call();input.gp[2]=profile;
    world.run_actual(0x14e7b0u,input);
    world.memory().read(profile,std::as_writable_bytes(std::span(configuration)));
    objects_->construct_player(player_,0,configuration);
  }
  auto &unit=simulation.player_;
  if(unit.source_datum!=null_datum && simulation.object_lifetimes_.contains(unit.source_datum)) {
    objects_->destroy_object(unit.source_datum);
    simulation.object_lifetimes_.observe_source_retirement(unit.source_datum);
  }
  const auto zoom_reset=unit.zoom_reset_serial+1u;
  unit={};unit.zoom_reset_serial=zoom_reset;unit.definition=simulation.content_.player;unit.bsp=int(simulation.bsp_);
  unit.source_player=player_;unit.yaw=spawn.facing;unit.team=unit.definition.team;
  unit.source_datum=objects_->create_biped(unit.definition.datum,spawn.position,direction(spawn.facing),player_);
  if(unit.source_datum==null_datum)throw FormatError("Original campaign biped birth returned NONE");
  simulation.object_lifetimes_.observe_source_birth(unit.source_datum);
  objects_->bind_player(player_,unit.source_datum);
  objects_->initialize_player_equipment(player_,unit.source_datum);
  objects_->set_local_player(player_,0);
  simulation.player_pitch_=0;adopt(simulation);
}
void CampaignSourceRuntime::adopt_unit(Unit &unit,const LevelContent &content,NativeObjectLifetimes &lifetimes) {
  const auto &memory=objects_->memory();const auto body=objects_->object_body(unit.source_datum);
  const auto motion=objects_->biped_motion(unit.source_datum);
  unit.body={motion.position,motion.velocity,unit.definition.height,unit.definition.radius,
      (motion.support_flags&1u)==0,unit.definition.maximum_slope_cosine};
  unit.crouch=motion.crouch;unit.source_parent=motion.parent;unit.source_seat=motion.seat;
  unit.source_action_word=motion.action_word;unit.source_unit_10a=motion.unit_flags;
  unit.grenade_action=objects_->grenade_action(unit.source_datum);
  const auto vitality=objects_->object_vitality(unit.source_datum);
  unit.vitality.body=vitality.body;unit.vitality.shield=vitality.shield;unit.damage_feedback=vitality.feedback;
  unit.vitality.dead=(motion.unit_flags&4u)!=0;
  const auto previous_zoom=unit.source_zoom;
  unit.source_zoom.current=read<std::int8_t>(memory,body,0x240);
  unit.source_zoom.requested=read<std::int8_t>(memory,body,0x241);
  unit.source_zoom.motion=read<std::uint32_t>(memory,body,0x26c);
  if(previous_zoom.requested!=-1 && unit.source_zoom.requested==-1)++unit.zoom_reset_serial;
  unit.source_membership=read<std::int16_t>(memory,body,0x138);
  unit.team=unit.source_membership;
  const auto source=objects_->player_equipment(unit.source_datum);
  std::array<std::optional<WeaponState>,4> weapons;
  for(unsigned slot=0;slot<4;++slot)if(source.weapons[slot]!=null_datum) {
    const auto handle=source.weapons[slot];
    if(!lifetimes.contains(handle))lifetimes.observe_source_birth(handle);
    const auto definition=content.weapons.find(source.definitions[slot]);
    if(definition==content.weapons.end())throw FormatError("Actual campaign equipment was not loaded from the cache");
    auto &state=weapons[slot].emplace(native_weapon_state(definition->second,handle));
    const auto weapon=objects_->object_body(handle);
    state.source_barrels=read<decltype(state.source_barrels)>(memory,weapon,0x1a4);
    state.source_triggers=read<decltype(state.source_triggers)>(memory,weapon,0x20c);
    state.source_magazines=read<decltype(state.source_magazines)>(memory,weapon,0x224);
    state.source_runtime_published=true;state.shader=source.shaders[slot];state.ready_clock=false;
    state.smg.input_flags=read<std::uint16_t>(memory,weapon,0x16e);
    state.charge_fraction=read<std::uint8_t>(memory,weapon,0x175);
    state.smg.trigger_state=state.source_triggers[0].mode;state.smg.trigger_ticks=state.source_triggers[0].ticks;
    state.loaded=state.reserve=0;
    if(!definition->second.triggers.empty()) {
      const auto barrel=definition->second.triggers[0].primary_barrel;
      if(barrel>=0) {
        if(unsigned(barrel)>=state.source_barrels.size() || unsigned(barrel)>=definition->second.barrel_ammunition.size())
          throw FormatError("Campaign primary barrel escapes its original fixed bank");
        state.smg.barrel=state.source_barrels[unsigned(barrel)];
        const auto magazine=definition->second.barrel_ammunition[unsigned(barrel)].magazine;
        if(magazine>=0) {
          if(unsigned(magazine)>=state.source_magazines.size())throw FormatError("Campaign magazine escapes its original fixed bank");
          state.magazine=state.source_magazines[unsigned(magazine)];
          state.loaded=state.magazine.loaded;state.reserve=state.magazine.reserve;
          state.smg.magazine_mode=state.magazine.mode;
        }
      }
    }
  }
  const auto primary=source.current[0];
  if(primary>=4 || (primary>=0 && !weapons[unsigned(primary)]))throw FormatError("Campaign source hand has no actual weapon");
  unit.weapon=primary<0?WeaponState{}:std::move(*weapons[unsigned(primary)]);
  if(primary>=0)weapons[unsigned(primary)].reset();
  unit.source_inventory={source.weapons,source.current,source.requested,source.inventory_sequence,
      source.replicated_sequence,source.ready_ticks,std::move(weapons)};
  unit.grenades=source.grenades;unit.grenade_current=objects_->current_grenade(unit.source_datum);
}
void CampaignSourceRuntime::equip(Simulation &simulation,std::uint32_t definition) {
  if(objects_->tu15_world().equip_weapon(simulation.player_.source_datum,definition)==null_datum)
    throw FormatError("Original campaign equipment owner rejected the selected weapon");
  adopt(simulation);
}
void CampaignSourceRuntime::adopt(Simulation &simulation) {
  publish_source_scene_physics(simulation.scene_,objects_->memory(),objects_->source_object_pool());
  simulation.object_lifetimes_.observe_source_retirements();
  DatumPool players(objects_->memory(),objects_->player_pool());
  const auto record=std::as_const(players).lookup(player_);
  if(record.empty())throw FormatError("Campaign presentation lost its actual source player identity");
  const auto current=Bytes(std::as_bytes(record)).u32(0x2c);
  if(current!=null_datum) {
    if(current!=simulation.player_.source_datum) {
      const auto zoom_reset=simulation.player_.zoom_reset_serial+1u;
      simulation.player_={};simulation.player_.definition=simulation.content_.player;
      simulation.player_.source_datum=current;simulation.player_.source_player=player_;
      simulation.player_.zoom_reset_serial=zoom_reset;simulation.player_.bsp=int(simulation.bsp_);
      if(!simulation.object_lifetimes_.contains(current))simulation.object_lifetimes_.observe_source_birth(current);
    }
    adopt_unit(simulation.player_,simulation.content_,simulation.object_lifetimes_);
  } else {
    // The player has no current source unit. Retire the native view selector
    // without modifying the retained corpse or its source inventory.
    if(!simulation.player_.vitality.dead) {
      source_zoom_reset(simulation.player_.source_zoom);++simulation.player_.zoom_reset_serial;
    }
    simulation.player_.vitality.dead=true;
  }
  std::set<std::uint32_t> live;
  for(const auto &object:objects_->live_objects())if(object.type==0 && object.object!=simulation.player_.source_datum) {
    // Unit+12C is the actor binding; +13C is its independent player binding.
    if(read<std::uint32_t>(objects_->memory(),objects_->object_body(object.object),0x12c)==null_datum)continue;
    const auto character=std::ranges::find_if(simulation.content_.characters,[&](const auto &c){return c.second.biped.datum==object.definition;});
    if(character==simulation.content_.characters.end())continue;
    auto found=std::ranges::find_if(simulation.actors_,[&](const auto &u){return u.source_datum==object.object;});
    if(found==simulation.actors_.end()) {
      Unit unit;unit.source_datum=object.object;unit.definition=character->second.biped;unit.character=character->first;
      unit.bsp=int(simulation.bsp_);simulation.actors_.push_back(std::move(unit));found=std::prev(simulation.actors_.end());
      if(!simulation.object_lifetimes_.contains(object.object))simulation.object_lifetimes_.observe_source_birth(object.object);
    }
    live.insert(object.object);adopt_unit(*found,simulation.content_,simulation.object_lifetimes_);
  }
  for(auto &actor:simulation.actors_)if(!live.contains(actor.source_datum))actor.exists=false;
  simulation.campaign_weapon_pickups_.clear();
  for(const auto &object:objects_->live_objects())if(object.type==2) {
    const auto definition=simulation.content_.weapons.find(object.definition);
    if(definition==simulation.content_.weapons.end() || definition->second.first_person_model==null_datum)continue;
    const auto placed=std::ranges::find_if(simulation.scene_.objects(),[&](const auto &o){return o.handle==object.object;});
    const bool scene_owned=placed!=simulation.scene_.objects().end();
    const bool visible=object.parent==null_datum;
    if(scene_owned)simulation.scene_.get(object.object).inventory_attached=!visible;
    if(!visible)continue;
    const auto status=objects_->tu15_world().weapon_hud_state(object.object);
    const auto transform=objects_->object_transform(object.object);
    auto weapon=native_weapon_state(definition->second,object.object);
    weapon.loaded=status.loaded;weapon.reserve=status.reserve;
    const auto body=objects_->object_body(object.object);
    weapon.shader={read<std::uint32_t>(objects_->memory(),body,0x12c),read<std::uint32_t>(objects_->memory(),body,0x16c),
        read<float>(objects_->memory(),body,0x184),read<float>(objects_->memory(),body,0x180)};
    simulation.campaign_weapon_pickups_.push_back({object.object,object.definition,0,transform.position,
        transform.basis[0],transform.basis[2],std::move(weapon),true,scene_owned,int(simulation.bsp_)});
  }
}
void CampaignSourceRuntime::place_ai(Simulation &simulation,std::uint32_t reference,
    std::optional<std::int16_t> count) {
  if(count)objects_->tu15_world().place_ai(reference,*count);
  else objects_->tu15_world().place_ai(reference);
  adopt(simulation);
}
std::size_t CampaignSourceRuntime::place_squad(Simulation &simulation,std::size_t squad) {
  const auto before=std::ranges::count_if(simulation.actors_,[](const auto &u){return u.exists;});
  place_ai(simulation,std::uint32_t(squad));
  const auto after=std::ranges::count_if(simulation.actors_,[](const auto &u){return u.exists;});
  return std::size_t(std::max<std::ptrdiff_t>(0,after-before));
}
void CampaignSourceRuntime::deliver_controls(Simulation &simulation,const Controls &input) {
  auto &unit=simulation.player_;std::array<std::byte,0x5c> command;
  initialize_source_neutral_command(command);
  const auto put=[&](unsigned offset,std::uint32_t value,unsigned n=4) {
    for(unsigned i=0;i<n;++i)command[offset+i]=std::byte(value>>(8*i));
  };
  const auto held=simulation.content_.weapons.find(unit.weapon.definition);
  const bool sword=held!=simulation.content_.weapons.end() && (held->second.inventory_flags&(1u<<24));
  put(0,unsigned(input.crouch)|(unsigned(input.jump)<<1)|(input.grenade?0x04000000u:0u)|
      (input.melee?0x20u:0u)|((sword && input.fire)?0x40u:0u)|(input.fire?0x10000u:0u)|
      (input.fire_secondary?0x220000u:0u)|(input.reload?0x10000000u:0u));
  put(4,std::bit_cast<std::uint32_t>(input.yaw));put(8,std::bit_cast<std::uint32_t>(input.pitch));
  put(0xc,std::bit_cast<std::uint32_t>(input.forward));put(0x10,std::bit_cast<std::uint32_t>(-input.right));
  put(0x14,std::bit_cast<std::uint32_t>(input.fire?1.f:0.f));put(0x18,std::bit_cast<std::uint32_t>(input.fire_secondary?1.f:0.f));
  put(0x1c,source_pickup_buttons(input.pickup,input.pickup_milliseconds,input.dual_wield,
      input.dual_wield_milliseconds,source_pickup_hold_threshold(simulation.content_.pickup_hold_seconds)),2);
  put(0x22,std::uint16_t(grenade_ready_type(unit.grenade_current,unit.grenades,input.switch_grenade?1:0)),2);
  put(0x24,std::uint16_t(input.zoom_reset_serial==unit.zoom_reset_serial?input.zoom:std::int16_t(-1)),2);
  const auto target=input.vehicle_interaction_target.value_or(simulation.action_prompt().target);
  if(target.object!=null_datum) {
    put(0x28,std::uint16_t(target.action)|(std::uint32_t(std::uint16_t(target.index))<<16));put(0x2c,target.object);
  }
  if(unit.exists && !unit.vitality.dead) {
    objects_->tu15_world().populate_weapon_selection(unit.source_datum,command,input.switch_weapon);
    objects_->prepare_player_look(player_,unit.source_datum,command);
    objects_->populate_weapon_targeting(player_,unit.source_datum,command);
    objects_->deliver_command(player_,unit.source_datum,command);
  }
}
void CampaignSourceRuntime::step(Simulation &simulation,const Controls &input) {
  deliver_controls(simulation,input);
  objects_->step_objects();adopt(simulation);
}
MultiplayerState::Player CampaignSourceRuntime::hud_player(const Simulation &simulation) {
  auto result=native_weapon_hud_player(simulation.player_,simulation.content_);
  // Zoom/action admission consumes the separate component at unit+346,
  // not the unit+134 control/camouflage word retained by the native view.
  result.source_action_word=simulation.player_.grenade_action.active_actions();
  result.source_player=player_;
  DatumPool players(objects_->memory(),objects_->player_pool());
  const auto record=std::as_const(players).lookup(player_);
  if(record.empty())throw FormatError("Campaign HUD lost its actual player record");
  const Bytes raw(std::as_bytes(record));
  result.source_flags=raw.u16(2);result.source_unit=raw.u32(0x2c);
  result.source_previous_unit=raw.u32(0x30);
  if(const auto look=objects_->player_look(player_,result.source_unit)) {
    result.yaw=look->x;result.pitch=look->y;
  }
  SourcePlayerConfiguration configuration;
  std::memcpy(configuration.data(),record.data()+0x44,configuration.size());
  result.source_configuration=configuration;
  const auto unit=simulation.player_.source_datum;
  if(!result.connected || result.dead || unit==null_datum)return result;
  result.source_parent=simulation.player_.source_parent;result.source_seat=simulation.player_.source_seat;
  result.source_hud_target_flags=objects_->weapon_hud_target_flags(unit);
  result.source_hud_autoaimed=read<float>(objects_->memory(),objects_->object_body(unit),0x1e4)>=1.f;
  result.source_vehicle_hud=objects_->vehicle_weapon_hud(unit);
  const auto publish=[&](std::uint32_t weapon,NativeWeaponTriggerPublication &out) {
    if(weapon==null_datum)return;
    const auto target=objects_->weapon_hud_target(weapon);out.target_acquired=target.acquired;
    if(target.position)out.target_position=std::array{target.position->x,target.position->y,target.position->z};
  };
  publish(result.source_weapon,result.weapon_trigger);publish(result.secondary_weapon.datum,result.secondary_weapon.trigger);
  return result;
}
}
