#include "halo2/native_player_profile.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/math.hpp"
#include "halo2/native_weapon_inventory.hpp"
#include "halo2/native_unit_interaction.hpp"
#include "halo2/source_player_departure.hpp"
#include "halo2/inventory.hpp"
#include "halo2/slayer.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/source_round_end.hpp"
#include "halo2/lethal_attribution.hpp"
#include "halo2/original_data.hpp"
#include "halo2/vista_player_birth.hpp"
#include "halo2/source_player_damage.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/source_execution_inputs.hpp"
#include "halo2/source_execution_calls.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/player_command.hpp"
#include "halo2/source_scene_physics.hpp"
#include "halo2/source_model_admission.hpp"
#include "halo2/source_effect_world_locations.hpp"
#include "halo2/raw_datum_pool.hpp"
#include <algorithm>
#include <limits>
#include <bit>
#include <cmath>
#include <set>
#include <cstring>
#include <ctime>
namespace halo2 {
namespace {
std::optional<std::vector<SourceEffectLocation>> weapon_effect_locations(
    MultiplayerObjectRuntime *world,std::uint32_t instance,std::uint32_t definition) {
  if(!world || !world->owns_physics() || instance==null_datum)return {};
  const auto &memory=world->memory();
  const auto pool=Bytes(memory.view(0x4f720cu,4)).u32(0);
  const SourceRawDatumPoolServices storage{.read=[&](auto p,auto n){return memory.view(p,n);}};
  if(!source_raw_datum_lookup(pool,instance,storage))return {};
  return source_effect_world_locations_tu15(memory,instance,definition);
}
void infection_appearance(MultiplayerPlayer &player,std::uint8_t color) {
  auto &profile=player.source_configuration;
  if(!profile) {
    const auto original=source_default_player_configuration();std::array<std::uint8_t,9> appearance;
    std::copy_n(original.begin()+0x40,9,appearance.begin());
    profile=native_selected_player_configuration(player.name,appearance,-1);
    (*profile)[0x7c]=std::uint8_t(player.source_membership);
  }
  (*profile)[0x40]=color;(*profile)[0x41]=color;
}
}
unsigned multiplayer_representation(const std::optional<SourcePlayerConfiguration> &profile) {
  if(!profile)return 2;
  const auto value=std::bit_cast<std::int8_t>((*profile)[0x44]);
  if(value==0 || value==2)return 2;
  if(value==1 || value==3)return 3;
  throw FormatError("Unsupported multiplayer player representation");
}
unsigned source_session_player_representation(const std::optional<SourcePlayerConfiguration> &profile,int game_type) {
  if(game_type==2)return multiplayer_representation(profile);
  if(game_type!=1 || !profile)throw FormatError("Campaign source representation requires its original configuration");
  const auto representation=std::bit_cast<std::int8_t>((*profile)[0x44]);
  if(representation<0)throw FormatError("Campaign source representation is absent");
  return unsigned(representation);
}
std::vector<unsigned> source_session_player_representations(const LevelContent &content,int game_type) {
  if(game_type!=1 && game_type!=2)throw FormatError("Unsupported source player world kind");
  std::vector<unsigned> result;
  for(unsigned i=game_type==2?2:0;i<content.player_representations.size() && (game_type==1 || i<4);++i)
    if(content.player_representations[i].biped.datum!=null_datum)result.push_back(i);
  return result;
}
const BipedDefinition &multiplayer_biped(const LevelContent &content,unsigned representation) {
  if(content.player_representations.empty() && representation==2)return content.player;
  if(representation>=content.player_representations.size() ||
      content.player_representations[representation].biped.datum==null_datum)
    throw FormatError("Selected multiplayer biped is absent from this map");
  return content.player_representations[representation].biped;
}
namespace {
const WeaponState *inventory_weapon(const Unit &unit, std::int16_t slot) {
  const auto &inventory = unit.source_inventory;
  if (slot < 0 || slot >= std::int16_t(inventory.slots.size())) return nullptr;
  const auto index = std::size_t(slot);
  const auto datum = inventory.slots[index];
  if (datum == null_datum) return nullptr;
  if (inventory.current[0] == slot)
    return unit.weapon.source_datum == datum ? &unit.weapon : nullptr;
  const auto &inactive = inventory.inactive_weapons[index];
  return inactive && inactive->source_datum == datum ? &*inactive : nullptr;
}
const WeaponState *inventory_weapon(const Unit &unit, std::uint32_t datum) {
  if (datum == null_datum) return nullptr;
  for (std::int16_t slot = 0; slot < 4; ++slot)
    if (unit.source_inventory.slots[std::size_t(slot)] == datum)
      return inventory_weapon(unit, slot);
  return nullptr;
}
SourceRoundServices slayer_round_services() {
  SourceRoundServices services;
  // Original registered Slayer +C initializer has no stores (U5 A6780),
  // and +90 phase callback is RET4 (U5 2E4D60). Later-round world reset,
  // preparation events and original replication remain required/unavailable.
  services.initialize_engine=[]{};
  services.phase_changed=[](std::int32_t){};
  return services;
}
} // namespace
int slayer_parameter_default(unsigned id) {
  if (id >= 112)
    throw FormatError("Unknown Slayer variant parameter");
  static const auto defaults=source_game_variant_default(0);
  return source_game_variant_get(defaults,int(id));
}
namespace {
SlayerRules native_engine_rules(const UiVariant &variant, bool team_session) {
  const bool king=variant.engine==3;
  const auto source=source_game_variant(variant);
  // This is admission, matching the source checker that validates a copy.
  // A saved-buffer owner can instead call the recovered reset normalizer.
  if (!source_game_variant_valid(source))
    throw FormatError("The multiplayer settings fail original game variant validation.");
  const auto setting=[&](int parameter){return source_game_variant_get(source,parameter);};
  const bool teams = setting(91) != 0;
  if (teams && !team_session)
    throw FormatError("Team Slayer requires selected-team session inputs.");
  if (!king && setting(28))
    throw FormatError("Slayer bonus scoring is unfinished.");
  const auto custom=custom_slayer(variant);
  const auto starting_weapon = setting(89);
  if (custom==CustomSlayer::none && starting_weapon != 0 && (starting_weapon<3 || starting_weapon>20))
    throw FormatError("Choose Map Default or a specific starting weapon.");
  // Capabilities cover source fields/bits rather than the spelling of an
  // override ID. Aliases, narrowed writes and absent descriptors therefore
  // have the same admission as their actual source data. This is still a
  // bounded native adapter; unsupported field changes remain gated.
  std::array<std::uint8_t,0x130> supported{};
  const auto descriptors=source_game_variant_descriptors();
  auto permit=[&](unsigned id) {
    const auto &d=descriptors[id];
    const unsigned width=d.kind==3 ? 1 : d.kind==4 ? 2 : 4;
    const auto bits=d.kind==1 ? std::uint32_t(1) << (d.operand & 31) : 0xffffffffu;
    for(unsigned n=0;n<width;++n) supported[d.offset+n] |= std::uint8_t(bits >> (n*8));
  };
  // Parameter16 is the original variant+7C life limit. Its shared native
  // initialization/death/respawn/end/scoreboard owners consume the field.
  // Round selector0 remains the default: original15DDC0 finishes index0
  // directly, so this admission does not require the later-round reset owner.
  for(const unsigned id : {1u,2u,3u,4u,5u,6u,7u,8u,9u,10u,16u,17u,18u,15u,90u,89u,88u,76u,77u,78u,81u,82u,83u,24u,104u,105u}) permit(id);
  if(custom!=CustomSlayer::none)permit(94);
  if(!king)for(const unsigned id : {29u,30u})permit(id);
  if(king)for(const unsigned id : {38u,39u,40u,41u,42u})permit(id);
  if(teams && team_session) for(const unsigned id : {91u,23u,25u,26u}) permit(id);
  const auto defaults=source_game_variant_default(variant.engine);
  for(unsigned n=0;n<source.bytes.size();++n)
    if((source.bytes[n]^defaults.bytes[n]) & std::uint8_t(~supported[n]))
      throw FormatError("This option is unfinished in the native source-settings adapter.");
  SlayerRules rules;
  rules.session_configuration=variant;
  rules.custom=custom;rules.secondary_starting_weapon=unsigned(setting(94));
  if(custom!=CustomSlayer::none && (king || teams || starting_weapon<0 || starting_weapon>20 || rules.secondary_starting_weapon>20))
    throw FormatError("Custom Slayer requires free-for-all Slayer and admitted weapon options");
  rules.source_engine=king ? 4 : 2;
  // The shared source checker owns numeric ranges, widths and masks.
  rules.name = variant.name;
  if (rules.name.empty() || rules.name.size() > 32 ||
      std::any_of(rules.name.begin(), rules.name.end(),
                  [](unsigned char c) { return c < 32; }))
    throw FormatError("Slayer variant name must contain 1..32 bytes");
  rules.score_limit = setting(2);
  rules.round_mode=setting(0);
  rules.life_limit=setting(16);
  const auto time = setting(0xa), respawn = setting(0x11),
             suicide = setting(0x12);
  rules.time_ticks = unsigned(time) * 30;
  rules.respawn_ticks = unsigned(respawn) * 30;
  rules.suicide_ticks = unsigned(suicide) * 30;
  rules.suicide_loss = setting(0x1d) != 0;
  rules.death_loss = setting(0x1e) != 0;
  rules.team_mode = teams;
  rules.team_scoring = setting(23);rules.respawn_modifier = setting(25);
  const auto betrayal = setting(26);
  if (teams && rules.respawn_modifier == 1 && respawn == 0)
    throw FormatError("Zero-interval source Team Slayer cycling remains unrecovered.");
  rules.betrayal_ticks = unsigned(betrayal) * 30;
  rules.starting_grenades = setting(0x58) != 0;
  const auto maximum = std::min(setting(0xf), setting(0x5a));
  if (maximum < 1 || maximum > 16)
    throw FormatError("Slayer maximum players must be 1..16");
  rules.maximum_players = unsigned(maximum);
  rules.starting_weapon = unsigned(starting_weapon);
  rules.weapon_set=unsigned(setting(83));
  if(rules.weapon_set!=0 && rules.weapon_set!=15 && rules.weapon_set!=16 && rules.weapon_set!=17)
    throw FormatError("This weapon set requires its original equipment owner");
  const auto vehicle=setting(76);
  // Literal primary-light choices in the original sily:4 is absent.
  if(vehicle<0 || vehicle>7 || vehicle==4)
    throw FormatError("Invalid Primary Light Vehicle setting");
  rules.primary_light_vehicle=vehicle;
  if(const auto heavy=setting(78);heavy!=0 && heavy!=1 && heavy!=2 && heavy!=4)
    throw FormatError("Random Primary Heavy Vehicle needs its original per-birth RNG/template owner");
  rules.primary_heavy_vehicle=setting(78);
  for(const unsigned id:{81u,82u})if(const auto choice=setting(id);choice<0 || choice>6 || choice==5)
    throw FormatError("Random or invalid Turret choice needs its original per-birth RNG/template owner");
  rules.player_flags=0;
  for(unsigned n=0;n<4;++n)rules.player_flags|=std::uint32_t(source.bytes[0x48+n])<<(n*8);
  if(king) {
    for(unsigned n=0;n<4;++n)rules.king_flags|=std::uint32_t(source.bytes[0xf0+n])<<(n*8);
    rules.king_move_seconds=std::int16_t(setting(40));
  }
  return rules;
}
} // namespace
SlayerRules slayer_rules(const UiVariant &variant,bool team_session) {
  if(variant.engine!=0)
    throw FormatError("This multiplayer build supports Slayer; other game engines are unfinished.");
  return native_engine_rules(variant,team_session);
}
SlayerRules king_rules(const UiVariant &variant,bool team_session) {
  if(variant.engine!=3)throw FormatError("King binding requires the original King variant type");
  return native_engine_rules(variant,team_session);
}
void validate_slayer_map_options(const LevelContent &content,const SlayerRules &rules) {
  // Original15C110->15BB10 (u5 15D4F0->15CEF0) passes the authored
  // netgame row+14 classification. Only case1 reads variant+CD/options+209;
  // changing this option never synthesizes an additional spawn location.
  // Retain the value on maps with no such rows. Affected maps require cold
  // templates resolved from the same variant, before any source birth.
  if(std::any_of(content.netgame_equipment.begin(),content.netgame_equipment.end(),
      [](const auto &row){return row.classification==1;}) &&
      content.light_vehicle_options[0]!=rules.primary_light_vehicle)
    throw FormatError("Primary Light Vehicle templates do not match the selected variant");
  if(rules.session_configuration) {
    const auto variant=source_game_variant(*rules.session_configuration);
    for(unsigned n=0;n<2;++n)if(std::any_of(content.netgame_equipment.begin(),content.netgame_equipment.end(),
        [n](const auto &row){return row.classification==int(6u+n);}) &&
        content.turret_vehicle_options[n]!=source_game_variant_get(variant,int(81u+n)))
      throw FormatError("Turret templates do not match the selected variant");
  }
}
bool slayer_spawn(const Spawn &spawn, std::size_t bsp, bool initial) {
  // 14EF00 (updated 150640) scores every player-start position. Its
  // 23BA90/23DB90 input reads only +0,+4,+8; the selector arrays belong to
  // older start metadata and are not a multiplayer eligibility filter.
  // Initial/respawn selection changes the dynamic-zone bank instead.
  // Paired complete player setup also ignores the authored BSP short. World
  // location is resolved later from the selected position, during creation.
  (void)spawn;
  (void)bsp;
  (void)initial;
  return true;
}
MultiplayerMatch::MultiplayerMatch(const LevelContent &content, std::size_t bsp,
                                   std::span<const Spawn> spawns,
                                   SlayerRules rules,MultiplayerKingServices king_services,
                                   std::function<std::uint32_t()> result_wall_clock,
                                   MultiplayerObjectRuntime *source_object_world,
                                    std::optional<SourceSceneGameContext> game_context,
                                    std::function<std::optional<Spawn>(unsigned,bool)> source_spawn_input)
    : content_(content),source_object_world_(source_object_world),
      source_game_context_(game_context.value_or(SourceSceneGameContext{2,rules.source_engine})),
      // Original multiplayer game type2; registered engine type is separate.
      simulation_(content, bsp, spawns.empty() ? Spawn{} : spawns.front(),
                  {}, {}, {}, source_game_context_,
                  source_object_world ? std::optional<NativeObjectLifetimes>(std::in_place,
                      source_object_world->memory(),source_object_world->object_pool()) : std::nullopt,
                  source_object_world && source_object_world->owns_physics() ? SourceSceneObjectOwner{
                      [source_object_world,&content](const auto &placement,SourceScenarioBirth birth){
                          if(placement.netgame_row>=0)
                              return source_object_world->create_netgame_object(unsigned(placement.netgame_row));
                          if(placement.model_attachment) {
                              const auto resolve=[&](auto &&self,std::size_t index,unsigned depth)->std::uint32_t {
                                  if(depth>8u)throw FormatError("Source vehicle model attachment chain is cyclic");
                                  const auto &parent=content.objects.at(index);
                                  if(parent.netgame_row>=0)
                                      return source_object_world->netgame_object(unsigned(parent.netgame_row));
                                  if(parent.model_attachment)
                                      return source_object_world->child_object(self(self,parent.model_attachment->parent,depth+1u),parent.definition);
                                  throw FormatError("Source vehicle child requires its actual retained parent identity");
                              };
                              const auto parent=resolve(resolve,placement.model_attachment->parent,0);
                              const auto child=source_object_world->child_object(parent,placement.definition);
                              if(child==null_datum)throw FormatError("Original source vehicle constructor has not published its authored model child");
                              return child;
                          }
                          return source_object_world->create_scenario_object(placement,birth);
                      },
                      [source_object_world](auto object){source_object_world->destroy_object(object);}}
                      : SourceSceneObjectOwner{}),
      spawns_(spawns.begin(), spawns.end()),source_spawn_input_(std::move(source_spawn_input)), rules_(rules),king_services_(std::move(king_services)),
      player_pool_(source_object_world ? DatumPool(source_object_world->memory(),source_object_world->player_pool())
                                      : DatumPool("players",multiplayer_capacity,0x21c)),
      result_wall_clock_(result_wall_clock ? std::move(result_wall_clock) : [] {
        const auto value=std::time(nullptr);
        if(value==std::time_t(-1))throw FormatError("Native result wall clock is unavailable");
        return std::uint32_t(value);
      }) {
  simulation_.source_physics_movement_=source_object_world_ && source_object_world_->owns_physics();
  if(simulation_.source_physics_movement_)simulation_.source_vehicle_seat_query_=
      [this](auto unit,auto vehicle){return source_object_world_->vehicle_seat_target(unit,vehicle);};
  if(simulation_.source_physics_movement_)simulation_.source_device_script_=
      [this](const auto &name,const auto &arguments) {
        const auto result=source_object_world_->script_device(name,arguments);
        if(name!="device_get_position" && name!="device_get_power" && name!="device_group_get")
          adopt_source_devices();
        return result;
      };

  if(rules_.weapon_set && !simulation_.source_physics_movement_)
    throw FormatError("Weapon-set variants require the original source equipment owner");
  if(simulation_.source_physics_movement_) {
    publish_source_scene_physics(simulation_.scene_,source_object_world_->memory(),source_object_world_->object_pool());
    adopt_source_devices();
    simulation_.synchronize_props();
    adopt_source_prop_regions();
  }
  if(rules_.source_engine!=2 && rules_.source_engine!=4)
    throw FormatError("Native match requires a bound original game engine");
  if(rules_.player_flags&4u)
    throw FormatError("Global camouflage requires its original timer, fade and render consumers");
  if(rules_.source_engine==4) {
    if((rules_.king_flags&~15u) || rules_.king_move_seconds<0)
      throw FormatError("King trait or malformed movement options are unavailable in this binding");
    if(!king_services_.score_changed || !king_services_.notification)
      throw FormatError("King requires actual score-event and objective-event consumers");
    king_markers_.resize(content_.netgame_flags.size());
    for(unsigned n=0;n<king_markers_.size();++n) {
      if(content_.netgame_flags[n].source_index!=n)
        throw FormatError("King scenario markers lost their original source order");
      std::memcpy(&king_markers_[n],content_.netgame_flags[n].source_bytes.data(),sizeof(SourceKingMarker));
    }
    source_king_initialize(king_state_,king_hills_,king_markers_);
  }
  validate_slayer_map_options(content,rules_);
  if(content.original_data && content.original_data->game_over_constants)
    game_over_constants_=*content.original_data->game_over_constants;
  // Original19A430 enables recording for the selected multiplayer engine
  // outside raw mode4. Missing original descriptors leave this view unavailable.
  if(content.original_data && content.original_data->result_stat_descriptors)
    retained_recording_.enabled=1;
  // Whole19A430 cold event fields: ring/index/start are zero; last is the
  // actual time32 sample. Full retained header/machine refresh is separate.
  result_events_.last=result_wall_time();
  if(rules_.maximum_players<1 || rules_.maximum_players>multiplayer_capacity)
    throw FormatError("Invalid multiplayer participant capacity");
  if(rules_.life_limit<0 || rules_.life_limit>32767)
    throw FormatError("Invalid original life limit");
  if(rules_.team_mode && (rules_.team_scoring < 0 || rules_.team_scoring > 2 ||
      rules_.respawn_modifier < 0 || rules_.respawn_modifier > 2 ||
      (rules_.respawn_modifier == 1 && !rules_.respawn_ticks)))
    throw FormatError("Invalid source Team Slayer session settings");
  if (!content.weapons.contains(rules.weapon))
    throw FormatError("Missing multiplayer starting weapon");
  if (content.player.datum == null_datum)
    throw FormatError("Missing multiplayer player representation");
  const auto &starting=content.weapons.at(rules.weapon);
  const bool energy_ready=starting.energy && starting.barrel_timing && starting.firing_presentation;
  if(starting.capacity<=0 && !(starting.inventory_flags&(1u<<24)) && !energy_ready)
    throw FormatError("Starting weapon requires its original trigger/energy owner");
  if (starting.capacity>0 && starting.initial_rounds < 0 &&
      !(starting.inventory_flags&(1u<<24)))
    throw FormatError("Missing authored initial magazine ammunition");
  simulation_.actors_.clear(); // No AI squads in a player-controlled match.
  simulation_.actors_.resize(multiplayer_capacity - 1);
  for (auto &unit : simulation_.actors_) {
    unit.exists = false;
    unit.controlled = true;
  }
  simulation_.player_.exists = false;
  simulation_.multiplayer_weapon_steps_ = true;
  simulation_.random_state_ = rules_.random_seed;
  simulation_.allocate_object_ = [this](std::uint8_t type) { return allocate_object(type); };
  simulation_.release_object_ = [this](std::uint32_t datum) {
    if (simulation_.object_lifetimes_.header(datum).empty())
      throw FormatError("Native projectile release received a stale full object datum");
    simulation_.object_lifetimes_.release(datum);
  };
  simulation_.damage_hook_ = [this](Unit &unit, const DamageOwner &owner,
      const UnitDamageResult &result,const WeaponDefinition &weapon,
      std::optional<DamageImpactContext> impact,const DamageFeedbackState &prior_feedback) {
    if(source_object_world_ && source_object_world_->owns_physics() && impact && weapon.damage!=null_datum) {
      source_object_world_->deliver_vitality(unit.source_datum,
          {unit.vitality.body,unit.vitality.shield,prior_feedback});
      source_object_world_->deliver_damage_reaction(unit.source_datum,weapon.damage,owner,result,*impact);
      source_object_world_->deliver_vitality(unit.source_datum,
          {unit.vitality.body,unit.vitality.shield,unit.damage_feedback});
    }
    damage_event(unit,owner,result,std::int16_t(weapon.damage_category));
  };
  simulation_.player_damage_scale_=[this](const DamageOwner& owner,std::uint32_t victim) {
    if(rules_.custom==CustomSlayer::infection && owner.player!=victim &&
        player_from_datum(owner.player) && player_from_datum(victim) &&
        bool(infection_zombies_&(1u<<(owner.player&0xffffu)))==bool(infection_zombies_&(1u<<(victim&0xffffu))))return 0.f;
    const auto raw_player=[this](std::uint32_t datum)->const MultiplayerPlayer& {
      // Whole15F520 and the selected trait callback index the raw low16 slot.
      // A captured old salt deliberately reads retained/reallocated slot data;
      // checked full-datum attribution is a later, separate source consumer.
      if(datum==null_datum || (datum&0xffffu)>=players_.size())
        throw FormatError("Player damage received an out-of-bounds raw player slot");
      return players_[datum&0xffffu];
    };
    SourcePlayerDamageServices services;
    services.player=[raw_player](std::uint32_t datum) {
      const auto& player=raw_player(datum);
      return SourcePlayerDamageState{player.source_strength,player.source_membership,player.source_field_194};
    };
    services.trait=[this,raw_player](std::uint32_t datum,std::int32_t selector) {
      const auto& player=raw_player(datum);
      // Slayer's actual registered +8C uses the shared global query. King
      // overrides it with the signed inside timer and hill bits, even false.
      return source_king_trait(rules_.source_engine==4 ? player.source_king_inside_ticks : 0,
          selector,rules_.king_flags,rules_.player_flags);
    };
    // Both bound engines register the actual shared +6C short inequality.
    // There is no additional team-mode test in whole15F520.
    services.enemies=[](std::int16_t a,std::int16_t b){return a!=b;};
    return source_player_damage_scale(true,rules_.player_flags,owner.player,victim,services);
  };
  simulation_.first_person_activity_hook_=[this](const Unit &unit,const WeaponState &weapon,std::uint32_t activity) {
    if(simulation_.source_physics_movement_)return;
    auto apply=[&](FirstPersonViewBinding &view,const auto &definitions) {
      if(view.unit!=unit.source_datum)return;
      const auto found=definitions.find({unsigned(view.representation),weapon.definition});
      if(found==definitions.end())return;
      for(auto &hand:view.hands)if(hand.weapon==weapon.source_datum)
        first_person_activity(hand,found->second,activity);
    };
    for(auto &view:first_person_views_)apply(view,first_person_definitions_);
    if(unit.source_player!=null_datum) {
      const auto id=unit.source_player&0xffffu;
      if(id<players_.size() && players_[id].source_datum==unit.source_player)
        apply(replicated_fp_views_[id],replicated_fp_definitions_);
    }
  };
  simulation_.first_person_timing_hook_=[this](const Unit &unit,const WeaponState &weapon,std::uint32_t activity,std::int16_t section) {
    //169F3B selects the owning player's representation and authored weapon
    //graph independently of167D32 local first-person registration.
    if(unit.source_player==null_datum)return std::int16_t(0);
    const auto *player=player_from_datum(unit.source_player);
    if(!player)throw FormatError("Weapon animation timing lost its actual player owner");
    if(!content_.original_data || !content_.original_data->melee_constants)
      throw FormatError("Weapon animation timing constants are unavailable");
    const auto representation=source_session_player_representation(player->source_configuration,source_game_context_.game_type);
    const auto &records=content_.weapons.at(weapon.definition).first_person_models;
    if(records.empty())return std::int16_t(0);
    const unsigned preferred=representation==1 || representation==3 ? 1u:0u;
    if(records[records.size()>preferred?preferred:0].animation==null_datum)return std::int16_t(0);
    const auto key=std::pair{representation,weapon.definition};
    const FirstPersonBindingDefinition *definition=nullptr;
    if(const auto found=replicated_fp_definitions_.find(key);found!=replicated_fp_definitions_.end())definition=&found->second;
    else if(const auto found=first_person_definitions_.find(key);found!=first_person_definitions_.end())definition=&found->second;
    if(!definition && activity==0x08000025u) {
      const auto &timing=content_.weapons.at(weapon.definition).put_away_timing[(representation&1u)!=0]
          [unit.source_inventory.current[0]!=-1 && unit.source_inventory.current[1]!=-1];
      if(!timing.present)return std::int16_t(0);
      return weapon_animation_timing(timing.frames,timing.event4,timing.event0,section,30,
          std::bit_cast<float>(content_.original_data->melee_constants->frame_seconds_bits));
    }
    if(!definition)throw FormatError("Original weapon animation timing requires its authored representation binding");
    return weapon_animation_activity_timing(*definition,
        unit.source_inventory.current[0]!=-1 && unit.source_inventory.current[1]!=-1,
        activity,section,30,std::bit_cast<float>(content_.original_data->melee_constants->frame_seconds_bits));
  };
  // Native presentation replication records the actual recovered firing
  // callback. Shot counters and animation activities cannot create effects.
  simulation_.weapon_effect_hook_=[this](const Unit &unit,const WeaponEffectInput &input){
    if(input.effect!=null_datum && unit.source_player!=null_datum){
      const auto id=unit.source_player&0xffffu;
      if(id>=players_.size() || players_[id].source_datum!=unit.source_player)
        throw FormatError("Weapon effect lost its current full player owner");
      weapon_effect_events_[id]={++weapon_effect_serial_,std::uint32_t(simulation_.ticks()+1),
          unit.source_datum,input.weapon,input.effect,input.a_bits,input.b_bits};
      weapon_effect_journal_.push_back(weapon_effect_events_[id]);
      if(weapon_effect_journal_.size()>64)weapon_effect_journal_.erase(weapon_effect_journal_.begin());
    }
    return weapon_effect_owner_?weapon_effect_owner_(unit,input):null_datum;
  };
  if(simulation_.source_physics_movement_)source_object_world_->observe_weapon_effects(
      [this](const SourceWeaponEffectEventTu15 &event) {
    NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;
    if(event.unit==null_datum || event.input.effect==null_datum ||
       event.presentation.instance==null_datum || !event.presentation.colors_observed)return;
    NativeWeaponEffectEvent completed{++weapon_effect_serial_,event.tick,event.unit,event.input.weapon,
        event.input.effect,event.input.a_bits,event.input.b_bits};
    completed.colors=event.presentation.colors;completed.velocity=event.velocity;
    completed.source_instance=event.presentation.instance;completed.source_presentation=true;completed.source_scheduled=true;
    if(auto locations=weapon_effect_locations(source_object_world_,completed.source_instance,completed.effect)) {
      completed.source_locations=std::move(*locations);completed.source_locations_observed=true;
    }
    // A source weapon's parent can be a vehicle or AI unit. The global
    // presentation journal does not require a native player-slot owner.
    if(const auto *unit=unit_from_datum(event.unit);unit && unit->source_player!=null_datum) {
      const auto id=unit->source_player&0xffffu;
      if(id>=players_.size() || players_[id].source_datum!=unit->source_player)
        throw FormatError("Scheduled weapon effect lost its full native player owner");
      weapon_effect_events_[id]=completed;
    }
    weapon_effect_journal_.push_back(std::move(completed));
    if(weapon_effect_journal_.size()>64)weapon_effect_journal_.erase(weapon_effect_journal_.begin());
  });
  // No participants or world objects exist at the original first-active
  // boundary. The shared owner initializes the clock, stat validity and bank.
  apply_round_phase();
  //20A750 initializes roots without dispatch. Ordinary Simulation::step
  //runs them before devices/object updates; remote snapshot observers do
  //not call this authoritative step.
  simulation_.multiplayer_script_lists_.emplace();
  simulation_.start_level();
  if(source_object_world_ && source_object_world_->owns_physics())
    source_object_world_->observe_projectile_effects(
        [this](const SourceProjectileDetonationTu15 &event){publish_source_projectile_contact(event);});
}
MultiplayerMatch::~MultiplayerMatch() {
  if(source_object_world_ && source_object_world_->owns_physics())source_object_world_->retire_projectile_effects();
}
void MultiplayerMatch::release_presentation_owners() {
  NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;
  weapon_effect_owner_={};first_person_event_owner_={};
  simulation_.world_weapon_effect_hook_={};
}
void MultiplayerMatch::configure_weapon_effect_owner(
    std::function<std::uint32_t(const Unit &,const WeaponEffectInput &)> owner) {
  NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;
  weapon_effect_owner_=std::move(owner);
}
void MultiplayerMatch::weapon_effect_presentation(std::uint32_t unit,std::uint32_t weapon,
    const SourceWeaponEffectPresentation &source,Vec3 velocity,std::span<const SourceEffectLocation> locations) {
  NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;
  if(weapon_effect_journal_.empty() || weapon_effect_journal_.back().unit!=unit ||
      weapon_effect_journal_.back().weapon!=weapon || !source.colors_observed)
    throw FormatError("Weapon presentation differs from its completed prepared-shot request");
  auto &event=weapon_effect_journal_.back();event.colors=source.colors;event.velocity=velocity;
  event.source_instance=source.instance;event.source_presentation=true;
  if(!locations.empty()) {
    event.source_locations.assign(locations.begin(),locations.end());event.source_locations_observed=true;
  }else if(auto frames=weapon_effect_locations(source_object_world_,source.instance,event.effect)) {
    event.source_locations=std::move(*frames);event.source_locations_observed=true;
  }
  for(auto &latest:weapon_effect_events_)if(latest.serial==event.serial)latest=event;
}
void MultiplayerMatch::configure_melee_animation(std::shared_ptr<const Cache> cache,NativeMeleeConstants constants) {
  NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;
  if(simulation_.melee_animation_)throw FormatError("Melee animation owner already configured");
  simulation_.melee_animation_=std::make_shared<NativeMeleeAnimation>(std::move(cache),content_,constants);
}
void MultiplayerMatch::configure_grenades(std::shared_ptr<const Cache> cache,NativeGrenadeConstants constants) {
  NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;
  if(simulation_.grenade_sandbox_)throw FormatError("Grenade owner is already configured");
  simulation_.grenade_sandbox_=std::make_shared<NativeGrenadeSandbox>(std::move(cache),content_.player.datum,constants);
}
void MultiplayerMatch::configure_first_person_event_owner(FirstPersonEventOwner owner) {
  NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;
  if (!owner)
    throw FormatError("First-person events require an explicit owner consumer");
  first_person_event_owner_ = std::move(owner);
}
void MultiplayerMatch::configure_world_weapon_rows(std::span<const SourceNetgameWeaponRegistration> records){
  NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;
  if(world_weapon_configured_ || simulation_.ticks()!=0)
    throw FormatError("source netgame rows must be configured once before the native clock");
  if(content_.netgame_equipment.size()>100)
    throw FormatError("source netgame row capacity exceeded");
  std::set<unsigned> rows;
  for(const auto&record:records){
    if(record.row>=content_.netgame_equipment.size() || !rows.insert(record.row).second)
      throw FormatError("source netgame registration has an invalid/duplicate row");
    const auto birth=source_netgame_weapon_birth(content_.netgame_equipment[record.row],record.definition,2);
    if(birth.descriptor!=record.birth.descriptor || birth.object_scale!=record.birth.object_scale ||
       birth.levitated!=record.birth.levitated)
      throw FormatError("source netgame registration differs from authored creation packet");
    (void)source_netgame_weapon_initial_state(record.definition,record.birth);
    if(!std::isfinite(record.sphere_center.x)||!std::isfinite(record.sphere_center.y)||
       !std::isfinite(record.sphere_center.z)||!std::isfinite(record.sphere_radius)||record.sphere_radius<0)
      throw FormatError("source netgame registration requires its actual constructor sphere");
  }
  world_weapon_definitions_.assign(records.begin(),records.end());
  world_weapon_rows_.resize(content_.netgame_equipment.size());
  world_weapon_configured_=true;
}
void MultiplayerMatch::step_world_weapons(std::uint32_t tick){
  if(!world_weapon_configured_)return;
  for(unsigned index=0;index<world_weapon_definitions_.size();++index){
    const auto&record=world_weapon_definitions_[index];auto&row=world_weapon_rows_.at(record.row);
    std::optional<SourceNetgameWeaponBound> bound;
    if(row.object!=null_datum){
      const auto owner=std::find_if(world_weapon_objects_.begin(),world_weapon_objects_.end(),
          [&](const auto&o){return o.handle==row.object && o.row_bound;});
      if(owner==world_weapon_objects_.end() || simulation_.object_lifetimes_.header(row.object).empty())
        throw FormatError("source netgame row retained a stale full native object handle");
      bound=SourceNetgameWeaponBound{owner->handle,record.sphere_center,
          owner->state.item.item_flags,owner->state.control10a};
    }
    if(source_netgame_weapon_step(row,content_.netgame_equipment[record.row],record.row,
        {std::bit_cast<std::int32_t>(tick),30,false,true,rules_.source_engine,0},bound)){
      source_netgame_weapon_allocate(row,record.row,record.birth,
          [&](std::uint32_t handle){
            auto owner=std::find_if(world_weapon_objects_.begin(),world_weapon_objects_.end(),
                [&](const auto&o){return o.handle==handle && o.row_bound;});
            if(owner==world_weapon_objects_.end())throw FormatError("source netgame unbind has no live native owner");
            //15B1E0 releases the row association, not the actual world object.
            owner->row_bound=false;
          },[&](const SourceNetgameWeaponBirth&,unsigned ordinal){
            const bool source_birth=source_object_world_ && source_object_world_->owns_physics();
            const auto handle=source_birth?source_object_world_->create_netgame_object(ordinal):allocate_object(2);
            if(handle!=null_datum) {
              if(source_birth)simulation_.object_lifetimes_.observe_source_birth(handle);
              world_weapon_objects_.push_back({index,ordinal,handle,tick,true,
                  source_netgame_weapon_initial_state(record.definition,record.birth)});
              if(!source_birth)simulation_.object_lifetimes_.publish_native(handle,record.definition.definition);
            }
            return handle;
          });
    }
  }
  for(auto&owner:world_weapon_objects_){
    const auto&definition=world_weapon_definitions_.at(owner.registration).definition;
    source_levitated_weapon_update(definition,owner.state.item);
  }
}
Unit &MultiplayerMatch::unit_mutable(unsigned id) {
  if (id >= multiplayer_capacity)
    throw FormatError("Invalid multiplayer participant");
  return id ? simulation_.actors_[id - 1] : simulation_.player_;
}
const Unit &MultiplayerMatch::unit(unsigned id) const {
  return const_cast<MultiplayerMatch *>(this)->unit_mutable(id);
}
std::vector<Transform> MultiplayerMatch::source_world_nodes(std::uint32_t object)const {
  if(!source_object_world_)throw FormatError("Match has no retained source object world");
  return source_object_world_->world_nodes(object);
}
ShadowCasterSphere MultiplayerMatch::source_object_sphere(std::uint32_t object)const {
  if(!source_object_world_)throw FormatError("Match has no retained source object world");
  return source_object_world_->object_sphere(object);
}
const SourceInventory &MultiplayerMatch::source_inventory(unsigned id) const {
  return unit(id).source_inventory;
}
const WeaponState *MultiplayerMatch::inventory_weapon(unsigned id, std::int16_t slot) const {
  return halo2::inventory_weapon(unit(id), slot);
}
const WeaponState *MultiplayerMatch::weapon_from_datum(std::uint32_t datum) const {
  const auto header = simulation_.object_lifetimes_.header(datum);
  if (header.empty() || header[3] != 2) return nullptr;
  for (unsigned id = 0; id < players_.size(); ++id)
    if (const auto *weapon = halo2::inventory_weapon(unit(id), datum)) return weapon;
  for (const auto &[unit_datum, unit] : retained_units_)
    if (const auto *weapon = halo2::inventory_weapon(unit, datum)) return weapon;
  return nullptr;
}
const MultiplayerPlayer &MultiplayerMatch::player(unsigned id) const {
  return players_.at(id);
}
SourceCamouflageServices MultiplayerMatch::camouflage_services() {
  SourceCamouflageServices services;
  services.player=[this](std::uint32_t datum) {
    if(datum==null_datum || (datum&0xffffu)>=players_.size())
      throw FormatError("Source powerup owner received an unavailable raw player slot");
    auto& player=players_[datum&0xffffu];
    return SourceCamouflagePlayerView{player.source_powerup_timers,player.source_current_object};
  };
  services.trait=[this](std::uint32_t datum,std::int32_t selector) {
    if(datum==null_datum || (datum&0xffffu)>=players_.size())
      throw FormatError("Source camouflage trait received an unavailable raw player slot");
    const auto& player=players_[datum&0xffffu];
    return source_king_trait(rules_.source_engine==4 ? player.source_king_inside_ticks : 0,
        selector,rules_.king_flags,rules_.player_flags);
  };
  services.unit=[this](std::uint32_t datum) {
    // Current-unit production owns this valid full native datum. The original
    // raw object-header projection and malformed/stale unit inputs are not
    // silently replaced by an unrelated participant or a new allocation.
    const auto* current=unit_from_datum(datum);
    if(!current)throw FormatError("Source powerup owner lost its current native unit");
    auto& unit=*const_cast<Unit*>(current);auto& fade=unit.source_camouflage;
    // Native UDP state is separate from the Xbox replication entity+D4.
    return SourceCamouflageUnitView{unit.source_action_word,fade.primary,fade.secondary,
        fade.fade_speed,null_datum};
  };
  return services;
}
void MultiplayerMatch::grant_powerup(unsigned id,std::int16_t index,std::int16_t ticks) {
  NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;
  const auto& player=players_.at(id);
  if(!player.connected || player.source_current_object==null_datum)
    throw FormatError("Native powerup grant requires a current participant unit");
  source_powerup_grant(player.source_datum,index,ticks,camouflage_services());
}
const MultiplayerPlayer *MultiplayerMatch::player_from_datum(std::uint32_t datum) const {
  if (player_pool_.lookup(datum).empty()) return nullptr;
  const auto &player = players_[datum & 0xffffu];
  return player.connected && player.source_datum == datum ? &player : nullptr;
}
const Unit *MultiplayerMatch::unit_from_datum(std::uint32_t datum) const {
  const auto header = simulation_.object_lifetimes_.header(datum);
  if (header.empty() || header[3] > 1) return nullptr;
  for (unsigned i = 0; i < players_.size(); ++i)
    if (unit(i).source_datum == datum) return &unit(i);
  const auto found = retained_units_.find(datum);
  return found == retained_units_.end() ? nullptr : &found->second;
}
std::uint32_t MultiplayerMatch::allocate_object(std::uint8_t type) {
  return simulation_.object_lifetimes_.reserve(type);
}
ObjectGarbageState &MultiplayerMatch::object_garbage(std::uint32_t datum) {
  const auto *unit = unit_from_datum(datum);
  if (!unit) throw FormatError("Garbage list received an absent full unit datum");
  return const_cast<Unit *>(unit)->source_garbage;
}
void MultiplayerMatch::finish_death(Unit &unit, MultiplayerPlayer &player,
                                   std::uint32_t tick) {
  if(source_object_world_) {
    source_object_world_->deliver_vitality(unit.source_datum,
        {unit.vitality.body,unit.vitality.shield,unit.damage_feedback});
    source_object_world_->kill_biped(unit.source_datum);
  }
  corpses_.insert_or_assign(unit.source_datum,MultiplayerState::Corpse{
      unit.source_datum,unit.definition.datum,player.source_datum,tick,
      player.source_configuration,unit.body.velocity});
  // Executed CBDC0 prefix: garbage admission precedes original owner detach;
  // the full header/body survive. Later animation/world deletion is unresolved.
  drop_carried_weapons(unit);
  const auto header = simulation_.object_lifetimes_.header(unit.source_datum);
  if (header.empty()) throw FormatError("Death cleanup received a stale unit datum");
  schedule_object_garbage(garbage_queue_, unit.source_datum, unit.source_garbage,
      (header[2] & 1) != 0, true, tick, 30 * 10,
      [this](std::uint32_t datum) -> ObjectGarbageState & { return object_garbage(datum); });
  player.source_previous_unit = player.source_current_object;
  player.source_current_object = null_datum;
  player.source_current_unit = false;
  unit.source_previous_player = unit.source_player;
  unit.source_player = null_datum;
  unit.source_death_tick = tick;
  player.death_counted = true;
  // Direct weapon damage completes death synchronously; environmental deaths
  // use the later sweep. Convert at their shared boundary, after retaining
  // the human corpse's appearance and before the next spawn chooses a loadout.
  if(rules_.custom==CustomSlayer::infection && !results_finalized_)
    infect(player.source_datum&0xffffu);
}
void MultiplayerMatch::retain_unit(Unit &unit) {
  if (unit.source_datum != null_datum)
    retained_units_.insert_or_assign(unit.source_datum, unit);
}
void MultiplayerMatch::synchronize_source_equipment(Unit &unit,const SourcePlayerEquipmentTu15 &source) {
  unit.grenades=source.grenades;
  std::array<std::optional<WeaponState>,4> states;
  for(unsigned slot=0;slot<4;++slot) {
    const auto handle=source.weapons[slot];if(handle==null_datum)continue;
    for(unsigned prior=0;prior<slot;++prior)if(source.weapons[prior]==handle)
      throw FormatError("Source inventory contains a duplicate full weapon identity");
    if(!simulation_.object_lifetimes_.contains(handle))simulation_.object_lifetimes_.observe_source_birth(handle);
    if(unit.weapon.source_datum==handle)states[slot]=std::move(unit.weapon);
    else for(auto &old:unit.source_inventory.inactive_weapons)
      if(old && old->source_datum==handle) {states[slot]=std::move(old);break;}
    if(!states[slot])for(auto &item:weapon_pickups_)
      if(item.handle==handle && item.weapon){states[slot]=std::move(item.weapon);item.weapon.reset();break;}
    if(!states[slot])states[slot]=native_weapon_state(content_.weapons.at(source.definitions[slot]),handle);
    if(states[slot]->definition!=source.definitions[slot])
      throw FormatError("Source inventory changed the definition of a live weapon identity");
    auto &state=*states[slot];
    const auto &definition=content_.weapons.at(source.definitions[slot]);
    const auto body=source_object_world_->object_body(handle);
    const auto &memory=source_object_world_->memory();
    const auto read=[&]<class T>(unsigned offset,T &value) {
      static_assert(std::endian::native==std::endian::little);
      const auto bytes=memory.view(body+offset,sizeof(T));
      std::memcpy(&value,bytes.data(),sizeof(T));
    };
    // The fixed52/12/16-byte runtime banks belong to the original constructor
    // and1021D0/1030F0 update. Preserve both banks even when only one is used.
    read(0x1a4,state.source_barrels);read(0x20c,state.source_triggers);
    read(0x224,state.source_magazines);
    state.source_runtime_published=true;
    state.shader=source.shaders[slot];state.ready_clock=false;
    read(0x19e,state.ready_countdown);read(0x188,state.cooling_inhibitor);
    read(0x175,state.charge_fraction);
    read(0x244,state.overheat_handle);read(0x17c,state.energy_blocking_ticks);
    read(0x178,state.smg.weapon_state);read(0x16c,state.smg.weapon_flags);
    read(0x16e,state.smg.input_flags);
    state.smg.trigger_state=state.source_triggers[0].mode;
    state.smg.trigger_ticks=state.source_triggers[0].ticks;
    state.smg.trigger_flags=state.source_triggers[0].flags;
    state.smg.trigger_effect_handle=state.source_triggers[0].effect;
    state.smg.second_trigger_state=state.source_triggers[1].mode;
    state.loaded=state.reserve=0;
    if(!definition.triggers.empty() && definition.triggers[0].primary_barrel>=0) {
      const auto barrel=unsigned(definition.triggers[0].primary_barrel);
      if(barrel>=state.source_barrels.size() || barrel>=definition.barrel_ammunition.size())
        throw FormatError("Original primary trigger selects an unavailable barrel");
      state.smg.barrel=state.source_barrels[barrel];
      state.alternate_barrel=state.source_barrels[1u-barrel];
      const auto magazine=definition.barrel_ammunition[barrel].magazine;
      if(magazine>=0) {
        if(unsigned(magazine)>=state.source_magazines.size())
          throw FormatError("Original barrel selects an unavailable magazine");
        state.magazine=state.source_magazines[unsigned(magazine)];
        state.loaded=state.magazine.loaded;state.reserve=state.magazine.reserve;
        state.smg.loaded=state.magazine.loaded;state.smg.reserve=state.magazine.reserve;
        state.smg.magazine_mode=state.magazine.mode;state.smg.magazine_ticks=state.magazine.ticks;
        state.smg.transfer_ticks=state.magazine.transfer_ticks;
        state.smg.magazine_other_ticks=state.magazine.other_ticks;
      }
    }
  }
  int primary=source.current[0];
  if(primary<0)primary=source.requested[0];
  if(primary<0)for(unsigned slot=0;slot<4;++slot)if(states[slot]) {primary=int(slot);break;}
  if(primary>=4 || (primary>=0 && !states[unsigned(primary)]))
    throw FormatError("Source primary hand selects an unavailable inventory weapon");
  // Original inventory actions can detach a weapon during14FEC0. Preserve
  // the existing native presentation state while adopting its actual root;
  // this does not invoke a second detach or assign source motion.
  const auto dropped=[&](WeaponState &old) {
    if(old.source_datum==null_datum || std::find(source.weapons.begin(),source.weapons.end(),old.source_datum)!=source.weapons.end())return;
    if(simulation_.object_lifetimes_.header(old.source_datum).empty() ||
        source_object_world_->object_parent(old.source_datum)!=null_datum)return;
    auto item=std::find_if(weapon_pickups_.begin(),weapon_pickups_.end(),[&](const auto &p){return p.handle==old.source_datum;});
    if(item==weapon_pickups_.end()){weapon_pickups_.push_back({});item=std::prev(weapon_pickups_.end());}
    item->handle=old.source_datum;item->definition=old.definition;
    item->unlock_tick=std::uint32_t(simulation_.ticks());item->weapon=std::move(old);
  };
  dropped(unit.weapon);
  for(auto &old:unit.source_inventory.inactive_weapons)if(old)dropped(*old);
  unit.weapon=primary<0?WeaponState{}:std::move(*states[unsigned(primary)]);
  if(primary>=0)states[unsigned(primary)].reset();
  auto &inventory=unit.source_inventory;
  inventory.slots=source.weapons;inventory.current=source.current;inventory.requested=source.requested;
  inventory.inventory_sequence=source.inventory_sequence;inventory.replicated_sequence=source.replicated_sequence;
  inventory.ready_ticks=source.ready_ticks;inventory.inactive_weapons=std::move(states);
}
void MultiplayerMatch::configure_local_views(std::span<const unsigned> ids,
                                            const FirstPersonBindingDefinition &definition) {
  NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;
  configure_local_views(ids,std::span(&definition,1));
}
void MultiplayerMatch::configure_local_views(std::span<const unsigned> ids,
    std::span<const FirstPersonBindingDefinition> definitions) {
  NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;
  if(ids.size()>first_person_views_.size())
    throw FormatError("Too many local first-person views");
  if(std::set<unsigned>(ids.begin(),ids.end()).size()!=ids.size())
    throw FormatError("A source player cannot own two local views");
  std::map<std::pair<unsigned,std::uint32_t>,FirstPersonBindingDefinition> selected;
  for(const auto &definition:definitions) {
    if(!content_.weapons.contains(definition.weapon_definition) ||
        (source_game_context_.game_type==2 ? (definition.representation!=2 && definition.representation!=3) :
          (definition.representation<0 || unsigned(definition.representation)>=content_.player_representations.size() ||
           content_.player_representations[unsigned(definition.representation)].biped.datum==null_datum)) ||
        !selected.emplace(std::pair{unsigned(definition.representation),definition.weapon_definition},definition).second)
      throw FormatError("Invalid or duplicate local first-person representation");
  }
  for(auto id:ids) {
    if(id>=players_.size() || !players_[id].connected)
      throw FormatError("Local first-person view has no selected player representation");
    const auto representation=source_session_player_representation(players_[id].source_configuration,source_game_context_.game_type);
    if(std::none_of(selected.begin(),selected.end(),[&](const auto& definition) {
          return definition.first.first==representation;
        }) || (players_[id].source_current_object!=null_datum &&
        unit(id).weapon.definition!=null_datum &&
        !selected.contains({representation,unit(id).weapon.definition})))
      throw FormatError("Local first-person view has no selected player representation");
    // Original TU1678C1+61 (167922/167926) skips inventory and hand
    // registration while player+2C/current unit is NONE. Retain the genuine
    // deferred birth without substituting a starting weapon or spawning here.
  }
  if(simulation_.source_physics_movement_) {
    for(auto id:first_person_players_)if(players_[id].connected)
      source_object_world_->set_local_player(players_[id].source_datum,-1);
    for(unsigned view=0;view<ids.size();++view)
      source_object_world_->set_local_player(players_[ids[view]].source_datum,std::int16_t(view));
  }
  for(auto &view:first_person_views_)release_first_person_binding(view);
  first_person_players_.assign(ids.begin(),ids.end());first_person_definitions_=std::move(selected);
  if(first_person_definitions_.empty())for(unsigned id=0;id<players_.size();++id)for(unsigned hand=0;hand<2;++hand)
    if(auto *weapon=native_hand_weapon(unit_mutable(id),hand))weapon->smg.hand_timer_registered=false;
  update_first_person_views();
  if(simulation_.source_physics_movement_) {
    first_person_clock_origins_=first_person_views_;
    for(unsigned view=0;view<first_person_players_.size();++view) {
      const auto id=first_person_players_[view];
      replicated_fp_origins_[id]=first_person_views_[view];
      replicated_fp_views_[id]=first_person_views_[view];
    }
  }
}
void MultiplayerMatch::update_first_person_views() {
  if(first_person_definitions_.empty())return;
  for(unsigned v=0;v<first_person_players_.size();++v) {
    const auto id=first_person_players_[v];const auto &player=players_[id];
    if(!player.connected) {release_first_person_binding(first_person_views_[v]);continue;}
    if(simulation_.source_physics_movement_) {
      first_person_views_[v]=source_object_world_->first_person_view(v);
      if(first_person_views_[v].unit!=player.source_current_object)
        throw FormatError("Original first-person view and player disagree about the current unit");
      continue;
    }
    FirstPersonBindingInput input;input.player=player.source_datum;input.unit=player.source_current_object;
    input.representation=source_session_player_representation(player.source_configuration,source_game_context_.game_type);
    if(input.unit!=null_datum)for(unsigned h=0;h<2;++h)
      if(const auto *w=native_hand_weapon(unit(id),h))input.weapons[h]=w->source_datum;
    const auto previous=first_person_views_[v];
    observe_first_person_binding(first_person_views_[v],input,[&](auto datum,auto representation)->const FirstPersonBindingDefinition * {
      const auto *w=weapon_from_datum(datum);if(!w)return nullptr;
      const auto found=first_person_definitions_.find({unsigned(representation),w->definition});
      return found==first_person_definitions_.end()?nullptr:&found->second;
    });
    if(weapon_pickups_configured_ && simulation_.ticks()>0)for(unsigned h=0;h<2;++h) {
      auto &hand=first_person_views_[v].hands[h];
      if(hand.weapon!=null_datum && hand.weapon!=previous.hands[h].weapon) {
        const auto *w=weapon_from_datum(hand.weapon);
        first_person_activity(hand,first_person_definitions_.at({unsigned(input.representation),w->definition}),0x05000024);
      }
    }
  }
  for(unsigned id=0;id<players_.size();++id)for(unsigned h=0;h<2;++h)
    if(auto *w=native_hand_weapon(unit_mutable(id),h)) {
      w->smg.hand_timer_registered=false;
      if(!players_[id].connected || players_[id].source_current_object==null_datum)continue;
      const auto view=first_person_weapon_view(first_person_views_,w->source_datum);
      if(view>=0 && unsigned(view)<first_person_players_.size())
        w->smg.hand_timer_registered=first_person_views_[view].unit==unit(id).source_datum;
    }
}
void MultiplayerMatch::configure_replicated_first_person(const FirstPersonBindingDefinition &definition) {
  NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;
  const auto key=std::pair{unsigned(definition.representation),definition.weapon_definition};
  if(simulation_.ticks()!=0 || !content_.weapons.contains(definition.weapon_definition) ||
      (source_game_context_.game_type==2 ? (definition.representation!=2 && definition.representation!=3) :
          (definition.representation<0 || unsigned(definition.representation)>=content_.player_representations.size() ||
           content_.player_representations[unsigned(definition.representation)].biped.datum==null_datum)) || replicated_fp_definitions_.contains(key))
    throw FormatError("Invalid initial replicated first-person definition");
  replicated_fp_definitions_.emplace(key,definition);update_replicated_first_person_views();
}
void MultiplayerMatch::update_replicated_first_person_views() {
  if(replicated_fp_definitions_.empty())return;
  for(unsigned id=0;id<players_.size();++id) {
    const auto &player=players_[id];
    if(!player.connected) {release_first_person_binding(replicated_fp_views_[id]);continue;}
    FirstPersonBindingInput input;input.player=player.source_datum;input.unit=player.source_current_object;
    input.representation=source_session_player_representation(player.source_configuration,source_game_context_.game_type);
    if(input.unit!=null_datum)for(unsigned h=0;h<2;++h)
      if(const auto *w=native_hand_weapon(unit(id),h))input.weapons[h]=w->source_datum;
    const auto previous=replicated_fp_views_[id];
    observe_first_person_binding(replicated_fp_views_[id],input,[&](auto datum,auto representation)->const FirstPersonBindingDefinition * {
      const auto *w=weapon_from_datum(datum);if(!w)return nullptr;
      const auto found=replicated_fp_definitions_.find({unsigned(representation),w->definition});
      return found==replicated_fp_definitions_.end()?nullptr:&found->second;
    });
    if(weapon_pickups_configured_ && simulation_.ticks()>0)for(unsigned h=0;h<2;++h) {
      auto &hand=replicated_fp_views_[id].hands[h];
      if(hand.weapon!=null_datum && hand.weapon!=previous.hands[h].weapon) {
        const auto *w=weapon_from_datum(hand.weapon);
        first_person_activity(hand,replicated_fp_definitions_.at({unsigned(input.representation),w->definition}),0x05000024);
      }
    }
  }
}
void MultiplayerMatch::owner_counter_event(const Unit &victim, const DamageOwner &owner,
                                          std::int16_t damage_class) {
  if(!victim.vitality.dead || (owner.player==null_datum && owner.unit==null_datum)) return;
  std::vector<OwnerCounterObject> objects;
  std::vector<Unit *> bodies;
  auto add=[&](Unit &unit) {
    const auto header=simulation_.object_lifetimes_.header(unit.source_datum);
    if(header.empty() || std::any_of(objects.begin(),objects.end(),[&](const auto &object) {
         return object.datum==unit.source_datum;
       })) return;
    objects.push_back({unit.source_datum,unit.source_player,unit.source_forward_9,
        unit.source_forward_other,header[3],true,unit.vitality.dead,unit.source_owner_counter});
    bodies.push_back(&unit);
  };
  for(unsigned i=0;i<players_.size();++i) add(unit_mutable(i));
  for(auto &[datum,unit]:retained_units_) add(unit);
  std::array<OwnerCounterPlayer,multiplayer_capacity> players{};unsigned count=0;
  for(const auto &player:players_)
    if(player.connected && player_from_datum(player.source_datum))
      players[count++]={player.source_datum,player.source_current_object};
  OwnerCounterInput input;
  input.tick=std::uint32_t(simulation_.ticks());input.seconds_per_tick=tick_seconds;
  input.incoming_player=owner.player;input.incoming_object=owner.unit;
  input.incoming_designator=owner.designator;input.victim_membership=victim.source_membership;
  input.damage_class=damage_class;input.lethal=1;
  // Registered original1CB3D0 explicitly clears AI+1 for multiplayer state2.
  input.enabled_cap=0;
  const auto selected=incoming_owner_counter(input,std::span(players.data(),count),objects);
  if(selected) bodies[*selected]->source_owner_counter=objects[*selected].counter;
}
void MultiplayerMatch::damage_event(Unit &victim, const DamageOwner &owner,
                                   const UnitDamageResult &amount, std::int16_t damage_class) {
  const auto full_victim = victim.source_player;
  const auto *victim_player = player_from_datum(full_victim);
  // Original finalizer clears stat-bank+0's validity byte. The attribution
  // reader then returns without stats/respawn callbacks; history, incoming
  // unit counters and ownership cleanup continue in the world pass.
  if (victim.vitality.dead && victim_player && !results_finalized_) {
    std::array<AttributionPlayer, multiplayer_capacity> live{};
    unsigned count = 0;
    for (const auto &player : players_)
      if (player.connected && player_from_datum(player.source_datum))
        live[count++] = {player.source_datum, player.source_membership};
    // This production callback supports ordinary Slayer stock SMG impacts.
    // Hit finding and normalization are the declared live-world boundaries;
    // the source selector receives previous history before this hit is written.
    LethalAttributionInput input;
    input.tick = std::uint32_t(simulation_.ticks());
    input.incoming_player = owner.player; input.victim_player = full_victim;
    input.result_flags = 1; input.incoming_designator = owner.designator;
    input.victim_membership = victim_player->source_membership;
    input.cause = 8; input.mode = 2; input.unit_type = 0;
    input.engine_present = input.statistics_valid = true;
    const auto attribution = lethal_attribution(input, victim.source_damage_history,
        std::span(live.data(), count));
    if (!attribution) throw FormatError("Ordinary SMG lethal attribution was rejected");
    const auto &result = *attribution;
    if (const auto *credited = player_from_datum(result.credited_player)) {
      auto &player = players_[credited->source_datum & 0xffffu];
      if (result.credited_kill) player.kills=record_credited_kill(credited->source_datum&0xffffu,result.cause);
      else if (result.nonenemy_credit) ++player.nonenemy_credit;
    }
    // Original 15E4D0 receives SELECTED credit. Its suicide penalty differs
    // from statistic4, which tests the incoming damage owner against the victim.
    prepare_respawn(full_victim & 0xffffu, result.credited_player, std::uint32_t(simulation_.ticks()));
    game_engine_death(result.credited_player,full_victim,result.relationship);
    for (unsigned i = 0; i < result.assist_count; ++i)
      if (const auto *player = player_from_datum(result.assists[i]))
        players_[player->source_datum & 0xffffu].assists=increment_stat(player->source_datum&0xffffu,6,8);
    auto &player = players_[full_victim & 0xffffu];
    if (result.victim_death)record_victim_death(full_victim&0xffffu,result.credited_player,
        result.suicide,result.relationship,result.cause);
    player.attribution_counted = true;
    player.incoming_suicide = result.suicide;
  }
  record_damage_result_history(victim.source_damage_history,
      std::uint32_t(simulation_.ticks()), amount.body, amount.shield,
      owner.unit, owner.player, 8);
  // Complete source CBA60 records first, then increments the incoming owner's
  // counter while player/current-unit bindings still describe this fatal hit.
  owner_counter_event(victim, owner, damage_class);
  if (victim.vitality.dead && victim_player && (victim_player->attribution_counted || results_finalized_) &&
      !victim_player->death_counted)
    finish_death(victim, players_[full_victim & 0xffffu], std::uint32_t(simulation_.ticks()));
}
SourcePlayerConfiguration MultiplayerMatch::prepare_player_configuration(
    const SourcePlayerConfiguration& profile,std::int8_t selected_team)const {
    auto sanitized=profile;
    if(rules_.team_mode && std::bit_cast<std::int8_t>(sanitized[0x7c])!=selected_team)
      throw FormatError("Selected profile team differs from the session selection");
    SourcePlayerConfigurationContext context;
    context.game_type=source_game_context_.game_type;context.representation_count=std::int32_t(content_.player_representation_count);
    context.scenario_representations=content_.player_start_representations;
    context.selected_engine=true;context.team_mode=rules_.team_mode;context.team_mask=0xff;
    context.representation_policy=content_.original_data && content_.original_data->image!=OriginalData::Image::Base
        ? SourceProfileRepresentationPolicy::MultiplayerMapping : SourceProfileRepresentationPolicy::Retain;
    source_sanitize_player_configuration(sanitized,context);
    multiplayer_biped(content_,source_session_player_representation(sanitized,source_game_context_.game_type));
    return sanitized;
}
void MultiplayerMatch::validate_player_configuration(const SourcePlayerConfiguration& profile,
    std::int8_t selected_team)const {
  prepare_player_configuration(profile,selected_team);
}
void MultiplayerMatch::set_source_player_activation_owner(bool deferred,
    std::function<void(MultiplayerPlayer&,unsigned)> birth,
    std::function<void(MultiplayerMatch&)> update) {
  NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;
  if(!deferred || !birth || !update || rules_.team_mode ||
      std::any_of(players_.begin(),players_.end(),[](const auto& p){return p.connected;}))
    throw FormatError("Source player activation requires a complete pre-join local FFA policy");
  defer_initial_player_spawn_=deferred;player_birth_owner_=std::move(birth);
  pre_player_update_owner_=std::move(update);
}
void MultiplayerMatch::apply_source_player_activation(std::uint32_t datum,
    std::uint16_t expected,std::uint16_t after) {
  NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;
  if(!player_birth_owner_ || datum==null_datum || (datum&0xffffu)>=players_.size())
    throw FormatError("Source player activation requires its actual allocated owner");
  auto& player=players_[datum&0xffffu];
  if(!player.connected || player.source_datum!=datum || player.source_flags!=expected ||
      ((expected^after)&0xfffeu)!=0)
    throw FormatError("Source player activation rejected a stale handle or unrelated flag write");
  player.source_flags=after;
}
unsigned MultiplayerMatch::join(std::string name, std::int8_t selected_team,
    std::optional<SourcePlayerConfiguration> profile, std::optional<unsigned> admitted_slot) {
  NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;
  if (finished_)
    throw FormatError("Match has ended");
  if(rules_.team_mode && (selected_team < 0 || selected_team > 7))
    throw FormatError("Team Slayer requires an explicitly selected source team 0..7");
  const auto sanitized=(profile || source_game_context_.game_type==1) ?
      std::optional(prepare_player_configuration(profile.value_or(source_default_player_configuration()),selected_team)) : std::nullopt;
  for (unsigned i = 0; i < rules_.maximum_players; ++i)
    if (!players_[i].connected && (!admitted_slot || *admitted_slot == i)) {
      players_[i] = {};
      end_stats_.players[i] = {};
      if(rules_.team_mode && source_object_world_ && source_object_world_->owns_physics())
        source_object_world_->publish_roster_team(i,selected_team);
      players_[i].source_datum = player_pool_.allocate_at(std::int32_t(i));
      if (players_[i].source_datum == null_datum)
        throw FormatError("Original player datum allocation failed");
      players_[i].connected = true;
      players_[i].name = std::move(name);
      players_[i].source_profile_request=profile;
      players_[i].source_configuration=sanitized;
      players_[i].source_strength=sanitized ? (*sanitized)[0x7d] : source_default_player_configuration()[0x7d];
      source_initialize_player_lives(players_[i].source_lives_remaining,rules_.life_limit,true,0);
      players_[i].source_ffa_identity = std::uint8_t(i);
      players_[i].source_membership = source_game_context_.game_type==1 ?
          std::bit_cast<std::int8_t>((*sanitized)[0x7c]) : (rules_.team_mode ? selected_team : std::int8_t(i));
      if(players_[i].source_configuration)
        (*players_[i].source_configuration)[0x7c]=std::uint8_t(players_[i].source_membership);
      if(source_object_world_) {
        auto configuration=players_[i].source_configuration.value_or(source_default_player_configuration());
        configuration[0x7c]=std::uint8_t(players_[i].source_membership);
        source_object_world_->construct_player(players_[i].source_datum,i,configuration);
      }
      if (rules_.team_mode) {
        const std::array<std::int8_t,1> source_roster{selected_team};
        // Original Slayer slot4 admits eight map designators; maximum-teams
        // default0 pads a real session roster to eight logical teams. Slot26
        // returns NONE, producing the original identity mapping.
        team_mapping_ = source_team_mapping(0xff, source_session_team_mask(0xff, 0, source_roster));
        std::array<TeamRespawnPlayer,multiplayer_capacity> present{};unsigned count=0;
        for(const auto &player:players_)if(player.connected)
          present[count++]={player.source_membership,player.source_flags,player.respawn.countdown};
        retained_team_mask_ = std::uint8_t(source_team_presence(retained_team_mask_,
            std::span(present.data(),count)).retained);
      }
      // A failed initial selection leaves the participant without a unit.
      // This native absent-unit storage is not a newly counted player death.
      auto &absent = unit_mutable(i);
      absent = {};
      absent.exists = false;
      absent.vitality.dead = true;
      players_[i].death_counted = true;
      if(player_birth_owner_) {
        const auto allocated=players_[i].source_datum;
        try {
          player_birth_owner_(players_[i],i);
          if(!players_[i].connected || players_[i].source_datum!=allocated ||
              players_[i].source_flags!=8 || players_[i].source_current_object!=null_datum ||
              players_[i].source_current_unit)
            throw FormatError("Source local birth owner did not preserve fresh absent-unit state");
        } catch(...) {
          player_pool_.release_unchecked(allocated);players_[i]={};
          end_stats_.players[i]={};throw;
        }
      }
      infection_zombies_&=std::uint16_t(~(1u<<i));
      if(!defer_initial_player_spawn_)spawn(i, true);
      if(rules_.custom==CustomSlayer::infection && infection_started_)infect(i);
      // Native presentation creation follows a genuine joined current unit.
      // It does not install a dedicated-server observer or hand-feedback owner.
      update_replicated_first_person_views();
      return i;
    }
  throw FormatError("Multiplayer match is full");
}
unsigned MultiplayerMatch::available_player_slots() const {
  unsigned count=0;
  for(unsigned i=0;i<rules_.maximum_players;++i)count+=!players_[i].connected;
  return count;
}
void MultiplayerMatch::leave(unsigned id) {
  if(simulation_.source_physics_movement_) {depart(id);return;}
  // The older native-owned compatibility world has no original deferred
  // departure owner yet. Keep its existing behavior isolated from TU1.5.
  cancel_join(id);
}
bool MultiplayerMatch::depart(unsigned id) {
  SourceExecutionCallScope journal(source_object_world_?&source_object_world_->memory():nullptr,
      SourceExecutionCallKind::DepartPlayer,[&]{return source_execution_arguments(id);});
  NativeAuthorityBoundary::Call active(authority_calls_);
  auto &p=players_.at(id);
  if(!p.connected || (p.source_flags&2u))return false;
  if(!simulation_.source_physics_movement_)
    throw FormatError("Retained departure requires the actual TU1.5 world");
  ++authority_calls_.mutations;
  if(!source_object_world_->depart_player(p.source_datum))
    throw FormatError("Native Match departure disagrees with its original player owner");
  SourceDeparturePlayer raw;
  const auto bytes=player_pool_.lookup(p.source_datum);
  if(bytes.size()!=sizeof(raw))throw FormatError("Departure lost its retained source player record");
  std::memcpy(&raw,bytes.data(),sizeof(raw));
  p.source_flags=raw.flags;p.source_current_object=raw.current_unit;
  p.source_current_unit=raw.current_unit!=null_datum;p.source_previous_unit=raw.previous_unit;
  // This latch excludes the later original unowned-unit death from the native
  // weapon/environmental death accountant. No kill/death/score is fabricated.
  p.death_counted=true;
  auto &u=unit_mutable(id);
  if(u.exists && u.source_datum!=null_datum) {
    const auto body=source_object_world_->object_body(u.source_datum);
    auto &bank=source_object_world_->memory();
    bank.read(body+0x13cu,std::as_writable_bytes(std::span(&u.source_player,1)));
    bank.read(body+0x140u,std::as_writable_bytes(std::span(&u.source_previous_player,1)));
  }
  native_presentation_controls_[id]={};
  update_first_person_views();update_replicated_first_person_views();
  return true;
}
void MultiplayerMatch::cancel_join(unsigned id) {
  NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;
  auto &player = players_.at(id);
  if (!player.connected) return;
  if(simulation_.source_physics_movement_)
    source_object_world_->set_local_player(player.source_datum,-1);
  drop_carried_weapons(unit_mutable(id));
  unit_mutable(id).source_previous_player = unit_mutable(id).source_player;
  unit_mutable(id).source_player = null_datum;
  player.source_previous_unit = player.source_current_object;
  player.source_current_object = null_datum;
  player.source_current_unit = false;
  retain_unit(unit_mutable(id));
  player_pool_.release_unchecked(player.source_datum);
  // Original14DCB0/16D4C0 with the actual player-pool flags0004 clears salt
  // and allocation, retaining these bytes until16D410 allocates the slot again.
  // Captured projectile damage still reads them through its raw low16 index.
  const auto strength=player.source_strength;
  const auto membership=player.source_membership;
  const auto field_194=player.source_field_194;
  const auto inside=player.source_king_inside_ticks;
  const auto powerup_timers=player.source_powerup_timers;
  players_.at(id) = {};
  player.source_strength=strength;player.source_membership=membership;
  player.source_field_194=field_194;player.source_king_inside_ticks=inside;
  player.source_powerup_timers=powerup_timers;
  unit_mutable(id).exists = false;
}
bool MultiplayerMatch::spawn(unsigned id, bool initial) {
  const Spawn *selected = nullptr;
  std::optional<Spawn> campaign_input;
  if(source_game_context_.game_type==1) {
    if(!source_spawn_input_)throw FormatError("Campaign player activation requires its original or declared spawn input owner");
    campaign_input=source_spawn_input_(id,initial);
    if(campaign_input)selected=&*campaign_input;
  } else if (content_.multiplayer_spawn) {
    const auto &settings = *content_.multiplayer_spawn;
    const auto &source_zones = initial ? settings.source_initial_zones : settings.source_respawn_zones;
    auto influences = source_zones.empty() ?
        (initial ? settings.initial_zones : settings.respawn_zones) :
        source_spawn_zone_bank(source_zones, unsigned(rules_.source_engine), rules_.team_mode, players_[id].source_membership,
            team_mapping_, [](const auto &) { return true; }); // Same original Slayer/King slot33.
    // Original static/player order and 128-entry limit. The currently absent
    // vehicle/explosive-object and death-marker producers remain open inputs.
    for (unsigned j = 0; j < players_.size() && influences.size() < 128; ++j) {
      if (j == id || !players_[j].connected || (players_[j].source_flags & 2u) ||
          !unit(j).exists || unit(j).vitality.dead) continue;
      const auto &type = settings.types[players_[j].source_membership ==
          players_[id].source_membership ? 1 : 0];
      influences.push_back({unit(j).body.position, type.inner, type.outer,
          settings.upper, settings.lower, type.weight});
    }
    // Original collector appends registered engine slot32 after its ordinary
    // influences. King uses authored spawn types8 then7 at the retained center.
    if(rules_.source_engine==4 && king_state_.marker_count>=4 && !(king_state_.marker_count&1))
      for(const unsigned kind:{8u,7u})if(influences.size()<128) {
        const auto& type=settings.types[kind];
        influences.push_back({{king_state_.center[0],king_state_.center[1],king_state_.center[2]},
            type.inner,type.outer,settings.upper,settings.lower,type.weight});
      }
    std::vector<Vec3> points;
    for (const auto &start : spawns_) points.push_back(start.position);
    auto seed = spawn_private_seed(players_[id].source_datum, rules_.random_seed,
        rules_.spawn_salt, players_[id].source_spawn_count);
    const auto choice = choose_spawn(points, influences, settings.random_bias, seed,
        [](std::size_t, Vec3) {
          // The proved clear-contact boundary accepts authored positions;
          // player setup does not filter the start's BSP metadata. Live
          // original shape/Havok admission remains a separate producer.
          return true;
        });
    if (choice.index >= 0) selected = &spawns_[choice.index];
    if (selected) players_[id].source_spawn_count = std::bit_cast<std::int16_t>(
        std::uint16_t(std::uint16_t(players_[id].source_spawn_count) + 1u));
  } else if (content_.original_data) {
    throw FormatError("Missing original multiplayer spawn settings");
  } else {
    // Synthetic/non-original content fixture compatibility path.
  float best = -1;
  for (const auto &spawn : spawns_) {
    if (!slayer_spawn(spawn, simulation_.bsp_, initial))
      continue;
    float distance = std::numeric_limits<float>::max();
    for (unsigned j = 0; j < players_.size(); ++j)
      if (j != id && players_[j].connected && !unit(j).vitality.dead)
        distance =
            std::min(distance, length(spawn.position - unit(j).body.position));
    if (!selected || distance > best) {
      selected = &spawn;
      best = distance;
    }
  }
  }
  if (!selected)
    return false; // Preserve timer/current unit/count; retry in an eligible phase.
  auto &u = unit_mutable(id);
  const auto &biped=multiplayer_biped(content_,source_session_player_representation(players_[id].source_configuration,source_game_context_.game_type));
  const auto unit_datum = source_object_world_
      ? source_object_world_->create_biped(biped.datum,selected->position,
          authored_basis({selected->facing,0,0}).forward,
          source_object_world_->owns_physics()?players_[id].source_datum:null_datum)
      : allocate_object(0);
  if (unit_datum == null_datum) throw FormatError("Original shared object pool is full");
  if(source_object_world_)simulation_.object_lifetimes_.observe_source_birth(unit_datum);
  std::optional<SourcePlayerEquipmentTu15> source_equipment;
  if(source_object_world_ && source_object_world_->owns_physics()) {
    source_object_world_->bind_player(players_[id].source_datum,unit_datum);
    source_equipment=source_object_world_->initialize_player_equipment(players_[id].source_datum,unit_datum);
  }
  const auto weapon_datum = source_equipment?null_datum:allocate_object(2);
  if (!source_equipment && weapon_datum == null_datum) {
    if(source_object_world_)
      throw FormatError("Source biped birth completed but its native starting weapon has no slot");
    simulation_.object_lifetimes_.release(unit_datum);
    throw FormatError("Original shared object pool has no starting weapon slot");
  }
  retain_unit(u);
  u = {};
  u.source_datum = unit_datum;
  // Native active-unit registration supplies header active bit0 to BC840.
  // Complete original world/header initialization remains a separate producer.
  if(!source_object_world_)simulation_.object_lifetimes_.header(unit_datum)[2] |= 1;
  u.exists = true;
  u.source_player = players_[id].source_datum;
  u.source_membership = players_[id].source_membership;
  u.controlled = id != 0;
  u.definition = biped;
  u.team = 0;
  u.bsp = int(simulation_.bsp_);
  u.yaw = selected->facing;
  u.vitality.body = u.definition.vitality.health > 0 ? 1.f : 0.f;
  u.vitality.shield = u.definition.vitality.shield > 0 ? 1.f : 0.f;
  u.body = {selected->position,  {},    u.definition.height,
            u.definition.radius, false, u.definition.maximum_slope_cosine};
  // Original player setup and all 66 real Lockout constructors preserve these
  // authored XYZ bits. Contact solving belongs to later physics updates.
  const auto primary_option=rules_.custom==CustomSlayer::infection ?
      ((infection_zombies_&(1u<<id))?17u:9u):rules_.starting_weapon;
  const auto secondary_option=rules_.custom==CustomSlayer::infection ?
      ((infection_zombies_&(1u<<id))?1u:4u):rules_.secondary_starting_weapon;
  std::uint32_t starting_definition=rules_.weapon,secondary=null_datum;
  if(rules_.custom!=CustomSlayer::none) {
    if(primary_option==2) {
      secondary=source_starting_weapon(rules_.weapon,secondary_option,null_datum,rules_.weapon_choices,simulation_.random_state_);
      starting_definition=source_starting_weapon(rules_.weapon,primary_option,secondary,rules_.weapon_choices,simulation_.random_state_);
    } else {
      starting_definition=source_starting_weapon(rules_.weapon,primary_option,null_datum,rules_.weapon_choices,simulation_.random_state_);
      secondary=source_starting_weapon(rules_.weapon,secondary_option,starting_definition,rules_.weapon_choices,simulation_.random_state_);
    }
    if(starting_definition==null_datum || !content_.weapons.contains(starting_definition) || (secondary!=null_datum && !content_.weapons.contains(secondary)))
      throw FormatError("Custom Slayer starting_definition an unavailable authored weapon definition");
  }
  const auto &weapon = content_.weapons.at(starting_definition);
  // Original FDAD0: the initial total is divided between loaded and reserve
  // rounds. Inventory maximum is a cap, not additional starting ammunition.
  const int loaded = std::min(weapon.initial_rounds, weapon.capacity);
  const int reserve =
      std::min(weapon.initial_rounds - loaded, std::max(0, weapon.reserve));
  u.weapon = {starting_definition, loaded, reserve, 0, 0, 0};
  u.weapon.source_datum = weapon_datum;
  u.weapon.smg.loaded = loaded;
  u.weapon.smg.reserve = reserve;
  if (weapon.smg) smg_initial_equip(u.weapon.smg, *weapon.smg);
  if(rules_.custom!=CustomSlayer::none)u.weapon=native_weapon_state(weapon,weapon_datum);
  if(!source_equipment) {
    // Initial CE510 carry reaches the same FB120/FB360 attachment stores as
    // pickup: attached/retained and actual player-owned ammunition bits.
    u.weapon.shader.item_flags=5u|(u.source_player!=null_datum?8u:0u);
  }
  // Registration is produced by an actual configured local observer after
  // the world phases. Dedicated authority has no local first-person view.
  u.weapon.smg.hand_timer_registered = false;
  // Bounded original initial carry, not a general pickup/drop implementation:
  // CFED0 stores the allocated item at+218 and clears its timestamp; CFE60
  // advances the constructor's zero sequences to1 in measured FFA mode1.
  // CE510 requests slot0, then E9D50 stores current0 and the source clock+8
  // at+228 (E9EC1/E9ECB). Keep ammo and its clocks solely in Unit.weapon.
  u.source_inventory.slots[0] = weapon_datum;
  u.source_inventory.inventory_sequence = 1;
  u.source_inventory.replicated_sequence = 1;
  u.source_inventory.requested[0] = 0;
  u.source_inventory.current[0] = 0;
  u.source_inventory.ready_ticks[0] = std::uint32_t(simulation_.ticks());
  auto equipment_flags=rules_.source_engine==2 ? content_.multiplayer_starting_equipment_flags : std::nullopt;
  for(const auto& row:content_.multiplayer_starting_equipment) {
    NetgameEquipment selection;selection.modes=row.modes;
    if(source_netgame_equipment_selector(selection,true,rules_.source_engine)>=0) {
      equipment_flags=row.flags;break;
    }
  }
  if (equipment_flags) {
    // TU15A460 owns the sticky engine flag at lifecycle initialization; it is
    // not a count recomputed from the current roster at every spawn.
    u.grenades = starting_grenades(*equipment_flags,
        rules_.starting_grenades, rules_.maximum_one_grenade, int(rules_.weapon_set));
  }
  if(secondary!=null_datum && !source_equipment) {
    const auto handle=allocate_object(2);
    if(handle==null_datum)throw FormatError("No object slot for custom Slayer secondary weapon");
    u.source_inventory.inactive_weapons[1]=native_weapon_state(content_.weapons.at(secondary),handle);
    u.source_inventory.slots[1]=handle;
    simulation_.object_lifetimes_.publish_native(handle,secondary);
  }
  if(rules_.custom==CustomSlayer::infection && (infection_zombies_&(1u<<id))) {u.vitality.shield=0;u.definition.vitality.shield=0;u.grenades={};}
  if(source_equipment) {
    synchronize_source_equipment(u,*source_equipment);
    u.grenades=source_equipment->grenades;
  }
  players_[id].death_counted = false;
  players_[id].attribution_counted = players_[id].incoming_suicide = false;
  players_[id].source_current_unit = true;
  players_[id].source_current_object = unit_datum;
  if(source_object_world_ && !source_equipment)source_object_world_->bind_player(players_[id].source_datum,unit_datum);
  // Original1509C0 installs the signed source clock tick rate after a
  // successful spawn.15D160 decrements this field before world damage.
  players_[id].source_field_194=30;
  // Whole1509C0 clears both signed powerup timers on successful spawn.
  players_[id].source_powerup_timers={};
  players_[id].source_initial_spawn = false;
  if(player_birth_owner_)
    players_[id].source_flags=native_player_spawn_completed_flags(players_[id].source_flags);
  if (!id)
    simulation_.player_pitch_ = 0;
  if(!source_object_world_)simulation_.object_lifetimes_.publish_native(unit_datum,u.definition.datum);
  if(!source_equipment)simulation_.object_lifetimes_.publish_native(weapon_datum,u.weapon.definition);
  return true;
}
void MultiplayerMatch::apply_score(unsigned id, std::int32_t delta) {
  auto &player = players_.at(id);
  auto score = std::bit_cast<std::int16_t>(std::uint16_t(player.score));
  const auto result = slayer_score_write(score, delta, rules_.team_mode ? 0 : rules_.score_limit,
      true, true, !finished_, player.connected);
  if (!result.applied) return;
  player.score = score;
  end_stats_.players[id].fields[0] = score;
  round_lifecycle_.flags|=0x20;
  if (rules_.team_mode && player.source_membership != -1) {
    std::array<SlayerRankingPlayer, multiplayer_capacity> ranks{};unsigned count = 0;
    for (const auto &p : players_) if(p.connected)
      ranks[count++] = {p.source_datum, std::int16_t(p.score), p.source_flags, std::uint8_t(p.source_membership)};
    const auto team = unsigned(player.source_membership);
    const auto written = slayer_team_score_write(team_scores_.at(team), player.source_membership,
        delta, rules_.score_limit, rules_.team_scoring, std::span(ranks.data(),count));
    end_stats_.teams[team].fields[0] = team_scores_[team];
    if (written.end_match) end_match(-1,int(team));
  }
  if (result.end_match) end_match(int(id),-1);
}
void MultiplayerMatch::game_engine_death(std::uint32_t attacker,std::uint32_t victim,bool relationship) {
  const auto* victim_player=player_from_datum(victim);
  if(!victim_player)throw FormatError("Engine death callback lost its actual victim");
  if(rules_.source_engine==2) {
    const auto requests=slayer_death_scores(attacker,victim,relationship,victim_player->source_flags,
        rules_.suicide_loss,rules_.death_loss);
    for(unsigned n=0;n<requests.count;++n)if(player_from_datum(requests.values[n].player))
      apply_score(requests.values[n].player&0xffffu,requests.values[n].delta);
    return;
  }
  // King slot30 publishes retained stats27/28 through the shared recorder;
  // it does not turn kills into objective points or invent match-bank fields.
  const auto* credited=player_from_datum(attacker);
  const SourceKingPlayer victim_row{victim,victim_player->source_current_object,
      victim_player->source_membership,victim_player->source_king_inside_ticks};
  SourceKingPlayer attacker_row;
  if(credited)attacker_row={attacker,credited->source_current_object,credited->source_membership,credited->source_king_inside_ticks};
  source_king_death_statistics(victim_row,credited ? &attacker_row : nullptr,std::uint8_t(relationship),
      [&](std::uint32_t datum,int stat) {
        if(auto publication=result_publication())
          source_result_stat_add(*publication,stat,int(datum&0xffffu),rules_.team_mode ? credited->source_membership : -1,1);
      });
}
void MultiplayerMatch::update_game_engine() {
  // Shared15D7C0 refreshes camouflage before15D160 and before the selected
  // engine's occupancy callback. It consumes the retained previous inside
  // value. Whole139750's actual control owner selects nearest RC;30Hz*0.5
  // is exact. Reserved guest FPCW bits do not change this conversion.
  const auto camouflage=camouflage_services();
  for(auto& player:players_)if(player.connected) {
    source_camouflage_refresh(true,player.source_datum,30,0x023f,camouflage);
    if(player.source_field_194>0)--player.source_field_194;
  }
  if(rules_.source_engine==2)return; // Registered Slayer slot18 is empty.
  std::array<SourceKingPlayer,multiplayer_capacity> roster{};unsigned count=0;
  const SourceKingPosition position=[&](std::uint32_t datum) {
    const auto* current=unit_from_datum(datum);
    if(!current)throw FormatError("King occupancy lost its full current-unit datum");
    return std::array<float,3>{current->body.position.x,current->body.position.y,current->body.position.z};
  };
  // Original shared player callbacks precede slot18 and world-object movement.
  for(auto& p:players_)if(p.connected) {
    auto& row=roster[count++];row={p.source_datum,p.source_current_object,p.source_membership,p.source_king_inside_ticks};
    source_king_update_player(king_state_,row,game_over_.active,1,position);
    p.source_king_inside_ticks=row.inside_ticks;
  }
  const auto synchronize=[&] {
    for(unsigned n=0;n<count;++n)
      players_[roster[n].datum&0xffffu].source_king_inside_ticks=roster[n].inside_ticks;
  };
  SourceKingContext context;context.team_mode=rules_.team_mode;context.game_over=game_over_.active;
  context.lifecycle=round_lifecycle_.phase;context.status=round_lifecycle_.status;
  context.flags=rules_.king_flags;context.move_seconds=rules_.king_move_seconds;
  SourceKingWorldServices services;
  services.scoring.score=[&](std::uint32_t datum) {
    synchronize();const auto* p=player_from_datum(datum);
    if(!p)throw FormatError("King score getter lost its actual player");
    return rules_.team_mode ? (p->source_membership==-1 ? 0 : int(team_scores_.at(unsigned(p->source_membership)))) : p->score;
  };
  services.scoring.award=[&](std::uint32_t datum,std::int32_t delta) {
    synchronize();if(!player_from_datum(datum))throw FormatError("King award lost its actual player");
    apply_score(datum&0xffffu,delta);
  };
  services.scoring.record=[&](std::uint32_t datum,std::int32_t kind,std::uint32_t other) {
    synchronize();record_result_event(datum,std::uint32_t(kind),other);
    if(king_services_.record)king_services_.record(*this,datum,kind,other);
  };
  services.scoring.score_changed=[&](std::uint32_t datum,std::int32_t score) {
    synchronize();king_services_.score_changed(*this,datum,score);
  };
  services.notification=[&](const SourceKingNotice& notice) {
    synchronize();king_services_.notification(*this,notice);
  };
  try {
    source_king_world_update(king_state_,king_hills_,context,king_markers_,
        std::span(roster.data(),count),simulation_.random_state_,services);
  } catch(...) {synchronize();throw;}
  synchronize();
}
std::array<const std::uint64_t *,2> MultiplayerMatch::execution_call_counters()const{
  return {&authority_calls_.mutations,&simulation_.authority_calls_.mutations};
}
void MultiplayerMatch::advance_weather_view(double time,std::int16_t view){
  SourceExecutionCallScope journal(source_object_world_?&source_object_world_->memory():nullptr,
      SourceExecutionCallKind::WeatherView,[&]{return source_execution_arguments(time,view);});
  NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;simulation_.advance_weather_view(time,view);
}
void MultiplayerMatch::advance_audio(float seconds){
  SourceExecutionCallScope journal(source_object_world_?&source_object_world_->memory():nullptr,
      SourceExecutionCallKind::AdvanceAudio,[&]{return source_execution_arguments(seconds);});
  NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;simulation_.advance_audio(seconds);
}
std::vector<SoundEvent> MultiplayerMatch::take_sounds(){
  SourceExecutionCallScope journal(source_object_world_?&source_object_world_->memory():nullptr,
      SourceExecutionCallKind::TakeSounds,[]{return std::vector<std::byte>{};});
  NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;return simulation_.take_sounds();
}
std::vector<PropSandboxEvent> MultiplayerMatch::take_prop_events(){
  SourceExecutionCallScope journal(source_object_world_?&source_object_world_->memory():nullptr,
      SourceExecutionCallKind::TakePropEvents,[]{return std::vector<std::byte>{};});
  NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;return simulation_.take_prop_events();
}
std::vector<NativeGrenadeEvent> MultiplayerMatch::take_grenade_events(){
  SourceExecutionCallScope journal(source_object_world_?&source_object_world_->memory():nullptr,
      SourceExecutionCallKind::TakeGrenadeEvents,[]{return std::vector<std::byte>{};});
  NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;
  return simulation_.grenade_sandbox_?simulation_.grenade_sandbox_->take_events():std::vector<NativeGrenadeEvent>{};
}
std::uint32_t MultiplayerMatch::result_wall_time() const {
  const auto actual=[this]{return std::uint32_t(result_wall_clock_()+result_wall_adjustment_);};
  if(source_object_world_)return std::uint32_t(source_execution_input(source_object_world_->memory(),
      SourceExecutionInputKind::ResultWallSeconds,0,actual));
  return actual();
}
void MultiplayerMatch::record_result_event(std::uint32_t player,std::uint32_t first,std::uint32_t second) {
  SourceResultEventServices services;services.time=[this]{return result_wall_time();};
  services.position=[&](std::uint32_t datum)->std::optional<std::array<std::uint32_t,3>> {
    // Original19DE80 indexes the raw player slot. It falls back only when
    // current-unit is NONE, rather than when the current handle is invalid.
    const auto slot=datum&0xffffu;
    if(slot>=players_.size())throw FormatError("Result event player escapes its native slot backing");
    const auto& p=players_[slot];auto handle=p.source_current_object;
    if(handle==null_datum)handle=p.source_previous_unit;
    if(handle==null_datum)return std::nullopt;
    const auto* body=unit_from_datum(handle);
    if(!body)throw FormatError("Result event lost its actual current/previous unit");
    if(body->source_parent!=null_datum)
      throw FormatError("Result event requires the actual parent/node world transform");
    return std::array<std::uint32_t,3>{std::bit_cast<std::uint32_t>(body->body.position.x),
        std::bit_cast<std::uint32_t>(body->body.position.y),std::bit_cast<std::uint32_t>(body->body.position.z)};
  };
  source_result_event_record(result_events_,retained_recording_,player,first,second,services);
}
const SourceResultStatistics* MultiplayerMatch::recorded_statistics()const {
  return content_.original_data && content_.original_data->result_stat_descriptors ? &retained_stats_ : nullptr;
}
std::optional<SourceResultPublication> MultiplayerMatch::result_publication() {
  if(!recorded_statistics())return std::nullopt;
  return SourceResultPublication{retained_stats_,retained_recording_,*content_.original_data->result_stat_descriptors};
}
std::int16_t MultiplayerMatch::increment_stat(unsigned id,unsigned field,int stat) {
  auto publication=result_publication();
  return source_game_stat_player_add(end_stats_,id,field,1,stat,
      publication ? &*publication : nullptr,rules_.team_mode ? players_.at(id).source_membership : -1);
}
std::int16_t MultiplayerMatch::record_credited_kill(unsigned id,std::optional<std::uint8_t> cause) {
  const auto* constants=content_.original_data && content_.original_data->result_capture_constants ?
      &*content_.original_data->result_capture_constants : nullptr;
  // The synthetic native adapter has no imported cutoff/weapon contract.
  if(!constants)return increment_stat(id,2,7);
  auto publication=result_publication();std::optional<SourceResultCapture> capture;
  if(publication)capture.emplace(*publication,*constants);
  const auto tick=std::uint32_t(simulation_.ticks());
  return source_result_credited_kill(end_stats_,id,tick,source_result_multikill_cutoff(tick,30,*constants),cause,
      capture ? &*capture : nullptr,rules_.team_mode ? players_.at(id).source_membership : -1);
}
void MultiplayerMatch::record_victim_death(unsigned id,std::uint32_t credited,bool self,bool betrayal,
    std::optional<std::uint8_t> cause) {
  auto publication=result_publication();std::optional<SourceResultCapture> capture;
  if(publication && content_.original_data->result_capture_constants)
    capture.emplace(*publication,*content_.original_data->result_capture_constants);
  const auto* credit=player_from_datum(credited);
  const auto credit_slot=credit ? int(credited&0xffffu) : -1;
  source_result_victim_death(end_stats_,id,credit_slot,self,betrayal,cause,capture ? &*capture : nullptr,
      rules_.team_mode ? players_.at(id).source_membership : -1,
      rules_.team_mode && credit ? credit->source_membership : -1);
  players_[id].deaths=end_stats_.players[id].fields[3];
  players_[id].suicides=end_stats_.players[id].fields[4];
}
void MultiplayerMatch::prepare_respawn(unsigned id, std::uint32_t credited, std::uint32_t clock) {
  auto &victim = players_.at(id);
  // Original death prefix decrements before its respawn and score callbacks;
  // a score-ending kill still consumes the victim's last active life.
  source_player_life_death(victim.source_lives_remaining,2,0,true,1,finished_?2:1);
  if (const auto *attacker = player_from_datum(credited); attacker && credited != victim.source_datum &&
      attacker->source_membership == victim.source_membership && victim.respawn.countdown == 0) {
    auto &timer = players_[credited & 0xffffu].respawn;
    timer.penalty = std::bit_cast<std::int32_t>(std::uint32_t(timer.penalty) + rules_.betrayal_ticks);
  }
  const auto suicide = credited == victim.source_datum ? std::int32_t(rules_.suicide_ticks) : 0;
  if (rules_.team_mode) {
    std::array<TeamRespawnPlayer,multiplayer_capacity> roster{};unsigned count = 0;
    for (const auto &p : players_) if(p.connected)
      roster[count++] = {p.source_membership, p.source_flags, p.respawn.countdown};
    initialize_team_respawn_timer(victim.respawn, victim.source_membership,
        std::span(roster.data(),count), clock, 0,
        std::int32_t(rules_.respawn_ticks), 30, suicide, rules_.respawn_modifier);
  } else initialize_respawn_timer(victim.respawn, clock, 0,
      std::int32_t(rules_.respawn_ticks), 30, suicide);
}
void MultiplayerMatch::infect(unsigned id) {
  if(!players_.at(id).connected || (infection_zombies_&(1u<<id)))return;
  infection_zombies_|=std::uint16_t(1u<<id);
  infection_appearance(players_[id],6);
  if(players_[id].source_current_object!=null_datum && !unit(id).vitality.dead) {
    auto &u=unit_mutable(id);drop_carried_weapons(u);
    const auto handle=allocate_object(2);
    if(handle==null_datum)throw FormatError("No object slot for initial zombie sword");
    const auto definition=source_starting_weapon(rules_.weapon,17,null_datum,rules_.weapon_choices,simulation_.random_state_);
    u.source_inventory={};u.weapon=native_weapon_state(content_.weapons.at(definition),handle);
    u.source_inventory.slots[0]=handle;u.source_inventory.current[0]=u.source_inventory.requested[0]=0;
    u.source_inventory.inventory_sequence=u.source_inventory.replicated_sequence=1;
    u.vitality.shield=0;u.definition.vitality.shield=0;u.grenades={};
    simulation_.object_lifetimes_.publish_native(handle,definition);
  }
}
void MultiplayerMatch::start_infection() {
  std::array<unsigned,16> present{};unsigned count=0;
  for(unsigned i=0;i<players_.size();++i)if(players_[i].connected)present[count++]=i;
  if(count<2)return;
  infection_started_=true;
  infection_start_tick_=std::uint32_t(simulation_.ticks());
  round_lifecycle_.start_tick=infection_start_tick_;
  for(unsigned i=0;i<count;++i)infection_appearance(players_[present[i]],2);
  simulation_.random_state_=simulation_.random_state_*1664525u+1013904223u;
  infect(present[(simulation_.random_state_>>16)%count]);
}
void MultiplayerMatch::update_infection() {
  if(!infection_started_ || finished_)return;
  unsigned humans=0;
  for(unsigned i=0;i<players_.size();++i)if(players_[i].connected && !(infection_zombies_&(1u<<i)))++humans;
  const bool survived=rules_.time_ticks &&
      std::uint32_t(simulation_.ticks()-infection_start_tick_)>=rules_.time_ticks;
  if(humans && !survived)return;
  if(survived && humans)for(unsigned i=0;i<players_.size();++i)
    if(players_[i].connected && !(infection_zombies_&(1u<<i)))apply_score(i,2);
  int winner=-1,score=std::numeric_limits<int>::min();bool tied=false;
  for(unsigned i=0;i<players_.size();++i)if(players_[i].connected) {
    if(players_[i].score>score){winner=int(i);score=players_[i].score;tied=false;}
    else if(players_[i].score==score)tied=true;
  }
  end_match(tied?-1:winner,-1);
}
void MultiplayerMatch::step(
    const std::array<Controls, multiplayer_capacity> &controls) {
  SourceExecutionCallScope journal(source_object_world_?&source_object_world_->memory():nullptr,
      SourceExecutionCallKind::MatchStep,[&]{return source_execution_arguments(controls);});
  NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;
  // Remote clients retain these authored templates and consume completed
  // authority state. Require the original factory when advancing a world,
  // rather than rejecting a display replica during construction.
  if(rules_.primary_heavy_vehicle && !simulation_.source_physics_movement_)
    throw FormatError("Primary Heavy Vehicle authority requires the retained original source world");
  if(!simulation_.source_physics_movement_ &&
      std::any_of(content_.netgame_equipment.begin(),content_.netgame_equipment.end(),[&](const auto &row) {
        return (row.classification==1 || row.classification==2) &&
            content_.light_vehicle_options[row.classification-1]!=0;
      }))throw FormatError("Light Vehicle authority requires the retained original source world");
  if(!simulation_.source_physics_movement_ &&
      std::any_of(content_.netgame_equipment.begin(),content_.netgame_equipment.end(),[&](const auto &row) {
        return (row.classification==6 || row.classification==7) &&
            content_.turret_vehicle_options[row.classification-6]!=0;
      }))throw FormatError("Turret authority requires the retained original source world");
  if(simulation_.devices().replicated())throw FormatError("replicated device match cannot step as authority");
  if(pre_player_update_owner_)pre_player_update_owner_(*this);
  if(rules_.custom==CustomSlayer::infection && !infection_started_)start_infection();
  // Original13AAC0/139750 keeps admitted world phases running after game
  // over. Respawn eligibility and stat writers have their own status gates.
  // Original TU139750 runs TU14EAE0 before the world phases and the clock
  // increment. Current unit NONE, rather than body depletion alone, admits
  // the player. Native ordinary deaths currently release at the death boundary.
  std::array<unsigned, multiplayer_capacity> queued{};
  unsigned queue_size = 0;
  const auto clock = std::uint32_t(simulation_.ticks());
  for (unsigned i = 0; i < players_.size(); ++i)
    if (players_[i].connected && players_[i].source_current_object == null_datum &&
        tick_respawn_timer(players_[i].respawn, players_[i].source_datum, clock, 0, 30,
            true,source_player_respawn_allowed(rules_.life_limit,players_[i].source_flags,
                players_[i].source_membership,players_[i].source_current_object,
                players_[i].source_lives_remaining),!finished_).eligible)
      queued[queue_size++] = i;
  while (queue_size) {
    unsigned selected = 0;
    if (queue_size > 1) {
      simulation_.random_state_ = simulation_.random_state_*0x19660du+0x3c6ef35fu;
      selected = (simulation_.random_state_>>16)%queue_size;
    }
    const auto id = queued[selected];
    if (tick_respawn_timer(players_[id].respawn, players_[id].source_datum, clock, 0, 30,
        true,source_player_respawn_allowed(rules_.life_limit,players_[id].source_flags,
            players_[id].source_membership,players_[id].source_current_object,
            players_[id].source_lives_remaining),!finished_).eligible)
      spawn(id, players_[id].source_initial_spawn);
    queued[selected] = queued[--queue_size];
  }
  // TU1397AC ->15D7C0 checks time before TU1397C0's world updates and the
  // later clock increment. The current world pass still runs after completion;
  // score writers observe inactive status and reject later score changes.
  //15E870 only writes engine+E0 while lifecycle1/status1 is active.
  if(!finished_)round_lifecycle_.clock_seconds = slayer_clock_seconds(clock, round_lifecycle_.start_tick, rules_.time_ticks);
  if (!finished_ && rules_.custom!=CustomSlayer::infection && rules_.time_ticks && slayer_remaining_ticks(clock, 0, rules_.time_ticks) == 0) {
    if (rules_.team_mode) {
      const auto winner = slayer_team_winner(team_scores_, retained_team_mask_);
      if (!winner) throw FormatError("Source team winner is unresolved without a retained team mask");
      end_match(-1,*winner == null_datum ? -1 : int(*winner));
    } else {
    std::array<SlayerRankingPlayer, multiplayer_capacity> ranks{};
    unsigned count = 0;
    for (unsigned i = 0; i < players_.size(); ++i)
      if (players_[i].connected)
        ranks[count++] = {players_[i].source_datum,
            std::bit_cast<std::int16_t>(std::uint16_t(players_[i].score)),
            players_[i].source_flags, players_[i].source_ffa_identity};
    const auto winner = slayer_ffa_winner(std::span(ranks.data(), count));
    end_match(winner == 0xffffffffu ? -1 : int(winner & 0xffffu),-1);
    }
  }
  update_game_engine();
  // Original15EBA0 (TU15D7C0) calls15E440 after its clock/engine work,
  // before world object updates. The shared query owns participant/life
  // exhaustion, including retained empty teams. Admitted round selector0
  // always finishes here; multi-round phase/reset owners remain gated.
  if(!finished_) {
    SourceRoundEndInput input;input.team_mode=rules_.team_mode;input.life_limit=rules_.life_limit;
    // TU15E443..15E463 first requires the registered original game engine.
    // Campaign session delivery uses engine0/NONE, not the native Slayer shell.
    input.engine_present=source_game_context_.game_type==2;
    input.source_engine=rules_.source_engine;
    input.retained_team_mask=retained_team_mask_;
    std::array<SourceRoundEndParticipant,multiplayer_capacity> roster{};unsigned count=0;
    for(const auto& p:players_)if(p.connected)
      roster[count++]={p.source_datum,p.source_flags,p.source_membership,p.source_current_object,p.source_lives_remaining};
    const auto decision=source_round_end_check(input,std::span(roster.data(),count));
    if(decision.end) {
      if(rules_.team_mode)end_match(-1,decision.winner==null_datum ? -1 : int(decision.winner));
      else end_match(decision.winner==null_datum ? -1 : int(decision.winner&0xffffu),-1);
    }
  }
  // Original13AAC0 runs pre-world15EBA0 (including scoring) then19A830.
  // Its first eligible refresh calls19DE10, setting started before sampling
  // start time. The event ring's completion/identity/machine consumers remain
  // separate; no retained report is manufactured by this timestamp producer.
  if(retained_recording_.enabled && !result_events_started_) {
    result_events_started_=true;result_events_.start=result_wall_time();
  }
  // Supplied PC44A514→47590F precedes world-object44A52D and later phase
  // application44A550. The optional Vista presentation owner consumes this
  // actual pre-world phase/status/tick; it cannot advance the simulation.
  if(game_engine_presentation_owner_)game_engine_presentation_owner_(*this);
  //15C150/15AE70 consumes the current simulation clock before its increment.
  //Only the configured complete levitated branch is represented here.
  step_world_weapons(clock);
  //15EBA0: clock/time owner, then15E540 samples stat13 before world objects.
  // Current unit is the source ownership binding, rather than the body flag.
  if(auto publication=result_publication())for(unsigned i=0;i<players_.size();++i)
    if(players_[i].connected)
      source_result_lifetime_tick(*publication,i,{clock,30,1,
          players_[i].source_current_object!=null_datum,true,!results_finalized_,!finished_});
  step_weapon_pickups(controls);
  if(simulation_.source_physics_movement_) {
    for(unsigned i=0;i<players_.size();++i) {
      const auto &p=players_[i];
      if(!p.connected || p.source_current_object==null_datum || unit(i).vitality.dead)continue;
      const auto &input=controls[i];std::array<std::byte,0x5c> command;
      initialize_source_neutral_command(command);
      const auto put=[&](unsigned offset,std::uint32_t bits,unsigned bytes=4) {
        for(unsigned n=0;n<bytes;++n)command[offset+n]=std::byte(bits>>(n*8));
      };
      // Native delivered-input boundary. Actual153070 -> C92C0 carries the
      // flags to unit+148; E4A40 consumes bit0 and the jump owner consumes bit1.
      //15265C..15281C admits ordinary melee from bit5 and sword trigger
      //from bit6; the original106390 predicate is authored weapon flag24.
      // Original1889D0 at1895B7/1895CA/1895D2 publishes the held trigger
      // flags. ActualC7DD0 consumes them; bit28 requests manual reload through
      // original1524E9 andC7F7A's action0/action10 dispatch.
      const auto held=content_.weapons.find(unit(i).weapon.definition);
      const bool sword=held!=content_.weapons.end() && (held->second.inventory_flags&(1u<<24));
      put(0,unsigned(input.crouch)|(unsigned(input.jump)<<1)|(input.grenade?0x04000000u:0u)|
          (input.melee?0x20u:0u)|((sword && input.fire)?0x40u:0u)|
          (input.fire?0x00010000u:0u)|(input.fire_secondary?0x00220000u:0u)|
          (input.reload?0x10000000u:0u));
      put(4,std::bit_cast<std::uint32_t>(input.yaw));
      put(8,std::bit_cast<std::uint32_t>(input.pitch));
      put(0xc,std::bit_cast<std::uint32_t>(input.forward));
      put(0x10,std::bit_cast<std::uint32_t>(-input.right));
      // Actual1530FE/153170 and153176/153196 retain command+14/+18;
      // C92F0..C92FC publishes them to unit+1C0/+1C4 unchanged.
      put(0x14,std::bit_cast<std::uint32_t>(input.fire?1.f:0.f));
      put(0x18,std::bit_cast<std::uint32_t>(input.fire_secondary?1.f:0.f));
      const auto buttons=source_pickup_buttons(input.pickup,input.pickup?input.pickup_milliseconds:0,
          input.dual_wield,input.dual_wield?input.dual_wield_milliseconds:0,
          source_pickup_hold_threshold(content_.pickup_hold_seconds));
      // Pickup occupies one word; preserve the source initializer's NONE
      // selection at +1E rather than overwriting it with a DWORD action store.
      put(0x1c,buttons,2);
      auto &grenade_owner=unit_mutable(i);
      const auto selected_grenade=grenade_ready_type(grenade_owner.grenade_current,grenade_owner.grenades,
          input.switch_grenade?1:0);
      if(selected_grenade!=-1)grenade_owner.grenade_requested=selected_grenade;
      put(0x22,std::uint16_t(grenade_owner.grenade_requested),2);
      //C92C0 copies command+24 to the unit's requested zoom byte. A source
      //reset invalidates cached native requests until the sender acknowledges it.
      put(0x24,std::uint16_t(input.zoom_reset_serial==grenade_owner.zoom_reset_serial?
          input.zoom:std::int16_t(-1)),2);
      auto target=input.vehicle_interaction_target?*input.vehicle_interaction_target:action_prompt(i).target;
      if(input.vehicle_interaction_target && target.object!=null_datum) {
        bool admitted=false;
        try {
          const auto body=source_object_world_->object_body(target.object);
          std::array<std::byte,1> kind{};source_object_world_->memory().read(body+0xaau,kind);
          const auto type=std::to_integer<unsigned>(kind[0]);
          const auto definition=source_object_world_->object_definition(target.object);
          const auto *assets=content_.unit_interactions.get();
          if(assets) {
            const auto unit=assets->units.find(definition);
            if(unit!=assets->units.end()) {
              admitted=target.action==4 && type==1;
              if((target.action==5 || target.action==6) && type<2 && target.index>=0)
                admitted=unsigned(target.index)<unit->second.seats.size();
            }
          }
        } catch(const FormatError &) {} // A retired source generation has no action.
        if(!admitted)target={};
      }
      if(target.object!=null_datum) {
        put(0x28,std::uint16_t(target.action)|(std::uint32_t(std::uint16_t(target.index))<<16));
        put(0x2c,target.object);
      }
      source_object_world_->tu15_world().populate_weapon_selection(p.source_current_object,command,input.switch_weapon);
      source_object_world_->prepare_player_look(p.source_datum,p.source_current_object,command);
      source_object_world_->populate_weapon_targeting(p.source_datum,p.source_current_object,command);
      source_object_world_->deliver_command(p.source_datum,p.source_current_object,command);
    }
  }
  auto delivered=controls;
  for(unsigned i=0;i<players_.size();++i)if(players_[i].source_flags&2u) {
    delivered[i]={};delivered[i].yaw=unit(i).yaw;delivered[i].pitch=unit(i).pitch;
  }
  simulation_.step_players(delivered);
  // Detached native items remain live weapon objects. The source-object
  // backend owns its separate raw update/observation path.
  if(!simulation_.source_physics_movement_)
    for(auto &item:weapon_pickups_)if(item.weapon)simulation_.update_passive_weapon(*item.weapon,nullptr);
  for(const auto &event:simulation_.take_rumble())rumble_events_.push_back({event,++rumble_serial_,simulation_.ticks()});
  if(rumble_events_.size()>64)rumble_events_.erase(rumble_events_.begin(),rumble_events_.end()-64);
  native_presentation_controls_=delivered;
  // TU1.5 source object/world phases produce the pose consumed by this
  // tick's kill-volume and death handling, rendering and snapshots.
  if(simulation_.source_physics_movement_) {
    first_person_clock_origins_=first_person_views_;
    source_object_world_->step_objects();
  }
  if(simulation_.source_physics_movement_) {
    for(auto effect:source_object_world_->take_unit_damage_effects()) {
      effect.serial=++simulation_.unit_damage_effect_serial_;effect.tick=simulation_.ticks();
      simulation_.unit_damage_effects_.push_back(effect);
    }
    if(simulation_.unit_damage_effects_.size()>64)
      simulation_.unit_damage_effects_.erase(simulation_.unit_damage_effects_.begin(),simulation_.unit_damage_effects_.end()-64);
    for(auto impact:source_object_world_->take_melee_impacts()) {
      impact.serial=++simulation_.bullet_impact_serial_;impact.tick=simulation_.ticks();
      simulation_.melee_impacts_.push_back(impact);
    }
    if(simulation_.melee_impacts_.size()>64)
      simulation_.melee_impacts_.erase(simulation_.melee_impacts_.begin(),simulation_.melee_impacts_.end()-64);
    publish_source_scene_physics(simulation_.scene_,source_object_world_->memory(),source_object_world_->object_pool());
    adopt_source_devices();
    simulation_.synchronize_props();
    adopt_source_prop_regions();
    // Read back after this tick's original prepare/release/physics owners.
    // Construction precedes player join, so adopting only there loses every
    // subsequently thrown grenade and leaves released poses stale.
    if(simulation_.grenade_sandbox_) {
      std::vector<NativeGrenadeSandbox::PresentationOwner> presentation_units;
      for(unsigned slot=0;slot<players_.size();++slot)if(players_[slot].connected)
        presentation_units.push_back({&unit(slot),int(slot)-1});
      simulation_.grenade_sandbox_->adopt_source_projectiles(source_object_world_->grenade_projectiles(),presentation_units);
      simulation_.grenade_sandbox_->adopt_source_detonations(simulation_.ticks(),
          source_object_world_->take_projectile_detonations());
    }
  }
  if(simulation_.source_physics_movement_)for(unsigned i=0;i<players_.size();++i) {
    auto &p=players_[i];auto &u=unit_mutable(i);
    const bool departed=p.connected && (p.source_flags&2u) && u.exists && u.source_previous_player==p.source_datum;
    const auto object=p.source_current_object!=null_datum?p.source_current_object:departed?u.source_datum:null_datum;
    if(!p.connected || object==null_datum)continue;
    if(simulation_.object_lifetimes_.header(object).empty()) {
      if(!departed)throw FormatError("Active source player lost its current unit");
      simulation_.object_lifetimes_.observe_source_retirement(object);u.exists=false;continue;
    }
    const auto motion=source_object_world_->biped_motion(object);
    u.body.position=motion.position;u.body.velocity=motion.velocity;
    // E4A40 copies the actual physical result's unsupported bit to348. It
    // changes on takeoff/contact while physical state1 remains walking mode.
    u.body.grounded=(motion.support_flags&1u)==0;
    u.source_parent=motion.parent;u.source_seat=motion.seat;
    u.source_action_word=motion.action_word;u.source_unit_10a=motion.unit_flags;
    const auto body=source_object_world_->object_body(object);
    const auto &memory=source_object_world_->memory();
    const auto previous_zoom=u.source_zoom;
    memory.read(body+0x240u,std::as_writable_bytes(std::span(&u.source_zoom.current,1)));
    memory.read(body+0x241u,std::as_writable_bytes(std::span(&u.source_zoom.requested,1)));
    memory.read(body+0x26cu,std::as_writable_bytes(std::span(&u.source_zoom.motion,1)));
    if(previous_zoom.requested!=-1 && u.source_zoom.requested==-1)
      ++u.zoom_reset_serial;
    const auto previous_grenade=u.grenade_action;
    u.grenade_action=source_object_world_->grenade_action(object);
    if(u.grenade_action.animation_active && (!previous_grenade.animation_active ||
        previous_grenade.graph!=u.grenade_action.graph || previous_grenade.clip!=u.grenade_action.clip ||
        u.grenade_action.frame<previous_grenade.frame) && simulation_.first_person_activity_hook_)
      simulation_.first_person_activity_hook_(u,u.weapon,u.grenade_action.activity);
    u.grenade_current=source_object_world_->current_grenade(object);
    const auto previous_melee=u.melee_action;
    u.melee_action=source_object_world_->melee_action(object);
    const bool melee_began=u.melee_action.active() && (!previous_melee.active() ||
        previous_melee.activity!=u.melee_action.activity || u.melee_action.progress<previous_melee.progress);
    //This timestamp belongs to native presentation; the source component
    //continues to own admission, progress, contact and completion.
    u.melee_action.started=melee_began?std::uint32_t(simulation_.ticks()):previous_melee.started;
    if(melee_began && simulation_.first_person_activity_hook_)
      simulation_.first_person_activity_hook_(u,u.weapon,u.melee_action.activity);
    // Falling/kill-volume requests can deplete the body inside the original
    // world phase. Publish that completed transition before native death
    // accounting; never leave the player alive beside its source corpse.
    const auto vitality=source_object_world_->object_vitality(u.source_datum);
    u.vitality.body=vitality.body;u.vitality.shield=vitality.shield;
    u.damage_feedback=vitality.feedback;
    if((motion.unit_flags&4u) && !u.vitality.dead) {
      u.vitality.dead=true;u.state=Unit::Dead;
      u.last_damager=-2;
    }
    if(departed || !u.vitality.dead)
      synchronize_source_equipment(u,source_object_world_->player_equipment(object));
    if(departed && u.vitality.dead && !corpses_.contains(object)) {
      // Original CBDC0 has already cleaned this body in the source world.
      // Adopt its presentation and detached equipment without a second kill,
      // weapon drop, native garbage admission or numerical death publication.
      u.source_death_tick=clock;
      corpses_.insert_or_assign(object,MultiplayerState::Corpse{object,u.definition.datum,
          p.source_datum,clock,p.source_configuration,u.body.velocity});
    }
    u.crouch=motion.crouch;
    u.body.height=capsule_segment_height(u.definition.height,u.definition.crouch_height,
        u.body.radius,u.crouch,(u.definition.physics_flags&2)!=0)+2*u.body.radius;
  }
  if(simulation_.source_physics_movement_)for(auto &item:weapon_pickups_) {
    if(item.handle==null_datum)continue;
    if(simulation_.object_lifetimes_.header(item.handle).empty()) {
      simulation_.object_lifetimes_.observe_source_retirement(item.handle);
      item.handle=null_datum;item.weapon.reset();item.visible=false;continue;
    }
    const auto root=source_object_world_->object_transform(item.handle);
    item.position=root.position;item.forward=root.basis[0];item.up=root.basis[2];
    item.visible=source_object_world_->object_parent(item.handle)==null_datum;
    if(!item.visible)item.weapon.reset();
  }
  for (unsigned i = 0; i < players_.size(); ++i)
    if (players_[i].connected && !unit(i).vitality.dead)
      for (auto index : content_.kill_triggers)
        if (simulation_.scene_.contains(content_.trigger_volumes.at(index),
                                        unit(i).body.position)) {
          auto &u = unit_mutable(i);
          u.vitality.dead = true;
          u.vitality.body = 0;
          // Complete original DEFC0 kill-trigger fatal return preserves current momentum.
          u.state = Unit::Dead;
          u.last_damager = -2;
          break;
        }
  for (unsigned i = 0; i < players_.size(); ++i) {
    auto &p = players_[i];
    if (!p.connected || (p.source_flags&2u))
      continue;
    if (unit(i).vitality.dead && !p.death_counted) {
      if(results_finalized_) {
        finish_death(unit_mutable(i),p,clock);
        continue;
      }
      p.death_counted = true;
      const auto owner = unit(i).last_damager;
      const int attacker = owner + 1;
      const bool credited = owner >= -1 && attacker < int(players_.size()) &&
                            unsigned(attacker) != i &&
                            players_[attacker].connected;
      const bool relationship = credited && players_[attacker].source_membership == p.source_membership;
      if (!p.attribution_counted && credited && !relationship) {
        // This fatal-input adapter has no recovered reporting-type producer.
        players_[attacker].kills=record_credited_kill(unsigned(attacker),std::nullopt);
      }
      if (!p.attribution_counted && relationship) ++players_[attacker].nonenemy_credit;
      const bool suicide = p.attribution_counted ? p.incoming_suicide
          : owner >= -1 && unsigned(attacker) == i;
      // Native last-damager translation remains an attribution boundary. The
      // source callback's score requests and active-status writes are exact.
      if (!p.attribution_counted) {
        prepare_respawn(i,credited || suicide ? players_[attacker].source_datum : null_datum,clock);
        record_victim_death(i,credited || suicide ? players_[attacker].source_datum : null_datum,
            suicide,relationship,std::nullopt);
        game_engine_death(credited || suicide ? players_[attacker].source_datum : null_datum,
            p.source_datum,suicide || relationship);
      }
      // Non-SMG/kill-volume deaths still enter at this native boundary. Stock
      // SMG cleanup already ran synchronously after its damage-history write.
      finish_death(unit_mutable(i), p, clock);
    }
  }
  if(source_object_world_ && !simulation_.source_physics_movement_)source_object_world_->step_objects();
  std::erase_if(corpses_,[this](const auto &entry) {
    const auto *body=unit_from_datum(entry.first);return !body || !body->vitality.dead;
  });
  std::erase_if(retained_units_,[this](const auto &entry) {
    return simulation_.object_lifetimes_.header(entry.first).empty();
  });
  // Original14EFF0 follows object updates and calls153920 only for a current
  // unit. Expiry therefore changes direction after this frame's unit fade.
  const auto camouflage=camouflage_services();
  for(const auto& player:players_)if(player.connected && player.source_current_object!=null_datum)
    source_powerup_tick(player.source_datum,camouflage);
  // Original139750 calls1678C1 after object update/postpass. Startup's native
  // configure call is a declared observer-creation boundary, not a full retail frame.
  update_first_person_views();
  if(simulation_.source_physics_movement_) {
    // A clock interval belongs to one source unit/hand registration. Birth,
    // replacement or a graph change starts at the actual new source sample;
    // a released/previous identity cannot be its interpolation endpoint.
    for(unsigned view=0;view<first_person_players_.size();++view) {
      auto &origin=first_person_clock_origins_[view];const auto &current=first_person_views_[view];
      if(origin.unit!=current.unit || origin.representation!=current.representation)origin=current;
      else for(unsigned h=0;h<2;++h)
        if(origin.hands[h].weapon!=current.hands[h].weapon || origin.hands[h].graph!=current.hands[h].graph)
          origin.hands[h]=current.hands[h];
    }
  } else first_person_clock_origins_=first_person_views_;
  for(unsigned view=0;view<first_person_players_.size();++view)
    for(unsigned h=0;h<2;++h) {
      if(simulation_.source_physics_movement_)continue;
      auto &binding=first_person_views_[view];auto &hand=binding.hands[h];
      const auto *w=weapon_from_datum(hand.weapon);if(!w)continue;
      const auto found=first_person_definitions_.find({unsigned(binding.representation),w->definition});
      if(found!=first_person_definitions_.end())first_person_animation_clock(hand,found->second,tick_seconds,
          [&](const FirstPersonAnimationEvent &event) {
            if(first_person_event_owner_)first_person_event_owner_(view,h,binding,event);
          });
    }
  update_replicated_first_person_views();
  if(simulation_.source_physics_movement_) {
    for(unsigned view=0;view<first_person_players_.size();++view) {
      const auto id=first_person_players_[view];
      replicated_fp_origins_[id]=first_person_clock_origins_[view];
      replicated_fp_views_[id]=first_person_views_[view];
    }
  } else replicated_fp_origins_=replicated_fp_views_;
  for(auto &view:replicated_fp_views_)for(auto &hand:view.hands) {
    if(simulation_.source_physics_movement_)continue;
    const auto *w=weapon_from_datum(hand.weapon);if(!w)continue;
    const auto found=replicated_fp_definitions_.find({unsigned(view.representation),w->definition});
    if(found!=replicated_fp_definitions_.end())first_person_animation_clock(hand,found->second,tick_seconds);
  }
  if(rules_.custom==CustomSlayer::infection)update_infection();
  if(finished_)finalize_results();
  source_game_over_tick(game_over_,tick_seconds,simulation_.sound_classes_,true,game_over_constants_);
}
void MultiplayerMatch::end_match(int player_winner,int team_winner) {
  if(finished_)return;
  finished_=true;winner_=player_winner;winner_team_=team_winner;
  // Admitted Slayer has round selector0/index0. Original15DDC0 awards the
  // winner synchronously; remaining score requests see inactive status.
  std::array<SourceGameEndParticipant,multiplayer_capacity> roster{};unsigned count=0;
  for(unsigned i=0;i<players_.size();++i)if(players_[i].connected)
    roster[count++]={players_[i].source_datum,players_[i].source_membership,players_[i].source_flags};
  const auto winner=rules_.team_mode ? (winner_team_<0 ? null_datum : std::uint32_t(winner_team_)) :
      (winner_<0 ? null_datum : players_[unsigned(winner_)].source_datum);
  auto publication=result_publication();
  source_game_end_award(end_stats_,std::span(roster.data(),count),rules_.team_mode,winner,
      publication ? &*publication : nullptr);
  round_lifecycle_.status=2;
  source_game_over_begin(game_over_,30,game_over_constants_);
  round_lifecycle_.flags|=0x20;
}
void MultiplayerMatch::apply_round_phase() {
  SourceRoundContext context;
  context.raw_mode=1;context.game_over=game_over_.active;
  context.tick=std::uint32_t(simulation_.ticks());
  context.time_limit_seconds=rules_.custom==CustomSlayer::infection?0:std::int32_t(rules_.time_ticks/30);
  context.life_limit=rules_.life_limit;
  // The admitted native lifecycle reaches phase1 before any player joins,
  // and phase2 after its final world pass. Neither needs player reset views.
  // The later-round reset/event owners are deliberately not installed.
  auto services=slayer_round_services();
  if(rules_.source_engine==4)
    services.initialize_engine=[&] {source_king_round_start(king_state_,king_markers_,rules_.king_move_seconds,30);};
  // Both registered phase callbacks are literal RET4; King round start is
  // distinct and retains existing team/player timers just like the original.
  source_round_apply_phase(round_lifecycle_,context,end_stats_,retained_recording_,{},
      world_weapon_rows_,services);
}
void MultiplayerMatch::finalize_results() {
  if(results_finalized_)return;
  // 15F450 ->15EFA0 ->160750 carries all score records exactly once after
  // the current world pass. Live event/profile/session owners remain open.
  for(unsigned i=0;i<players_.size();++i)if(players_[i].connected)
    end_stats_.players[i].fields[0]=std::bit_cast<std::int16_t>(std::uint16_t(players_[i].score));
  //15EFA0 applies the pending lifecycle2 after the current world pass.
  apply_round_phase();
  source_game_end_accumulate(end_stats_,true);
  results_finalized_=true;
}
void MultiplayerMatch::adopt_source_prop_regions() {
  auto rows=simulation_.props_.region_snapshot();
  for(auto &row:rows)row.regions=source_object_world_->object_model_regions(row.handle);
  simulation_.props_.observe_source_regions(rows);
  simulation_.synchronize_props();
}
std::uint32_t MultiplayerMatch::create_world_vehicle(std::size_t placement) {
  NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;
  if(!source_object_world_ || !source_object_world_->owns_physics() || placement>=content_.objects.size() || content_.objects[placement].type!=1)
    throw FormatError("Vehicle placement requires an authored type1 and the retained source world");
  return create_world_object(placement);
}
void MultiplayerMatch::destroy_world_object(std::uint32_t object) {
  NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;
  if(!source_object_world_ || !source_object_world_->owns_physics())
    throw FormatError("World object retirement requires the retained source world");
  simulation_.scene_.destroy(object);
}
std::uint32_t MultiplayerMatch::create_world_object(std::size_t placement) {
  NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;
  if(!source_object_world_ || !source_object_world_->owns_physics() || placement>=content_.objects.size())
    throw FormatError("World object placement requires its authored definition and retained source world");
  for(const auto& object:simulation_.scene_.objects())if(object.handle!=null_datum && object.placement==placement)return object.handle;
  const auto handle=simulation_.scene_.create_placement(placement,simulation_.ticks());
  if(content_.objects[placement].model_attachment) {
    const auto parent=source_object_world_->object_parent(handle);
    if(parent==null_datum)throw FormatError("Source vehicle attachment lost its constructed parent");
    simulation_.scene_.get(handle).attachment_parent=parent;
  }
  return handle;
}
MultiplayerState MultiplayerMatch::state() const {
  SourceExecutionCallScope journal(source_object_world_?&source_object_world_->memory():nullptr,
      SourceExecutionCallKind::MatchState,[]{return std::vector<std::byte>{};});
  MultiplayerState result;
  result.devices=simulation_.devices().snapshot(simulation_.ticks());
  const auto device_events=simulation_.device_events();
  result.device_events.assign(device_events.begin(),device_events.end());
  result.rumble_events=rumble_events_;
  const auto removals=simulation_.prop_removals();
  result.prop_removals.assign(removals.begin(),removals.end());
  result.prop_regions=simulation_.props().region_snapshot();
  for(const auto &o:simulation_.scene().objects())if(o.handle!=null_datum && o.visible && content_.objects[o.placement].type==1) {
    Transform transform;if(o.solved_transform)transform=*o.solved_transform;else {const auto b=authored_basis(o.rotation);transform={o.scale,{b.forward,b.left,b.up},o.position};}
    WorldVehiclePresentation vehicle{o.handle,std::uint32_t(o.placement),transform};
    if(source_object_world_ && source_object_world_->owns_physics()) {
      vehicle.regions=source_object_world_->object_model_regions(o.handle);
      auto root=o.handle;std::set<std::uint32_t> visited;
      for(;;){
        if(!visited.insert(root).second)throw FormatError("Vehicle lighting parent cycle");
        const auto parent=source_object_world_->object_parent(root);if(parent==null_datum)break;root=parent;
      }
      const auto body=source_object_world_->object_body(root);const auto& bank=source_object_world_->memory();
      const auto own=source_object_world_->object_body(o.handle);
      bank.read(own+4,std::as_writable_bytes(std::span(&vehicle.geometry_flags,1)));
      bank.read(own+0x10a,std::as_writable_bytes(std::span(&vehicle.geometry_control_flags,1)));
      vehicle.lighting_owner=root;vehicle.lighting_definition=source_object_world_->object_definition(root);
      vehicle.lighting_origin=source_object_world_->object_sphere(root).center;
      vehicle.lighting_velocity=source_object_world_->object_linear_velocity(root);
      bank.read(body+4,std::as_writable_bytes(std::span(&vehicle.lighting_flags,1)));
      vehicle.lighting_frozen=(std::to_integer<unsigned>(bank.view(body+0xc1,1)[0])&1u)!=0;
    }
    result.world_vehicles.push_back(vehicle);
  }
  result.tick = std::uint32_t(simulation_.ticks());
  result.source_game_type=source_game_context_.game_type;
  result.source_engine=rules_.source_engine;
  if(rules_.source_engine==4)
    result.king=MultiplayerState::KingObjective{king_state_.hill,king_state_.occupants};
  result.finished = finished_;
  result.results_finalized = results_finalized_;
  result.game_over_countdown=game_over_.countdown;
  result.game_over_fade_started=game_over_.fade_started;
  result.winner = winner_;
  result.score_limit = rules_.score_limit;
  result.round_mode=rules_.round_mode;
  result.life_limit=rules_.life_limit;
  result.time_ticks = rules_.time_ticks;
  result.clock_seconds = round_lifecycle_.clock_seconds;
  result.variant_name = rules_.name;
  result.team_mode = rules_.team_mode;result.retained_team_mask = retained_team_mask_;
  result.winner_team = winner_team_;result.team_scores = team_scores_;
  for(unsigned i=0;i<8;++i) {
    result.team_statistics[i]=end_stats_.teams[i].fields;
    result.team_total_scores[i]=end_stats_.teams[i].fields[1];
    result.team_wins[i]=end_stats_.teams[i].fields[7];
  }
  for (unsigned i = 0; i < players_.size(); ++i)
    if (players_[i].connected) {
      const auto &u = unit(i);
      const auto &p = players_[i];
      result.players[i] = {true,
                           u.vitality.dead || bool(p.source_flags&2u),
                           u.body.position,
                           u.yaw,
                           i ? u.pitch : simulation_.player_pitch_,
                           u.crouch,
                           u.vitality.body,
                           u.vitality.shield,
                           u.weapon.definition,
                           u.weapon.loaded,
                           u.weapon.reserve,
                           p.score,
                           p.kills,
                           p.deaths,
                           std::uint32_t(u.weapon.last_shot),
                           std::uint32_t(u.weapon.reload_started),
                           u.weapon.reload};
      if(source_object_world_ && source_object_world_->owns_physics())
        if(const auto look=source_object_world_->player_look(p.source_datum,p.source_current_object)) {
          result.players[i].yaw=look->x;result.players[i].pitch=look->y;
        }
      result.players[i].reload_empty = u.weapon.reload_empty;
      result.players[i].action_prompt=action_prompt(i);
      result.players[i].weapon_shader=u.weapon.shader;
      result.players[i].weapon_trigger={u.weapon.smg.input_flags,u.weapon.smg.trigger_ticks,
          u.weapon.smg.trigger_state,u.weapon.charge_fraction};
      const auto publish_target=[&](std::uint32_t weapon,NativeWeaponTriggerPublication &out) {
        if(!simulation_.source_physics_movement_ || weapon==null_datum)return;
        const auto target=source_object_world_->weapon_hud_target(weapon);out.target_acquired=target.acquired;
        if(target.position)out.target_position=std::array{target.position->x,target.position->y,target.position->z};
      };
      publish_target(p.source_current_object!=null_datum?u.weapon.source_datum:null_datum,result.players[i].weapon_trigger);
      result.players[i].ammunition_available=native_weapon_ammunition_available(u,u.weapon);
      result.players[i].damage_feedback=u.damage_feedback;
      result.players[i].shield_recharging=u.vitality.shield_recharging;
      if(const auto *offhand=p.source_current_object!=null_datum?native_hand_weapon(u,1):nullptr) {
        auto &out=result.players[i].secondary_weapon;
        out.definition=offhand->definition;out.datum=offhand->source_datum;
        out.loaded=offhand->loaded;out.reserve=offhand->reserve;
        out.shot_tick=std::uint32_t(offhand->last_shot);out.reload_tick=std::uint32_t(offhand->reload_started);
        out.reload_seconds=offhand->reload;out.reload_empty=offhand->reload_empty;
        out.shader=offhand->shader;
        out.trigger={offhand->smg.input_flags,offhand->smg.trigger_ticks,offhand->smg.trigger_state,offhand->charge_fraction};
        publish_target(out.datum,out.trigger);
        out.ammunition_available=native_weapon_ammunition_available(u,*offhand);
        out.magazine_mode=offhand->source_runtime_published ||
            (content_.weapons.contains(offhand->definition) && content_.weapons.at(offhand->definition).smg)?
            offhand->smg.magazine_mode:offhand->magazine.mode;
        if((!simulation_.source_physics_movement_ ||
            std::find(first_person_players_.begin(),first_person_players_.end(),i)!=first_person_players_.end()) &&
            replicated_fp_views_[i].hands[1].weapon==offhand->source_datum)
          out.clock={true,first_person_channel_state(replicated_fp_origins_[i].hands[1]),
              first_person_channel_state(replicated_fp_views_[i].hands[1])};
      }
      result.players[i].velocity=u.body.velocity;
      result.players[i].native_forward=std::clamp(native_presentation_controls_[i].forward,-1.f,1.f);
      result.players[i].native_right=std::clamp(native_presentation_controls_[i].right,-1.f,1.f);
      // The native capsule owns accepted crouch, including a stand-up blocked
      // by a ceiling. Presentation must not stand merely because the key rose.
      result.players[i].native_crouching=u.crouch>0;
      result.players[i].native_grounded=u.body.grounded;
      result.players[i].landing=u.landing;
      if(u.body.support_triangle && *u.body.support_triangle<content_.collision.size())
        result.players[i].native_support_material=simulation_.collision().triangles()[*u.body.support_triangle].material;
      result.players[i].grenades = u.grenades;
      result.players[i].grenade_current = u.grenade_current;
      result.players[i].grenade_action = u.grenade_action;
      result.players[i].melee_action=u.melee_action;
      result.players[i].weapon_effect=weapon_effect_events_[i];
      result.players[i].assists = p.assists;
      result.players[i].suicides = p.suicides;
      result.players[i].total_score=end_stats_.players[i].fields[1];
      result.players[i].wins=end_stats_.players[i].fields[7];
      result.players[i].source_respawn_countdown=p.respawn.countdown;
      result.players[i].source_lives_remaining=p.source_lives_remaining;
      result.players[i].source_king_inside_ticks=p.source_king_inside_ticks;
      result.players[i].source_player=p.source_datum;
      result.players[i].source_unit=p.source_current_object;
      result.players[i].source_previous_unit=p.source_previous_unit;
      result.players[i].source_death_tick=u.source_death_tick;
      result.players[i].source_weapon=p.source_current_object!=null_datum ? u.weapon.source_datum : null_datum;
      result.players[i].name=p.name;
      result.players[i].source_configuration=p.source_configuration;
      result.players[i].source_flags=p.source_flags;
      // Roster membership is the signed source player+C0 byte, independent
      // of any current/corpse unit's attribution membership field.
      result.players[i].source_membership=p.source_membership;
      result.players[i].source_ffa_identity=p.source_ffa_identity;
      result.players[i].source_zoom=u.source_zoom;
      if(simulation_.source_physics_movement_ && p.source_current_object!=null_datum) {
        result.players[i].source_hud_target_flags=source_object_world_->weapon_hud_target_flags(p.source_current_object);
        float confidence{};
        source_object_world_->memory().read(source_object_world_->object_body(p.source_current_object)+0x1e4u,
            std::as_writable_bytes(std::span(&confidence,1)));
        result.players[i].source_hud_autoaimed=confidence>=1.f;
        result.players[i].source_vehicle_hud=source_object_world_->vehicle_weapon_hud(p.source_current_object);
      }
      result.players[i].source_zoom_magazine_mode=u.weapon.source_runtime_published ||
          (content_.weapons.contains(u.weapon.definition) && content_.weapons.at(u.weapon.definition).smg)?
          u.weapon.smg.magazine_mode:u.weapon.magazine.mode;
      result.players[i].source_parent=u.source_parent;
      result.players[i].source_seat=u.source_seat;
      result.players[i].source_action_word=simulation_.source_physics_movement_?u.grenade_action.active_actions():u.source_action_word;
      result.players[i].source_camouflage=u.source_camouflage;
      result.players[i].source_unit_10a=u.source_unit_10a;
      result.players[i].motion_sensor_control=
          (native_presentation_controls_[i].fire?1u:0u) |
          (native_presentation_controls_[i].fire_secondary?0x20u:0u);
      result.players[i].zoom_reset_serial=u.zoom_reset_serial;
      const auto definition=content_.weapons.find(u.weapon.definition);
      const std::array<std::int16_t,1> modes{result.players[i].source_zoom_magazine_mode};
      const ZoomWeaponView primary{definition!=content_.weapons.end() ? definition->second.zoom : ZoomWeaponDefinition{},modes};
      result.players[i].source_hud_zoom_mask=source_zoom_hud_mask(source_zoom_hud_status(
          u.source_zoom.current,result.players[i].source_weapon!=null_datum && definition!=content_.weapons.end() ? &primary : nullptr));
      if((!simulation_.source_physics_movement_ ||
          std::find(first_person_players_.begin(),first_person_players_.end(),i)!=first_person_players_.end()) &&
          !replicated_fp_definitions_.empty() && result.tick && replicated_fp_origins_[i].unit==replicated_fp_views_[i].unit &&
          replicated_fp_origins_[i].hands[0].weapon==replicated_fp_views_[i].hands[0].weapon &&
          replicated_fp_views_[i].unit==p.source_current_object &&
          p.source_current_object!=null_datum && replicated_fp_views_[i].hands[0].weapon==u.weapon.source_datum)
        result.players[i].first_person_clock={true,first_person_channel_state(replicated_fp_origins_[i].hands[0]),
            first_person_channel_state(replicated_fp_views_[i].hands[0])};
    }
  if(simulation_.grenade_sandbox_) {
    const auto grenades=simulation_.grenade_sandbox_->projectiles();result.grenade_projectiles.assign(grenades.begin(),grenades.end());
    const auto detonations=simulation_.grenade_sandbox_->detonations();result.grenade_detonations.assign(detonations.begin(),detonations.end());
  }
  const auto impacts=simulation_.bullet_impacts();result.bullet_impacts.assign(impacts.begin(),impacts.end());
  const auto melee=simulation_.melee_impacts();result.melee_impacts.assign(melee.begin(),melee.end());
  const auto damage_effects=simulation_.unit_damage_effects();
  result.unit_damage_effects.assign(damage_effects.begin(),damage_effects.end());
  const auto voices=simulation_.unit_damage_voices();
  result.unit_damage_voices.assign(voices.begin(),voices.end());
  if(source_object_world_ && source_object_world_->owns_physics())
    result.source_contrails=capture_source_contrail_snapshot(source_object_world_->memory());
  result.weapon_projectiles=simulation_.presentation_projectiles();
  if(simulation_.source_physics_movement_) {
    result.weapon_projectiles.clear();
    const auto &memory=source_object_world_->memory();
    for(const auto &object:source_object_world_->live_objects()) {
      if(object.type!=5 || object.parent!=null_datum ||
          std::any_of(result.grenade_projectiles.begin(),result.grenade_projectiles.end(),
              [&](const auto &g){return g.datum==object.object;}))continue;
      const auto body=source_object_world_->object_body(object.object);
      const auto scalar=[&](unsigned offset){return std::bit_cast<float>(Bytes(memory.view(body+offset,4)).u32(0));};
      const auto root=source_object_world_->object_transform(object.object);
      result.weapon_projectiles.push_back({object.object,object.definition,root.position,
          {scalar(0x88),scalar(0x8c),scalar(0x90)},root.basis[0],root.basis[2],scalar(0x168)});
    }
  }
  for(const auto &item:weapon_pickups_)if(item.weapon)
    result.world_weapons.push_back({item.handle,item.definition,item.position,item.forward,item.up,
        item.weapon->loaded,item.weapon->reserve,item.weapon->shader});
  result.source_body_poses_authoritative=source_object_world_!=nullptr;
  for(const auto &[object,appearance]:corpses_) {
    const auto *unit=unit_from_datum(object);if(!unit || !unit->vitality.dead)continue;
    if(source_object_world_ && !source_object_world_->camera_target(object))continue;
    auto corpse=appearance;
    corpse.health=unit->vitality.body;corpse.shield=unit->vitality.shield;
    corpse.damage_feedback=unit->damage_feedback;
    if(source_object_world_ && source_object_world_->owns_physics()) {
      const auto motion=source_object_world_->biped_motion(object);
      corpse.velocity=motion.velocity;corpse.physical_state=motion.physical_state;
    }
    if(source_object_world_) {
      const auto vitality=source_object_world_->object_vitality(object);
      corpse.health=vitality.body;corpse.shield=vitality.shield;corpse.damage_feedback=vitality.feedback;
    }
    result.corpses.push_back(std::move(corpse));
  }
  result.weapon_effects=weapon_effect_journal_;
  // Read-only current source frames follow moving parents on remote views too.
  // A retired source instance retains its completed birth bank in the journal.
  std::map<std::uint32_t,std::vector<SourceEffectLocation>> live_effect_locations;
  for(auto &event:result.weapon_effects)if(event.source_presentation) {
    if(!live_effect_locations.contains(event.source_instance))
      if(auto locations=weapon_effect_locations(source_object_world_,event.source_instance,event.effect))
        live_effect_locations.emplace(event.source_instance,std::move(*locations));
    if(const auto found=live_effect_locations.find(event.source_instance);found!=live_effect_locations.end()) {
      event.source_locations=found->second;event.source_locations_observed=true;
    }
    for(auto &player:result.players)if(player.weapon_effect.serial==event.serial)player.weapon_effect=event;
  }
  if(source_object_world_) {
    for(const auto &player:result.players)if(player.connected && !player.dead && player.source_unit!=null_datum)
      result.source_body_poses.push_back({player.source_unit,source_object_world_->world_nodes(player.source_unit),
          source_object_world_->object_sphere(player.source_unit)});
    for(const auto &player:result.players)if(player.connected && !player.dead)
      for(const auto weapon:{player.source_weapon,player.secondary_weapon.datum})if(weapon!=null_datum)
        result.source_body_poses.push_back({weapon,source_object_world_->world_nodes(weapon),
            source_object_world_->object_sphere(weapon)});
    for(const auto &corpse:result.corpses)
      result.source_body_poses.push_back({corpse.object,source_object_world_->world_nodes(corpse.object),
          source_object_world_->object_sphere(corpse.object)});
    for(const auto &vehicle:result.world_vehicles)
      result.source_body_poses.push_back({vehicle.handle,source_object_world_->world_nodes(vehicle.handle),
          source_object_world_->object_sphere(vehicle.handle)});
  }
  if(source_object_world_ && source_object_world_->owns_physics()) {
    result.source_scene_roots_authoritative=true;
    for(const auto &object:simulation_.scene().objects())if(object.handle!=null_datum &&
        content_.objects.at(object.placement).type!=1) {
      if(!object.solved_transform || !object.solved_nodes)
        throw FormatError("Source scene snapshot lacks its completed root or WORLD bank");
      SourceSceneRoot root{object.handle,content_.objects[object.placement].definition,
          std::uint32_t(object.placement),*object.solved_transform,object.visible,!object.solved_nodes->empty()};
      const auto body=source_object_world_->source_object_body(object.handle);
      auto &bank=source_object_world_->memory();
      root.velocity=source_object_world_->object_linear_velocity(object.handle);
      bank.read(body+4u,std::as_writable_bytes(std::span(&root.runtime_flags,1)));
      root.frozen=(std::to_integer<unsigned>(bank.view(body+0xc1u,1)[0])&1u)!=0;
      root.created_tick=object.created_tick;
      root.attached_effects=source_object_world_->attached_effects(object.handle);
      result.source_scene_roots.push_back(root);
      if(!object.solved_nodes->empty())result.source_body_poses.push_back({object.handle,*object.solved_nodes,
          source_object_world_->object_sphere(object.handle)});
    }
    std::set<std::uint32_t> placed;
    for(const auto &object:simulation_.scene().objects())if(object.handle!=null_datum)placed.insert(object.handle);
    SourceModelResourceWorld visibility;auto &bank=source_object_world_->memory();
    visibility.banks.objects=source_object_world_->source_object_pool();
    visibility.banks.read=[&bank](auto p,auto n){return bank.view(p,n);};
    for(const auto &object:source_object_world_->live_objects()) {
      if((object.type!=3 && object.type!=4 && object.type!=11) || placed.contains(object.object))continue;
      const auto body=source_object_world_->source_object_body(object.object);
      SourceDynamicObject dynamic;dynamic.object=object.object;dynamic.definition=object.definition;
      dynamic.parent=object.parent;dynamic.type=object.type;
      dynamic.transform=source_object_world_->object_transform(object.object);
      dynamic.velocity=source_object_world_->object_linear_velocity(object.object);
      dynamic.sphere=source_object_world_->object_sphere(object.object);
      dynamic.visible=!source_model_owner_hidden(object.object,visibility);
      bank.read(body+4u,std::as_writable_bytes(std::span(&dynamic.runtime_flags,1)));
      bank.read(body+0x10au,std::as_writable_bytes(std::span(&dynamic.control_flags,1)));
      dynamic.frozen=(std::to_integer<unsigned>(bank.view(body+0xc1u,1)[0])&1u)!=0;
      dynamic.regions=source_object_world_->object_model_regions(object.object);
      dynamic.colors=source_live_object_colors_tu15(bank,source_object_world_->object_pool(),object.object);
      auto nodes=source_object_world_->world_nodes(object.object);dynamic.has_world_nodes=!nodes.empty();
      if(dynamic.has_world_nodes)result.source_body_poses.push_back({object.object,std::move(nodes),dynamic.sphere});
      result.source_dynamic_objects.push_back(std::move(dynamic));
    }
  }
  journal.complete([&]{return source_execution_state_result(result);});
  return result;
}
void observe_multiplayer_first_person(FirstPersonViewBinding &view,
    const MultiplayerState::Player &player,const FirstPersonBindingDefinition &definition) {
  if(!player.connected) {release_first_person_binding(view);return;}
  if(player.source_player==null_datum ||
      (player.source_unit==null_datum && player.source_weapon!=null_datum))
    throw FormatError("Remote first-person view has inconsistent current identities");
  FirstPersonBindingInput input;
  input.player=player.source_player;input.unit=player.source_unit;
  input.representation=multiplayer_representation(player.source_configuration);input.weapons[0]=player.source_weapon;
  if(input.representation!=definition.representation)
    throw FormatError("Remote first-person definition differs from the player's representation");
  observe_first_person_binding(view,input,[&](auto datum,auto representation)->const FirstPersonBindingDefinition * {
    return datum==player.source_weapon && representation==definition.representation &&
        player.weapon==definition.weapon_definition ? &definition : nullptr;
  });
}
std::array<std::uint32_t,18> Cache::multiplayer_weapon_choices() const {
  const Bytes data(cache_bytes());
  const auto globals=query_tag({.group=tag_groups::globals});
  if(!globals)throw FormatError("Missing multiplayer globals");
  const auto matg=metadata_offset(globals->address,globals->size);
  const auto mulg=query_tag({.handle=TagHandle{data.u32(matg+0x16c)},.group=TagGroup::from_string("mulg")});
  if(!mulg)throw FormatError("Missing multiplayer weapon runtime");
  const auto [count,runtime]=metadata_block(metadata_offset(mulg->address,mulg->size)+8,0x568);
  if(count!=1)throw FormatError("Unsupported multiplayer runtime count");
  const auto [weapons,entries]=metadata_block(runtime+0x68,8);
  if(weapons<18)throw FormatError("Incomplete original multiplayer weapon reference bank");
  std::array<std::uint32_t,18> result;
  for(unsigned i=0;i<result.size();++i)result[i]=data.u32(entries+i*8+4);
  return result;
}
std::uint32_t Cache::slayer_starting_weapon(unsigned option,int source_engine) const {
  if (option != 0 && (option < 3 || option > 20))
    throw FormatError("Unsupported multiplayer starting-weapon option");
  if(option>=3 && option!=5) {
    // Complete original19EDA0 / TU1.5 1A89B0: direct choices select the
    // authored runtime bank. Only choice2 consumes RNG; it stays separate.
    std::uint32_t unused_random{};
    const auto selected=source_starting_weapon(null_datum,std::int8_t(option),null_datum,
        multiplayer_weapon_choices(),unused_random);
    if(selected==null_datum || !query_tag({.handle=TagHandle{selected},.group=tag_groups::weapon,.require_address=true}))
      throw FormatError("Authored starting-weapon choice has no resident weapon");
    return selected;
  }
  const Bytes data(cache_bytes());
  auto root = [&](std::uint32_t id, const std::string &group,
                  std::size_t size) {
    const auto t = query_tag({.handle=TagHandle{id}, .group=TagGroup::from_string(group)});
    if (!t || t->size < size)
      throw FormatError("Missing multiplayer " + group);
    return metadata_offset(t->address, t->size);
  };
  
  if (option == 5) {
    // Original 19EDA0 option 5 selects multiplayer Runtime.Weapons[1].
    // 19EF40 applies it to the selected scenario starting equipment;
    // variant parameter 59 is its serialized +D6 option byte.
    const auto globals = query_tag({.group=tag_groups::globals});
    if (!globals)
      throw FormatError("Missing multiplayer globals");
    const auto mulg = data.u32(root(globals->datum, "matg", 0x170) + 0x16c);
    const auto [runtimes, runtime] = metadata_block(root(mulg, "mulg", 0x10) + 8, 0x568);
    if (runtimes != 1)
      throw FormatError("Unsupported multiplayer runtime globals count");
    const auto [weapons, entries] = metadata_block(runtime + 0x68, 8);
    if (weapons < 2)
      throw FormatError("Missing multiplayer SMG weapon option");
    const auto weapon = data.u32(entries + 8 + 4);
    root(weapon, "weap", 0xb4);
    return weapon;
  }
  const auto scenario =
      query_tag({.group=tag_groups::scenario});
  if (!scenario)
    throw FormatError("Missing multiplayer scenario");
  const auto [count, offset] =
      metadata_block(root(scenario->datum, "scnr", 0x130) + 0x128, 0x9c);
  for (std::size_t i = 0; i < count; ++i) {
    const auto at = offset + i * 0x9c;
    NetgameEquipment selection;
    for (unsigned j = 0; j < 4; ++j) {
      selection.modes[j]=std::int16_t(data.u16(at + 4 + j * 2));
    }
    if (source_netgame_equipment_selector(selection,true,source_engine)<0)
      continue;
    const auto collection = data.u32(at + 0x40);
    if (collection == null_datum)
      continue;
    const auto [items, entry] = metadata_block(root(collection, "itmc", 8), 0x10);
    if (items != 1)
      throw FormatError(
          "Weighted multiplayer starting collections are unfinished");
    const auto weapon = data.u32(entry + 8);
    root(weapon, "weap", 0xb4);
    return weapon;
  }
  throw FormatError("No authored starting weapon for the selected source engine");
}
} // namespace halo2
