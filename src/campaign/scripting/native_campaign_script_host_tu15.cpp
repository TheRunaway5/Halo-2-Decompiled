#include "halo2/native_campaign_script_host_tu15.hpp"
#include "halo2/native_object_factory_deviceanim_tu15.hpp"
#include "halo2/native_object_factory_services_tu15.hpp"
#include "halo2/cache.hpp"
#include "halo2/kernels.hpp"
#include <array>
#include <map>

namespace halo2 {
namespace {
namespace k=native_campaign_script_tu15;
using Arguments=std::vector<std::uint32_t>;
// World memory view for natives written over SourcePhysicsContactMemory:
// reads go to the world, stores through the bank's journaled writer.
struct BankWorld final:SourcePhysicsContactMemory {
  NativeCampaignScriptBank& bank;
  explicit BankWorld(NativeCampaignScriptBank& b):bank(b){}
  void read(std::uint32_t address,std::span<std::byte> output)const override{bank.memory().read(address,output);}
  void write(std::uint32_t address,std::span<const std::byte> input)override{bank.store(address,input);}
};
struct Context {
  NativeCampaignScriptBank& bank;
  const Arguments& a;
  const NativeCampaignScriptHostServices& services;
  std::vector<std::uint32_t> members(std::size_t n)const {
    if(!services.list_members)throw FormatError("Native campaign list builtin requires the HS list bank");
    auto result=services.list_members(a.at(n));
    for(auto& member:result)if(member==0)member=services.player;
    return result;
  }
  std::uint8_t boolean(std::size_t n)const{return std::uint8_t(a.at(n));}
  std::uint32_t volume_list(std::size_t n,std::uint32_t mask)const {
    if(!services.list_allocate || !services.list_append)
      throw FormatError("Native volume list builtin requires the HS list bank");
    const auto index=volume(n);
    const auto list=services.list_allocate();
    for(const auto object:k::volume_return_objects(bank,index,mask))services.list_append(list,object);
    return list;
  }
  // Authored trigger index; the scenario block (+108 count) bounds it, as the
  // previous host did, instead of reading past the original block.
  std::int16_t volume(std::size_t n)const {
    const auto index=std::int16_t(a.at(n));
    const auto count=std::int32_t(bank.u32(bank.u32(0x4ecc20u)+0x108u));
    if(index<0 || index>=count)throw FormatError("invalid trigger volume");
    return index;
  }
};
using Handler=std::function<std::optional<std::uint32_t>(const Context&)>;
struct Builtin {
  std::vector<std::uint16_t> types;
  std::uint16_t result=4;
  bool floating_point=false;
  Handler run;
};
std::optional<std::uint32_t> none(){return std::nullopt;}
const std::map<std::string_view,Builtin>& table() {
  static const std::map<std::string_view,Builtin> builtins{
    {"pin",{{6,6,6},6,true,[](const Context& c){
      return std::optional(k::pin(c.a[0],c.a[1],c.a[2],c.bank.floating_point().mxcsr));}}},
    {"random_range",{{7,7},7,false,[](const Context& c){
      return std::optional<std::uint32_t>(k::random_range(c.bank,std::uint16_t(c.a[0]),std::uint16_t(c.a[1])));}}},
    {"real_random_range",{{6,6},6,true,[](const Context& c){return std::optional(k::real_random_range(c.bank,c.a[0],c.a[1]));}}},
    {"garbage_collect_now",{{},4,false,[](const Context& c){k::garbage_collect(c.bank,false);return none();}}},
    {"garbage_collect_unsafe",{{},4,false,[](const Context& c){k::garbage_collect(c.bank,true);return none();}}},
    {"physics_set_gravity",{{6},4,true,[](const Context& c){k::physics_set_gravity(c.bank,c.a[0]);return none();}}},
    {"physics_set_velocity_frame",{{6,6,6},4,false,[](const Context& c){
      k::physics_set_velocity_frame(c.bank,c.a[0],c.a[1],c.a[2]);return none();}}},
    {"physics_disable_character_ground_adhesion_forces",{{6},4,true,[](const Context& c){
      k::physics_disable_character_ground_adhesion_forces(c.bank,c.a[0]);return none();}}},
    {"player_effect_set_max_rotation",{{6,6,6},4,true,[](const Context& c){
      k::player_effect_set_max_rotation(c.bank,c.a[0],c.a[1],c.a[2]);return none();}}},
    {"player_effect_start",{{6,6},4,true,[](const Context& c){k::player_effect_start(c.bank,c.a[0],c.a[1]);return none();}}},
    {"player_effect_stop",{{6},4,true,[](const Context& c){k::player_effect_stop(c.bank,c.a[0]);return none();}}},
    {"pvs_set_object",{{50},4,false,[](const Context& c){k::pvs_set_object(c.bank,c.a[0]);return none();}}},
    {"pvs_clear",{{},4,false,[](const Context& c){k::pvs_clear(c.bank);return none();}}},
    {"object_get_parent",{{50},50,false,[](const Context& c){return std::optional(k::object_get_parent(c.bank,c.a[0]));}}},
    {"ai_play_line_on_object",{{50,11},6,false,[](const Context& c){
      if(!c.services.play_line_on_object)throw FormatError("ai_play_line_on_object requires its retained speech World");
      return std::optional(c.services.play_line_on_object(c.a[0],c.a[1]));}}},
    {"object_set_phantom_power",{{50,5},4,false,[](const Context& c){
      k::object_set_phantom_power(c.bank,c.a[0],c.boolean(1));return none();}}},
    {"object_set_deleted_when_deactivated",{{50},4,false,[](const Context& c){
      k::object_set_deleted_when_deactivated(c.bank,c.a[0]);return none();}}},
    {"object_set_shield_stun_infinite",{{50},4,false,[](const Context& c){
      k::object_set_shield_stun_infinite(c.bank,c.a[0]);return none();}}},
    {"object_get_shield",{{50},6,false,[](const Context& c){return std::optional(k::object_get_shield(c.bank,c.a[0]));}}},
    // 2AB7A0 and 2AB730 serve the object and unit names alike.
    {"unit_get_shield",{{51},6,false,[](const Context& c){return std::optional(k::object_get_shield(c.bank,c.a[0]));}}},
    {"object_set_shield",{{50,6},4,true,[](const Context& c){k::object_set_shield(c.bank,c.a[0],c.a[1]);return none();}}},
    {"object_get_health",{{50},6,false,[](const Context& c){return std::optional(k::object_get_health(c.bank,c.a[0]));}}},
    {"unit_get_health",{{51},6,false,[](const Context& c){return std::optional(k::object_get_health(c.bank,c.a[0]));}}},
    {"object_clear_all_function_variables",{{50},4,false,[](const Context& c){
      k::object_clear_all_function_variables(c.bank,c.a[0]);return none();}}},
    {"unit_kill",{{51},4,false,[](const Context& c){k::unit_kill(c.bank,c.a[0],false);return none();}}},
    {"unit_kill_silent",{{51},4,false,[](const Context& c){k::unit_kill(c.bank,c.a[0],true);return none();}}},
    {"device_operates_automatically_set",{{54,5},4,false,[](const Context& c){
      k::device_operates_automatically_set(c.bank,c.a[0],c.boolean(1));return none();}}},
    {"device_closes_automatically_set",{{54,5},4,false,[](const Context& c){
      k::device_closes_automatically_set(c.bank,c.a[0],c.boolean(1));return none();}}},
    {"device_one_sided_set",{{54,5},4,false,[](const Context& c){
      k::device_one_sided_set(c.bank,c.a[0],c.boolean(1));return none();}}},
    {"device_set_never_appears_locked",{{54,5},4,false,[](const Context& c){
      k::device_set_never_appears_locked(c.bank,c.a[0],c.boolean(1));return none();}}},
    {"device_group_change_only_once_more_set",{{18,5},4,false,[](const Context& c){
      k::device_group_change_only_once_more_set(c.bank,c.a[0],c.boolean(1));return none();}}},
    {"device_get_position",{{54},6,false,[](const Context& c){return std::optional(k::device_get_position(c.bank,c.a[0]));}}},
    {"device_group_get",{{18},6,false,[](const Context& c){return std::optional(k::device_group_get(c.bank,c.a[0]));}}},
    {"device_group_set_immediate",{{18,6},4,true,[](const Context& c){
      k::device_group_set_immediate(c.bank,c.a[0],c.a[1]);return none();}}},
    {"device_set_position_immediate",{{54,6},4,true,[](const Context& c){
      k::device_set_position_immediate(c.bank,c.a[0],c.a[1]);return none();}}},
    {"unit_in_vehicle",{{51},5,false,[](const Context& c){return std::optional<std::uint32_t>(k::unit_in_vehicle(c.bank,c.a[0]));}}},
    {"unit_only_takes_damage_from_players_team",{{51,5},4,false,[](const Context& c){
      k::unit_only_takes_damage_from_players_team(c.bank,c.a[0],c.boolean(1));return none();}}},
    {"unit_doesnt_drop_items",{{31},4,false,[](const Context& c){
      const auto members=c.members(0);k::unit_doesnt_drop_items(c.bank,members);return none();}}},
    {"unit_is_emitting",{{51},5,false,[](const Context& c){return std::optional<std::uint32_t>(k::unit_is_emitting(c.bank,c.a[0]));}}},
    {"vehicle_driver",{{51},51,false,[](const Context& c){return std::optional(k::vehicle_driver(c.bank,c.a[0]));}}},
    {"list_count_not_dead",{{31},7,false,[](const Context& c){
      const auto members=c.members(0);return std::optional<std::uint32_t>(k::list_count_not_dead(c.bank,members));}}},
    // Trigger volumes over the live world's object records (TU1.5 2AAB50,
    // 2AABD0, 2AAC30 -> 11EBA0/2A93D0).
    {"volume_test_object",{{13,50},5,true,[](const Context& c){
      return std::optional<std::uint32_t>(k::volume_test_object(c.bank,c.volume(0),c.a[1]));}}},
    {"volume_test_objects",{{13,31},5,true,[](const Context& c){
      const auto members=c.members(1);return std::optional<std::uint32_t>(k::volume_test_objects(c.bank,c.volume(0),members,false));}}},
    {"volume_test_objects_all",{{13,31},5,true,[](const Context& c){
      const auto members=c.members(1);return std::optional<std::uint32_t>(k::volume_test_objects(c.bank,c.volume(0),members,true));}}},
    // AI references over the actor/squad/encounter-group banks
    // (27CAA0/27CB30 walks, 27C880 resolution).
    {"ai_vehicle_get_from_starting_location",{{19},52,false,[](const Context& c){
      return std::optional(k::ai_vehicle_get_from_starting_location(c.bank,c.a[0]));}}},
    {"ai_vehicle_get",{{19},52,false,[](const Context& c){return std::optional(k::ai_vehicle_get(c.bank,c.a[0]));}}},
    {"object_get_ai",{{50},19,false,[](const Context& c){return std::optional(k::object_get_ai(c.bank,c.a[0]));}}},
    {"ai_spawn_count",{{19},7,false,[](const Context& c){return std::optional<std::uint32_t>(k::ai_spawn_count(c.bank,c.a[0]));}}},
    {"ai_combat_status",{{19},7,false,[](const Context& c){return std::optional<std::uint32_t>(k::ai_combat_status(c.bank,c.a[0]));}}},
    {"ai_vitality_pinned",{{19},5,true,[](const Context& c){return std::optional<std::uint32_t>(k::ai_vitality_pinned(c.bank,c.a[0]));}}},
    {"ai_renew",{{19},4,true,[](const Context& c){k::ai_renew(c.bank,c.a[0]);return none();}}},
    // CAF20 seat trees (11D240, 11D310, 27FCD0, 27FBC0).
    {"vehicle_test_seat",{{52,11,51},5,false,[](const Context& c){
      return std::optional<std::uint32_t>(k::vehicle_test_seat(c.bank,c.a[0],c.a[1],c.a[2]));}}},
    {"vehicle_test_seat_list",{{52,11,31},5,false,[](const Context& c){
      const auto members=c.members(2);return std::optional<std::uint32_t>(k::vehicle_test_seat_list(c.bank,c.a[0],c.a[1],members));}}},
    {"ai_vehicle_reserve",{{52,5},5,false,[](const Context& c){
      return std::optional<std::uint32_t>(k::ai_vehicle_reserve(c.bank,c.a[0],c.boolean(1)));}}},
    {"ai_vehicle_reserve_seat",{{52,11,5},5,false,[](const Context& c){
      return std::optional<std::uint32_t>(k::ai_vehicle_reserve_seat(c.bank,c.a[0],c.a[1],c.boolean(2)));}}},
    {"weapon_hold_trigger",{{53,8,5},4,false,[](const Context& c){k::weapon_hold_trigger(c.bank,c.a[0],c.a[1],c.boolean(2));return none();}}},
    // 25B6AF/25B77A team nav points: kind 1 names an object, kind 0 a flag.
    {"activate_team_nav_point_object",{{27,45,50,6},4,false,[](const Context& c){
      k::team_nav_point(c.bank,true,std::uint16_t(c.a[0]),std::uint16_t(c.a[1]),1,c.a[2],c.a[3]);return none();}}},
    {"activate_team_nav_point_flag",{{27,45,14,6},4,false,[](const Context& c){
      k::team_nav_point(c.bank,true,std::uint16_t(c.a[0]),std::uint16_t(c.a[1]),0,
          std::uint32_t(std::int32_t(std::int16_t(c.a[2]))),c.a[3]);return none();}}},
    {"deactivate_team_nav_point_object",{{45,50},4,false,[](const Context& c){
      k::team_nav_point(c.bank,false,0,std::uint16_t(c.a[0]),1,c.a[1],0);return none();}}},
    {"deactivate_team_nav_point_flag",{{45,14},4,false,[](const Context& c){
      k::team_nav_point(c.bank,false,0,std::uint16_t(c.a[0]),0,std::uint32_t(std::int32_t(std::int16_t(c.a[1]))),0);return none();}}},
    {"unit_set_active_camo",{{51,5,6},4,true,[](const Context& c){k::unit_set_active_camo(c.bank,c.a[0],c.boolean(1),c.a[2]);return none();}}},
    {"biped_ragdoll",{{51},4,false,[](const Context& c){k::biped_ragdoll(c.bank,c.a[0]);return none();}}},
    {"units_set_maximum_vitality",{{31,6,6},4,true,[](const Context& c){
      const auto objects=c.members(0);k::units_set_maximum_vitality(c.bank,objects,c.a[1],c.a[2]);return none();}}},
    {"player_flashlight_on",{{},5,true,[](const Context& c){return std::optional<std::uint32_t>(k::player_flashlight_on(c.bank));}}},
    {"ice_cream_flavor_available",{{8},5,false,[](const Context& c){return std::optional<std::uint32_t>(k::ice_cream_flavor_available(c.bank,c.a[0]));}}},
    {"ai_set_active_camo",{{19,5},4,false,[](const Context& c){k::ai_set_active_camo(c.bank,c.a[0],c.boolean(1));return none();}}},
    {"ai_berserk",{{19,5},4,false,[](const Context& c){k::ai_berserk(c.bank,c.a[0],c.boolean(1));return none();}}},
    {"flock_start",{{11},5,false,[](const Context& c){return std::optional<std::uint32_t>(k::flock_set_active(c.bank,c.a[0],true));}}},
    {"flock_stop",{{11},5,false,[](const Context& c){return std::optional<std::uint32_t>(k::flock_set_active(c.bank,c.a[0],false));}}},
    {"ai_magically_see",{{19,19},4,true,[](const Context& c){k::ai_magically_see(c.bank,c.a[0],c.a[1]);return none();}}},
    {"object_model_targets_destroyed",{{50,11},7,true,[](const Context& c){
      return std::optional<std::uint32_t>(k::object_model_targets_destroyed(c.bank,c.a[0],c.a[1]));}}},
    {"object_at_marker",{{50,11},50,true,[](const Context& c){return std::optional(k::object_at_marker(c.bank,c.a[0],c.a[1]));}}},
    {"ai_disposable",{{19,5},4,false,[](const Context& c){k::ai_disposable(c.bank,c.a[0],c.boolean(1));return none();}}},
    {"ai_set_blind",{{19,5},4,false,[](const Context& c){k::ai_set_blind(c.bank,c.a[0],c.boolean(1));return none();}}},
    {"ai_set_deaf",{{19,5},4,false,[](const Context& c){k::ai_set_deaf(c.bank,c.a[0],c.boolean(1));return none();}}},
    {"ai_suppress_combat",{{19,5},4,false,[](const Context& c){k::ai_suppress_combat(c.bank,c.a[0],c.boolean(1));return none();}}},
    {"ai_magically_see_object",{{19,50},4,true,[](const Context& c){
      k::ai_magically_see_object(c.bank,c.a[0],c.a[1]);return none();}}},
    {"ai_disregard",{{31,5},4,false,[](const Context& c){
      const auto objects=c.members(0);k::ai_target_preference(c.bank,objects,c.boolean(1),false);return none();}}},
    {"ai_prefer_target",{{31,5},4,false,[](const Context& c){
      const auto objects=c.members(0);k::ai_target_preference(c.bank,objects,c.boolean(1),true);return none();}}},
    // View cones from each unit's head marker (2A9680, 2A9560 -> CE790).
    {"objects_can_see_flag",{{31,14,6},5,true,[](const Context& c){
      const auto viewers=c.members(0);
      return std::optional<std::uint32_t>(k::objects_can_see_flag(c.bank,viewers,std::int16_t(c.a[1]),c.a[2]));}}},
    {"objects_can_see_object",{{31,50,6},5,true,[](const Context& c){
      const auto viewers=c.members(0);
      return std::optional<std::uint32_t>(k::objects_can_see_object(c.bank,viewers,c.a[1],c.a[2]));}}},
    // 11ED20: allocate first (1E7010), then append qualifying objects.
    {"volume_return_objects",{{13},31,true,[](const Context& c){return std::optional(c.volume_list(0,k::none));}}},
    {"volume_return_objects_by_type",{{13,8},31,true,[](const Context& c){return std::optional(c.volume_list(0,c.a[1]));}}},
    {"device_animate_position",{{54,6,6,6,6,5},4,true,[](const Context& c){
      k::device_animate_position(c.bank,c.a[0],c.a[1],c.a[2],c.a[3],c.a[4],c.boolean(5));return none();}}},
    // 10D5E0 / 10CF80: native device animation tracks (track deviceanim).
    // Stores reach the world through the bank's journaled writer.
    {"device_set_overlay_track",{{54,11},5,true,[](const Context& c){
      BankWorld world(c.bank);
      return std::optional<std::uint32_t>(native_object_factory_tu15::device_animation::device_set_overlay_track(
          world,c.a[0],c.a[1],c.bank.floating_point()));}}},
    {"device_set_position_track",{{54,11,6},5,true,[](const Context& c){
      BankWorld world(c.bank);
      return std::optional<std::uint32_t>(native_object_factory_tu15::device_animation::device_set_position_track(
          world,c.a[0],c.a[1],c.a[2],c.bank.floating_point()));}}},
    // 2ADE60 / 2ADC30 / 2AFE00: vehicle exits through the unitactions track's
    // 11D990 / 11D670 / 27EAB0 (the services track's evaluator tails;
    // ai_vehicle_exit walks its reference with 27CAA0/27CB30). Stores go
    // through the bank's journaled writer.
    {"vehicle_unload",{{50,12},7,true,[](const Context& c){
      return std::optional<std::uint32_t>(native_object_factory_tu15::script_vehicle_unload(c.bank,c.a[0],c.a[1]));}}},
    {"unit_exit_vehicle",{{51,7},4,true,[](const Context& c){
      native_object_factory_tu15::script_unit_exit_vehicle(c.bank,c.a[0],std::uint16_t(c.a[1]));return none();}}},
    {"ai_vehicle_exit",{{19},4,true,[](const Context& c){
      native_object_factory_tu15::script_ai_vehicle_exit(c.bank,c.a[0]);return none();}}},
    {"device_animate_overlay",{{54,6,6,6,6},4,true,[](const Context& c){
      k::device_animate_overlay(c.bank,c.a[0],c.a[1],c.a[2],c.a[3],c.a[4]);return none();}}},
    {"interpolator_start",{{11,6,6},8,true,[](const Context& c){
      return std::optional(k::interpolator_start(c.bank,c.a[0],c.a[1],c.a[2]));}}},
    {"cheat_active_camouflage_by_player",{{7,5},4,false,[](const Context&){return none();}}},
    {"time_code_show",{{5},4,false,[](const Context&){return none();}}},
    {"time_code_start",{{5},4,false,[](const Context&){return none();}}},
  };
  return builtins;
}
bool scalar(unsigned type){return type>=5 && type<=8;}
bool object(unsigned type){return type>=50 && type<=55;}
}

bool native_campaign_script_builtin_admitted_tu15(std::string_view name) {
  return table().contains(name);
}

std::optional<ScriptValue> native_campaign_script_builtin_tu15(std::string_view name,
    const std::vector<ScriptValue>& arguments,const NativeCampaignScriptHostServices& services) {
  const auto found=table().find(name);
  if(found==table().end())return std::nullopt;
  const auto& builtin=found->second;
  if(arguments.size()!=builtin.types.size())
    throw FormatError("invalid "+std::string(name)+" arity");
  if(!services.memory)throw FormatError(std::string(name)+" requires its retained TU1.5 campaign world");
  NativeCampaignScriptFloatingPoint floating_point{};
  if(builtin.floating_point) {
    if(!services.floating_point)throw FormatError(std::string(name)+" requires its world floating-point controls");
    floating_point=services.floating_point();
  }
  NativeCampaignScriptBank bank(*services.memory,services.writer,floating_point);
  Arguments values;values.reserve(arguments.size());
  for(std::size_t n=0;n<arguments.size();++n) {
    const auto declared=builtin.types[n];auto bits=arguments[n].bits;
    if(scalar(declared) && arguments[n].type!=declared) {
      if(!scalar(arguments[n].type) && arguments[n].type!=3)
        throw FormatError(std::string(name)+" argument lost its compiled scalar type");
      bits=scalar_cast_bits(bits,arguments[n].type,declared);
    }
    if(object(declared) && bits==0)bits=services.player;
    // Native script values keep authored group ordinals; original evaluators
    // receive the full generation resolved by 10C180.
    if(declared==18)bits=k::device_group_from_ordinal(bank,std::uint16_t(bits));
    values.push_back(bits);
  }
  const auto result=builtin.run(Context{bank,values,services});
  if(builtin.result==4)return ScriptValue{};
  return ScriptValue{builtin.result,result.value_or(0),{}};
}
}
