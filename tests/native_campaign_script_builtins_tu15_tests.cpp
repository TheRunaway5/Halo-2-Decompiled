// Native side of the physical original comparison for TU1.5 campaign HS
// builtins. Each stdin line declares input banks, one kernel and its already
// evaluated arguments; stdout reports ordered stores, result and final banks.
// With --self-test it runs fixed expectations derived from the original
// listings so the module is checked without the original executable.
#include "halo2/native_campaign_script_builtins_tu15.hpp"
#ifdef HALO2_NATIVE_SCRIPT_HOST_TESTS
#include "halo2/native_campaign_script_host_tu15.hpp"
#endif
#include "halo2/native_tu15_object_motion.hpp"
#include "halo2/native_object_factory_tu15.hpp"
#include "native_object_factory_test_registry.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

namespace {
using namespace halo2;
namespace k=native_campaign_script_tu15;
using J=nlohmann::json;

std::vector<std::byte> decode(const std::string& text) {
  std::vector<std::byte> out;
  for(std::size_t n=0;n<text.size();n+=2)out.push_back(std::byte(std::stoul(text.substr(n,2),nullptr,16)));
  return out;
}
std::string hex(std::span<const std::byte> bytes) {
  constexpr char digits[]="0123456789abcdef";std::string out;
  for(const auto value:bytes){const auto n=std::to_integer<unsigned>(value);out+=digits[n>>4];out+=digits[n&15];}
  return out;
}
struct Memory:SourcePhysicsContactMemory {
  struct Region {std::uint32_t address;std::vector<std::byte> bytes;};
  std::vector<Region> regions;J writes=J::array();
  std::span<std::byte> view(std::uint32_t address,std::size_t size) {
    for(auto& region:regions)
      if(address>=region.address && std::uint64_t(address-region.address)+size<=region.bytes.size())
        return std::span(region.bytes).subspan(address-region.address,size);
    throw std::runtime_error("Native script kernel left its declared banks");
  }
  void read(std::uint32_t address,std::span<std::byte> output)const override {
    const auto bytes=const_cast<Memory*>(this)->view(address,output.size());
    std::ranges::copy(bytes,output.begin());
  }
  void write(std::uint32_t address,std::span<const std::byte> input)override {
    std::ranges::copy(input,view(address,input.size()).begin());
    writes.push_back(J::array({address,hex(input)}));
  }
};
std::vector<std::uint32_t> words(const J& row,const char* key) {
  std::vector<std::uint32_t> out;
  if(row.contains(key))for(const auto& value:row.at(key))out.push_back(value.get<std::uint32_t>());
  return out;
}
std::uint32_t execute(NativeCampaignScriptBank& bank,const std::string& kernel,const std::vector<std::uint32_t>& a,
    const std::vector<std::uint32_t>& members) {
  auto arg=[&](std::size_t n){if(n>=a.size())throw std::invalid_argument("Missing declared argument");return a[n];};
  auto boolean=[&](std::size_t n){return std::uint8_t(arg(n));};
  // Object factory owners (B9B70 leaves) run on the raw world memory: their
  // stores are the routine's own, published in original order.
  namespace f=native_object_factory_tu15;
  auto& world=const_cast<SourcePhysicsContactMemory&>(bank.memory());
  if(const auto track=native_object_factory_tests::kernels().find(kernel);track!=native_object_factory_tests::kernels().end())
    return track->second(world,bank,a);
  if(kernel=="object_memory_allocate")return f::memory_pool_allocate(world,arg(3),arg(0),arg(1),arg(4),arg(2));
  if(kernel=="object_memory_resize")return f::memory_pool_resize(world,arg(0),arg(4),arg(3),arg(2),arg(1),arg(5));
  if(kernel=="object_allocate")return f::object_allocate(world,std::int16_t(arg(0)));
  if(kernel=="object_block_allocate")
    return f::object_block_allocate(world,arg(3),std::int16_t(arg(0)),std::int16_t(arg(1)),std::uint8_t(arg(2)),arg(4));
  if(kernel=="object_model_tag_pair")return f::object_model_tag_pair(world,arg(2),arg(0),arg(1));
  if(kernel=="object_damage_info")return f::object_damage_info(world,arg(0));
  if(kernel=="object_root_header_bit6")return f::object_root_header_bit6(world,arg(0));
  if(kernel=="object_clear_block_12a"){f::object_clear_block_12a(world,arg(0));return 0;}
  if(kernel=="object_queue_definition_flagged"){f::object_queue_definition_flagged(world,arg(0));return 0;}
  if(kernel=="object_definition_physics_capacity")return f::object_definition_physics_capacity(world,arg(0));
  if(kernel=="object_memory_locked")return f::object_memory_locked(world);
  if(kernel=="havok_record_initialize"){f::havok_record_initialize(world,arg(0));return 0;}
  if(kernel=="havok_record_reset"){f::havok_record_reset(world,arg(0));return 0;}
  if(kernel=="havok_record_clear_links"){f::havok_record_clear_links(world,arg(0));return 0;}
  if(kernel=="object_free"){f::object_free(world,arg(0));return 0;}
  if(kernel=="definition_object_type")return f::definition_object_type(world,arg(0));
  if(kernel=="model_variant_index")return f::model_variant_index(world,arg(0),arg(1));
  if(kernel=="object_set_variant"){f::object_set_variant(world,arg(0),arg(1));return 0;}
  if(kernel=="object_node_matrices") {
    // The caller's count slot receives the original's single store.
    std::uint32_t count=0;const auto matrices=f::object_node_matrices(world,arg(1),count);
    bank.store32(arg(0),count);return matrices;
  }
  if(kernel=="object_retire_lens_queries"){f::object_retire_lens_queries(world,arg(0));return 0;}
  if(kernel=="pin")return k::pin(arg(0),arg(1),arg(2),bank.floating_point().mxcsr);
  if(kernel=="random_range")return k::random_range(bank,std::uint16_t(arg(0)),std::uint16_t(arg(1)));
  if(kernel=="real_random_range")return k::real_random_range(bank,arg(0),arg(1));
  if(kernel=="garbage_collect_now"){k::garbage_collect(bank,false);return 0;}
  if(kernel=="garbage_collect_unsafe"){k::garbage_collect(bank,true);return 0;}
  if(kernel=="physics_set_gravity"){k::physics_set_gravity(bank,arg(0));return 0;}
  if(kernel=="physics_set_velocity_frame"){k::physics_set_velocity_frame(bank,arg(0),arg(1),arg(2));return 0;}
  if(kernel=="physics_disable_character_ground_adhesion_forces"){k::physics_disable_character_ground_adhesion_forces(bank,arg(0));return 0;}
  if(kernel=="player_effect_set_max_rotation"){k::player_effect_set_max_rotation(bank,arg(0),arg(1),arg(2));return 0;}
  if(kernel=="player_effect_start"){k::player_effect_start(bank,arg(0),arg(1));return 0;}
  if(kernel=="player_effect_stop"){k::player_effect_stop(bank,arg(0));return 0;}
  if(kernel=="pvs_set_object"){k::pvs_set_object(bank,arg(0));return 0;}
  if(kernel=="pvs_clear"){k::pvs_clear(bank);return 0;}
  if(kernel=="object_get_parent")return k::object_get_parent(bank,arg(0));
  if(kernel=="object_set_phantom_power"){k::object_set_phantom_power(bank,arg(0),boolean(1));return 0;}
  if(kernel=="object_set_deleted_when_deactivated"){k::object_set_deleted_when_deactivated(bank,arg(0));return 0;}
  if(kernel=="object_set_shield_stun_infinite"){k::object_set_shield_stun_infinite(bank,arg(0));return 0;}
  if(kernel=="object_get_shield")return k::object_get_shield(bank,arg(0));
  if(kernel=="object_get_health")return k::object_get_health(bank,arg(0));
  if(kernel=="object_set_shield"){k::object_set_shield(bank,arg(0),arg(1));return 0;}
  if(kernel=="object_clear_all_function_variables"){k::object_clear_all_function_variables(bank,arg(0));return 0;}
  if(kernel=="unit_kill"){k::unit_kill(bank,arg(0),false);return 0;}
  if(kernel=="unit_kill_silent"){k::unit_kill(bank,arg(0),true);return 0;}
  if(kernel=="device_operates_automatically_set"){k::device_operates_automatically_set(bank,arg(0),boolean(1));return 0;}
  if(kernel=="device_closes_automatically_set"){k::device_closes_automatically_set(bank,arg(0),boolean(1));return 0;}
  if(kernel=="device_one_sided_set"){k::device_one_sided_set(bank,arg(0),boolean(1));return 0;}
  if(kernel=="device_set_never_appears_locked"){k::device_set_never_appears_locked(bank,arg(0),boolean(1));return 0;}
  if(kernel=="device_group_change_only_once_more_set"){k::device_group_change_only_once_more_set(bank,arg(0),boolean(1));return 0;}
  if(kernel=="device_get_position")return k::device_get_position(bank,arg(0));
  if(kernel=="device_group_get")return k::device_group_get(bank,arg(0));
  if(kernel=="device_group_set_immediate"){k::device_group_set_immediate(bank,arg(0),arg(1));return 0;}
  if(kernel=="device_set_position_immediate"){k::device_set_position_immediate(bank,arg(0),arg(1));return 0;}
  if(kernel=="unit_in_vehicle")return k::unit_in_vehicle(bank,arg(0));
  if(kernel=="unit_only_takes_damage_from_players_team"){k::unit_only_takes_damage_from_players_team(bank,arg(0),boolean(1));return 0;}
  if(kernel=="unit_doesnt_drop_items"){k::unit_doesnt_drop_items(bank,members);return 0;}
  if(kernel=="unit_is_emitting")return k::unit_is_emitting(bank,arg(0));
  if(kernel=="vehicle_driver")return k::vehicle_driver(bank,arg(0));
  if(kernel=="list_count_not_dead")return k::list_count_not_dead(bank,members);
  if(kernel=="object_cinematic_lod"){k::object_cinematic_lod(bank,arg(0),boolean(1));return 0;}
  if(kernel=="unit_set_enterable_by_player"){k::unit_set_enterable_by_player(bank,arg(0),boolean(1));return 0;}
  if(kernel=="object_clear_function_variable"){k::object_clear_function_variable(bank,arg(0),arg(1));return 0;}
  if(kernel=="texture_camera_off"){k::texture_camera_off(bank);return 0;}
  if(kernel=="cinematic_shadow_control"){k::cinematic_shadow_control(bank,boolean(0),arg(1),arg(2),arg(3));return 0;}
  if(kernel=="cinematic_depth_of_field") {
    const std::array<std::uint32_t,7> values{arg(0),arg(1),arg(2),arg(3),arg(4),arg(5),arg(6)};
    k::cinematic_depth_of_field(bank,values);return 0;
  }
  if(kernel=="camera_set_field_of_view"){k::camera_set_field_of_view(bank,arg(0),std::uint16_t(arg(1)));return 0;}
  if(kernel=="cinematic_stop"){k::cinematic_stop(bank);return 0;}
  if(kernel=="object_function_value") {
    // Original output ABI: slot=NONE first, then value and slot when found.
    bank.store32(arg(3),k::none);
    const auto found=k::object_function_value(bank,arg(0),arg(1));
    if(!found)return 0;
    bank.store32(arg(2),found->value);bank.store32(arg(3),found->slot);return 1;
  }
  if(kernel=="object_set_function_variable"){k::object_set_function_variable(bank,arg(0),arg(1),arg(2),arg(3));return 0;}
  if(kernel=="device_animate_position"){k::device_animate_position(bank,arg(0),arg(1),arg(2),arg(3),arg(4),boolean(5));return 0;}
  if(kernel=="device_animate_overlay"){k::device_animate_overlay(bank,arg(0),arg(1),arg(2),arg(3),arg(4));return 0;}
  if(kernel=="original_strncpy"){k::original_strncpy(bank,arg(0),arg(1),arg(2));return 0;}
  if(kernel=="texture_camera_set_object_marker"){k::texture_camera_set_object_marker(bank,arg(0),arg(1),arg(2));return 0;}
  if(kernel=="scenario_location") {
    // Original 11E600 output ABI at arg(1): NONE BSP writes cluster, BSP,
    // leaf; otherwise leaf, BSP, cluster.
    const std::array<std::uint32_t,3> point{bank.u32(arg(0)),bank.u32(arg(0)+4u),bank.u32(arg(0)+8u)};
    const auto location=k::scenario_location(bank,point);
    if(location.bsp==0xffffu){bank.store16(arg(1)+4u,0xffffu);bank.store16(arg(1)+6u,0xffffu);bank.store32(arg(1),k::none);return 0;}
    bank.store32(arg(1),location.leaf);bank.store16(arg(1)+6u,location.bsp);bank.store16(arg(1)+4u,location.cluster);
    return 0;
  }
  if(kernel=="objects_in_sphere") {
    const k::NativeCampaignLocation location{bank.u32(arg(2)),bank.u16(arg(2)+4u),bank.u16(arg(2)+6u)};
    const std::array<std::uint32_t,3> center{bank.u32(arg(3)),bank.u32(arg(3)+4u),bank.u32(arg(3)+8u)};
    return std::uint16_t(k::objects_in_sphere(bank,arg(0),arg(1),location,center,arg(4),std::int16_t(arg(6)),arg(5)).size());
  }
  if(kernel=="ai_vehicle_get_from_starting_location")return k::ai_vehicle_get_from_starting_location(bank,arg(0));
  if(kernel=="ai_vehicle_get")return k::ai_vehicle_get(bank,arg(0));
  if(kernel=="object_get_ai")return k::object_get_ai(bank,arg(0));
  if(kernel=="ai_spawn_count")return k::ai_spawn_count(bank,arg(0));
  if(kernel=="ai_combat_status")return k::ai_combat_status(bank,arg(0));
  if(kernel=="ai_disposable"){k::ai_disposable(bank,arg(0),boolean(1));return 0;}
  if(kernel=="ai_set_blind"){k::ai_set_blind(bank,arg(0),boolean(1));return 0;}
  if(kernel=="ai_set_deaf"){k::ai_set_deaf(bank,arg(0),boolean(1));return 0;}
  if(kernel=="ai_suppress_combat"){k::ai_suppress_combat(bank,arg(0),boolean(1));return 0;}
  if(kernel=="ai_magically_see_object"){k::ai_magically_see_object(bank,arg(0),arg(1));return 0;}
  if(kernel=="ai_vitality_pinned")return k::ai_vitality_pinned(bank,arg(0));
  if(kernel=="ai_renew"){k::ai_renew(bank,arg(0));return 0;}
  if(kernel=="ai_vehicle_reserve")return k::ai_vehicle_reserve(bank,arg(0),boolean(1));
  if(kernel=="ai_vehicle_reserve_seat")return k::ai_vehicle_reserve_seat(bank,arg(0),arg(1),boolean(2));
  if(kernel=="vehicle_test_seat")return k::vehicle_test_seat(bank,arg(0),arg(1),arg(2));
  if(kernel=="vehicle_test_seat_list")return k::vehicle_test_seat_list(bank,arg(0),arg(1),members);
  if(kernel=="object_set_velocity"){
    const auto& fp=bank.floating_point();
    native_tu15_object_set_velocity(const_cast<SourcePhysicsContactMemory&>(bank.memory()),arg(0),
        std::bit_cast<float>(arg(1)),std::bit_cast<float>(arg(2)),std::bit_cast<float>(arg(3)),{fp.x87_control_word,fp.mxcsr});
    return 0;
  }
  if(kernel=="player_profile_normalize"){k::player_profile_normalize(bank,arg(0));return 0;}
  if(kernel=="weapon_hold_trigger"){k::weapon_hold_trigger(bank,arg(0),arg(1),boolean(2));return 0;}
  if(kernel=="activate_team_nav_point_object"){k::team_nav_point(bank,true,std::uint16_t(arg(0)),std::uint16_t(arg(1)),1,arg(2),arg(3));return 0;}
  if(kernel=="activate_team_nav_point_flag"){k::team_nav_point(bank,true,std::uint16_t(arg(0)),std::uint16_t(arg(1)),0,
    std::uint32_t(std::int32_t(std::int16_t(arg(2)))),arg(3));return 0;}
  if(kernel=="deactivate_team_nav_point_object"){k::team_nav_point(bank,false,0,std::uint16_t(arg(0)),1,arg(1),0);return 0;}
  if(kernel=="deactivate_team_nav_point_flag"){k::team_nav_point(bank,false,0,std::uint16_t(arg(0)),0,
    std::uint32_t(std::int32_t(std::int16_t(arg(1)))),0);return 0;}
  if(kernel=="unit_set_active_camo"){k::unit_set_active_camo(bank,arg(0),boolean(1),arg(2));return 0;}
  if(kernel=="biped_ragdoll"){k::biped_ragdoll(bank,arg(0));return 0;}
  if(kernel=="units_set_maximum_vitality"){k::units_set_maximum_vitality(bank,members,arg(1),arg(2));return 0;}
  if(kernel=="player_flashlight_on")return k::player_flashlight_on(bank);
  if(kernel=="ice_cream_flavor_available")return k::ice_cream_flavor_available(bank,arg(0));
  if(kernel=="ai_set_active_camo"){k::ai_set_active_camo(bank,arg(0),boolean(1));return 0;}
  if(kernel=="ai_berserk"){k::ai_berserk(bank,arg(0),boolean(1));return 0;}
  if(kernel=="flock_start")return k::flock_set_active(bank,arg(0),true);
  if(kernel=="flock_stop")return k::flock_set_active(bank,arg(0),false);
  if(kernel=="ai_magically_see"){k::ai_magically_see(bank,arg(0),arg(1));return 0;}
  if(kernel=="object_model_targets_destroyed")return k::object_model_targets_destroyed(bank,arg(0),arg(1));
  if(kernel=="object_at_marker")return k::object_at_marker(bank,arg(0),arg(1));
  if(kernel=="sound_effect_reference")return k::sound_effect_reference(bank,arg(0));
  if(kernel=="ai_disregard"){k::ai_target_preference(bank,members,boolean(1),false);return 0;}
  if(kernel=="ai_prefer_target"){k::ai_target_preference(bank,members,boolean(1),true);return 0;}
  if(kernel=="objects_can_see_flag")return k::objects_can_see_flag(bank,members,std::int16_t(arg(1)),arg(2));
  if(kernel=="objects_can_see_object")return k::objects_can_see_object(bank,members,arg(1),arg(2));
  if(kernel=="volume_test_object")return k::volume_test_object(bank,std::int16_t(arg(0)),arg(1));
  if(kernel=="volume_test_objects")return k::volume_test_objects(bank,std::int16_t(arg(0)),members,false);
  if(kernel=="volume_test_objects_all")return k::volume_test_objects(bank,std::int16_t(arg(0)),members,true);
  if(kernel=="camera_control"){k::camera_control(bank,boolean(0));return 0;}
  if(kernel=="object_get_markers")
    return k::object_get_markers(bank,arg(0),arg(1),std::int16_t(arg(2)),(arg(3)&0xffu)!=0,arg(4));
  if(kernel=="unit_set_emotion"){k::unit_set_emotion(bank,arg(0),arg(1),arg(2),arg(3));return 0;}
  if(kernel=="device_group_from_ordinal")return k::device_group_from_ordinal(bank,std::uint16_t(arg(0)));
  if(kernel=="interpolator_start")return k::interpolator_start(bank,arg(0),arg(1),arg(2));
  if(native_campaign_script_noop_builtin_tu15(kernel))return 0;
  throw std::invalid_argument("Unknown native script kernel "+kernel);
}
J run(const J& row) {
  Memory memory;
  for(const auto& bank:row.at("banks"))memory.regions.push_back({bank.at("address"),decode(bank.at("before"))});
  const auto fp=row.value("fp",std::vector<std::uint32_t>{0x027f,0x1f80});
  NativeCampaignScriptFloatingPoint floating_point{std::uint16_t(fp.at(0)),fp.at(1)};
  NativeCampaignScriptWriter writer;
  if(row.value("publisher",false))writer=[&](std::uint32_t address,std::span<const std::byte> bytes){memory.write(address,bytes);};
  NativeCampaignScriptBank bank(memory,writer,floating_point);
  bool completed=true;std::uint32_t result=0;std::string fault;J list=nullptr;
  native_object_factory_tests::stopped_at=0x3000000u;
  try{
    const std::string kernel=row.at("kernel");
    if(kernel=="volume_return_objects" || kernel=="volume_return_objects_by_type") {
      const auto args=words(row,"args");
      const auto mask=kernel=="volume_return_objects"?k::none:args.at(1);
      list=k::volume_return_objects(bank,std::int16_t(args.at(0)),mask);
    } else result=execute(bank,kernel,words(row,"args"),words(row,"members"));
  }
  catch(const std::runtime_error& error){completed=false;fault=error.what();}
  J banks=J::array();for(const auto& region:memory.regions)banks.push_back({{"address",region.address},{"after",hex(region.bytes)}});
  J out{{"completed",completed},{"result",result},{"writes",memory.writes},{"banks",banks}};
  if(row.value("observe_stop_entry",false))out["stopped_at"]=native_object_factory_tests::stopped_at;
  if(!list.is_null())out["list"]=list;
  if(!fault.empty())out["fault"]=fault;
  return out;
}

// ---- Self test: expectations transcribed from the TU1.5 listings ----------
void require(bool condition,const std::string& message){if(!condition)throw std::runtime_error("self-test: "+message);}
struct Fixture {
  Memory memory;
  static constexpr std::uint32_t pool=0x1000000,records=0x1000100,bodies=0x1000400,globals=0x4e0000;
  Fixture() {
    memory.regions.push_back({pool,std::vector<std::byte>(0x4000)});
    memory.regions.push_back({globals,std::vector<std::byte>(0x60000)});
    memory.regions.push_back({0x1100000,std::vector<std::byte>(0x4000)});
    put(0x4ecbd0,pool);put(pool+0x24,12);put(pool+0x38,4);put(pool+0x44,records);
    for(unsigned n=0;n<4;++n){
      put16(records+n*12,std::uint16_t(0x1234+n));memory.view(records+n*12+3,1)[0]=std::byte(n==1?7:n==2?1:0);
      put(records+n*12+8,bodies+n*0x400);put(bodies+n*0x400+0x14,0xffffffffu);
    }
  }
  void put(std::uint32_t a,std::uint32_t v){std::memcpy(memory.view(a,4).data(),&v,4);}
  void put16(std::uint32_t a,std::uint16_t v){std::memcpy(memory.view(a,2).data(),&v,2);}
  std::uint32_t get(std::uint32_t a){std::uint32_t v;std::memcpy(&v,memory.view(a,4).data(),4);return v;}
  std::uint8_t get8(std::uint32_t a){return std::to_integer<std::uint8_t>(memory.view(a,1)[0]);}
  static std::uint32_t handle(unsigned n){return (std::uint32_t(0x1234+n)<<16)|n;}
};
void self_test() {
#ifdef HALO2_NATIVE_SCRIPT_HOST_TESTS
  {
    Fixture f;
    constexpr auto group_pool=0x1100000u,groups=0x1100100u,scenario=0x1100200u,group=0x81230001u;
    const auto body=Fixture::bodies+0x400u;
    f.put(0x4ecbf8u,group_pool);f.put(group_pool+0x24u,12);f.put(group_pool+0x44u,groups);
    f.put(0x4ecc20u,scenario);f.put(scenario+0xa0u,2);f.put16(groups+12u,0x8123u);
    f.put(body+0x130u,group);f.put(body+0x13cu,group);
    f.put(body+0x10u,k::none);f.memory.view(body+0xaau,1)[0]=std::byte(7);
    f.put(body+0x138u,0xdeadbeefu);f.put(body+0x144u,0xfeedbeefu);
    unsigned published=0,fp_calls=0;
    NativeCampaignScriptHostServices services{.memory=&f.memory,
      .writer=[&](auto at,auto bytes){++published;f.memory.write(at,bytes);},
      .floating_point=[&]{++fp_calls;return NativeCampaignScriptFloatingPoint{};}};
    auto result=native_campaign_script_builtin_tu15("device_group_set_immediate",{{18,1,{}},{7,2,{}}},services);
    require(result && result->type==4 && fp_calls==1 && published>0,"immediate device host uses scalar cast, FP and writer");
    require(f.get(groups+16u)==0x3f800000u && f.get(body+0x134u)==0x3f800000u &&
        f.get(body+0x140u)==0x3f800000u && f.get(body+0x138u)==0 && f.get(body+0x144u)==0,
        "authored ordinal resolves high-bit salt and updates both device memberships");
    result=native_campaign_script_builtin_tu15("device_group_get",{{18,1,{}}},services);
    require(result && result->type==6 && result->bits==0x3f800000u,"native group getter is registered");
    result=native_campaign_script_builtin_tu15("device_set_position_immediate",{{54,Fixture::handle(1),{}},{6,0x3f000000u,{}}},services);
    require(result && result->type==4 && (f.get(body+0x12cu)&0x84u)==0x84u,"native immediate device setter refreshes its owner");
    result=native_campaign_script_builtin_tu15("device_get_position",{{54,Fixture::handle(1),{}}},services);
    require(result && result->type==6 && result->bits==0x3f000000u,"native device getter is registered");
    result=native_campaign_script_builtin_tu15("device_get_position",{{54,k::none,{}}},services);
    require(result && result->bits==0 && fp_calls==2,"NONE device getter needs no FP state");
  }
#endif
  {
    // 262590/26264B command context: actor index | 0x80000000 and its squad;
    // the clear leaves ai_current_squad, a unit without an actor clears only.
    Fixture f;NativeCampaignScriptBank bank(f.memory);
    f.put(0x5020c8,0x1100000);f.put(0x1100000+0x44,0x1100100);
    f.put(Fixture::bodies+0x12c,2);f.memory.view(Fixture::bodies+0xaa,1)[0]=std::byte(0);
    f.put(0x1100100+2*0x888+0x30,0x00070005u);
    k::ai_command_context(bank,Fixture::handle(0));
    require(f.get(0x50ef00)==0x80000002u && f.get(0x50ef04)==5u,"command context publishes actor and squad");
    k::ai_command_context(bank,std::nullopt);
    require(f.get(0x50ef00)==0xffffffffu && f.get(0x50ef04)==5u,"command context clears only the actor");
    f.put(0x1100100+2*0x888+0x30,0xffffffffu);k::ai_command_context(bank,Fixture::handle(0));
    require(f.get(0x50ef04)==0xc3e703e7u,"squadless actor publishes the null AI reference");
    f.put(Fixture::bodies+0x12c,0xffffffffu);k::ai_command_context(bank,Fixture::handle(0));
    require(f.get(0x50ef00)==0xffffffffu,"unit without an actor leaves the actor global clear");
    require(k::backed_global(809)->address==0x47c24cu && k::backed_global(809)->constant==4u &&
        k::backed_global(818)->constant==3u && !k::backed_global(798)->constant && k::backed_global(798)->address==0x50ef00u &&
        !k::backed_global(796),"backed external globals follow the 47FB88 table");
  }
  require(k::pin(std::bit_cast<std::uint32_t>(5.f),std::bit_cast<std::uint32_t>(0.f),std::bit_cast<std::uint32_t>(1.f),0x1f80)==std::bit_cast<std::uint32_t>(1.f),"pin upper");
  require(k::pin(std::bit_cast<std::uint32_t>(-5.f),std::bit_cast<std::uint32_t>(0.f),std::bit_cast<std::uint32_t>(1.f),0x1f80)==std::bit_cast<std::uint32_t>(0.f),"pin lower");
  const auto nan=std::bit_cast<std::uint32_t>(std::nanf(""));
  require(k::pin(nan,0,std::bit_cast<std::uint32_t>(1.f),0x1f80)==nan,"pin unordered keeps value");
  require(k::pin(0x80000000u,0x000116c2u,0xc245ee6du,0x1fc0)==0xc245ee6du,"pin DAZ compares a denormal bound as zero");
  require(k::pin(0x80000000u,0x000116c2u,0xc245ee6du,0x1f80)==0x000116c2u,"pin without DAZ keeps the denormal bound");
  {
    Fixture f;f.put(0x4f3cd8,0x4f3d00);f.put(0x4f3d00,0x12345678);NativeCampaignScriptBank bank(f.memory);
    const auto state=0x12345678u*0x19660du+0x3c6ef35fu;
    const auto value=k::random_range(bank,10,20);
    require(f.get(0x4f3d00)==state,"random_range advances the primary seed");
    require(value==std::uint16_t(((10u*(state>>16))>>16)+10u),"random_range arithmetic");
    const auto real=std::bit_cast<float>(k::real_random_range(bank,std::bit_cast<std::uint32_t>(2.f),std::bit_cast<std::uint32_t>(4.f)));
    require(real>=2.f && real<=4.f,"real_random_range interval");
  }
  {
    Fixture f;NativeCampaignScriptBank bank(f.memory);
    const auto body=Fixture::bodies+0x400;
    k::object_set_phantom_power(bank,Fixture::handle(1),1);require(f.get8(body+0xc1)==4,"phantom power set");
    k::object_set_phantom_power(bank,Fixture::handle(1),0);require(f.get8(body+0xc1)==0,"phantom power clear");
    k::unit_kill(bank,Fixture::handle(0),true);
    require(f.get8(Fixture::bodies+0x10a)==0x40 && (f.get8(Fixture::records+2)&2),"silent kill marks update");
    f.put(body+0x14,Fixture::handle(0));require(k::object_get_parent(bank,Fixture::handle(1))==Fixture::handle(0),"parent");
    require(k::object_get_parent(bank,k::none)==k::none,"NONE parent");
    // Device type 7 bit is inside mask 0x80.
    k::device_operates_automatically_set(bank,Fixture::handle(1),0);require(f.get(body+0x1cc)==1,"operates automatically false sets bit0");
    k::device_operates_automatically_set(bank,Fixture::handle(1),1);require(f.get(body+0x1cc)==0,"operates automatically true clears bit0");
    k::device_one_sided_set(bank,Fixture::handle(0),1);require(f.get(Fixture::bodies+0x1cc)==0,"one sided ignores non-device");
    require(f.get(0x1000000+0x3fc0)==0,"untouched bank");
    require(std::bit_cast<float>(k::object_get_shield(bank,k::none))==-1.f,"absent shield is -1");
    std::array members{Fixture::handle(0),Fixture::handle(1),k::none,Fixture::handle(2)};
    require(k::list_count_not_dead(bank,members)==2,"not-dead count stops at NONE");
    f.memory.view(Fixture::bodies+0x10a,1)[0]|=std::byte(4);
    require(k::list_count_not_dead(bank,members)==1,"dead member excluded");
  }
  {
    // MULSS/FISTP pipeline: 30 ticks/s * 0.5 s = 15 ticks after time 1000.
    Fixture f;f.put(0x52be68,0x1100000);f.put(0x51df54,0x1100100);f.put16(0x1100102,30);f.put(0x1100108,1000);
    NativeCampaignScriptBank bank(f.memory);
    k::physics_disable_character_ground_adhesion_forces(bank,std::bit_cast<std::uint32_t>(.5f));
    require(f.get(0x1100018)==1015,"ground adhesion expiry");
    k::physics_disable_character_ground_adhesion_forces(bank,std::bit_cast<std::uint32_t>(9.f));
    require(f.get(0x1100018)==1150,"ground adhesion clamps to five seconds");
    k::physics_set_gravity(bank,std::bit_cast<std::uint32_t>(1.f));
    require(f.get(0x1100000)==0x40857af5u,"gravity scale constant");
  }
  if(const auto it=native_object_factory_tests::kernels().find("nodebuild_codec_frames");
      it!=native_object_factory_tests::kernels().end()) {
    // 284360 (kind 2, mode 2): 28CC70/28D310 (masked) and 28D6E0/28DD20 (unmasked).
    // Verify root-only translation/scale clamping, entry +18 z scaling, second-frame
    // blending/restore, and exact SSE operand order (NaN payload selection).
    constexpr std::uint32_t state=0x510f30u,entry=0x1100000u,remap=0x1100100u,entry_mask=0x1100120u,
        node_mask=0x1100140u,rot_flags=0x1100160u,trans_flags=0x1100164u,scale_flags=0x1100168u,
        data=0x1100200u,orientations=0x1100800u;
    const auto fbits=[](float v){return std::bit_cast<std::uint32_t>(v);};
    const auto run_kind2_mode2=[&](bool masked,bool second,bool nan_check) {
      Fixture f;
      NativeCampaignScriptBank bank(f.memory);
      f.put(entry+0x0cu,remap);f.put(entry+0x14u,entry_mask);f.put(entry+0x18u,fbits(2.f));
      f.put16(remap+0u,1u);f.put16(remap+2u,2u);
      f.put(entry_mask,0x3u);
      f.put(node_mask,0x6u);
      f.memory.view(rot_flags,1)[0]=std::byte(0x3);
      f.memory.view(trans_flags,1)[0]=std::byte(0x3);
      f.memory.view(scale_flags,1)[0]=std::byte(0x3);
      // Track data for 2963D0 (float rotation), 296410 (float translation), 2CEC60 (float scale).
      f.put(data+0x0cu,0x100u);f.put(data+0x10u,0x200u);
      f.put(data+0x14u,0x20u);f.put(data+0x18u,0x18u);f.put(data+0x1cu,0x08u);
      // Track 0 rotation (frames 0,1): identity; Track 1 rotation (frames 0,1): 180 deg around Z.
      for(std::uint32_t frame=0;frame<2;++frame) {
        f.put(data+0x20u+frame*16u+12u,fbits(1.f));
        f.put(data+0x40u+frame*16u+8u,fbits(1.f));
      }
      // Track 0 translation: frame 0 = (1, 2, 3), frame 1 = (3, 6, 7); Track 1: (100, 100, 100).
      f.put(data+0x100u,nan_check?0x7fc00011u:fbits(1.f));
      f.put(data+0x104u,fbits(2.f));f.put(data+0x108u,fbits(3.f));
      f.put(data+0x10cu,nan_check?0x7fc00011u:fbits(3.f));
      f.put(data+0x110u,fbits(6.f));f.put(data+0x114u,fbits(7.f));
      f.put(data+0x118u,fbits(100.f));f.put(data+0x11cu,fbits(100.f));f.put(data+0x120u,fbits(100.f));
      // Track 0 scale: frame 0 = 2.0, frame 1 = 4.0; Track 1 = 99.0.
      f.put(data+0x200u,nan_check?0x7fc00033u:fbits(2.f));
      f.put(data+0x204u,nan_check?0x7fc00033u:fbits(4.f));
      f.put(data+0x208u,fbits(99.f));f.put(data+0x20cu,fbits(99.f));
      // Orientations rows 1 (remapped node 0) and 2 (remapped node 1).
      const auto row1=orientations+0x20u,row2=orientations+0x40u;
      f.put(row1+0x0cu,fbits(1.f));
      f.put(row1+0x10u,nan_check?0x7fc00022u:fbits(10.f));
      f.put(row1+0x14u,fbits(20.f));f.put(row1+0x18u,fbits(30.f));
      f.put(row1+0x1cu,nan_check?0x7fc00044u:fbits(5.f));
      f.put(row2+0x0cu,fbits(1.f));
      f.put(row2+0x10u,fbits(7.f));f.put(row2+0x14u,fbits(8.f));f.put(row2+0x18u,fbits(9.f));
      f.put(row2+0x1cu,fbits(1.5f));
      // Codec state block at 0x510F30.
      f.put(state+0x00u,2u);f.put(state+0x04u,2u);
      f.put(state+0x0cu,2u);f.put(state+0x10u,4u);
      f.put(state+0x14u,0u);f.put(state+0x18u,1u);f.put(state+0x1cu,fbits(0.25f));
      f.put(state+0x20u,fbits(1.f));f.put(state+0x28u,entry);
      f.put(state+0x2cu,masked?node_mask:0u);f.put(state+0x30u,data);
      f.put(state+0x34u,0x2963d0u);f.put(state+0x38u,0x296410u);f.put(state+0x3cu,0x2cec60u);
      f.put(state+0x40u,rot_flags);f.put(state+0x44u,trans_flags);f.put(state+0x48u,scale_flags);
      f.put(state+0x4cu,orientations);
      f.memory.view(state+0x50u,1)[0]=std::byte(second?1:0);
      f.put(state+0x54u,1u);f.put(state+0x58u,1u);f.put(state+0x5cu,fbits(0.f));f.put(state+0x60u,fbits(0.5f));
      it->second(f.memory,bank,{});
      // Pass 0 rotates node 1 (row 2) even though mode 2 clamps pass 1/2 to node 0.
      // Second-frame nlerp renormalizes with RSQRTSS (~1e-3 accuracy on 1.0f).
      const float r2z=std::bit_cast<float>(f.get(row2+0x08u));
      require((second?std::fabs(r2z-1.f)<1e-3f:f.get(row2+0x08u)==fbits(1.f)) &&
          f.get(row2+0x0cu)==fbits(0.f),"284360 pass 0 rotates node 1");
      // Pass 1/2 clamp to node 0: row 2 translation/scale stay untouched.
      require(f.get(row2+0x10u)==fbits(7.f) && f.get(row2+0x14u)==fbits(8.f) &&
          f.get(row2+0x18u)==fbits(9.f) && f.get(row2+0x1cu)==fbits(1.5f),
          "284360 clamps translation and scale to root node");
      // Frame/next/fraction/output restored after second-frame passes.
      require(f.get(state+0x14u)==0u && f.get(state+0x18u)==1u && f.get(state+0x1cu)==fbits(0.25f) &&
          f.get(state+0x70u)==0x510f10u,"284360 restores codec state");
      if(nan_check) {
        // Translation is decoded-first in all four loops; scale is output-first when masked, decoded-first unmasked.
        require(f.get(row1+0x10u)==0x7fc00011u,"284360 translation operand order is decoded-first");
        require(f.get(row1+0x1cu)==(masked?0x7fc00044u:0x7fc00033u),"284360 scale operand order matches masked flag");
      } else {
        require(f.get(row1+0x10u)==fbits(second?12.f:11.f) && f.get(row1+0x14u)==fbits(second?24.f:22.f) &&
            f.get(row1+0x18u)==fbits(second?40.f:36.f),"284360 layers node 0 translation with entry+18 z scale");
        require(f.get(row1+0x1cu)==fbits(second?15.f:10.f),"284360 layers node 0 scale");
      }
    };
    for(const bool masked:{false,true})
      for(const bool second:{false,true})
        for(const bool nan_check:{false,true})
          run_kind2_mode2(masked,second,nan_check);
  }
  std::cout<<"native campaign script builtin self-test passed\n";
}
}

int main(int argc,char** argv)try {
  if(argc>1 && std::string(argv[1])=="--self-test"){self_test();return 0;}
  for(std::string line;std::getline(std::cin,line);)
    if(!line.empty())std::cout<<run(J::parse(line)).dump()<<'\n'<<std::flush;
  return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
