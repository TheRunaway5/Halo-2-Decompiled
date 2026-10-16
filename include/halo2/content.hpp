#pragma once
#include "halo2/weapon_magazine.hpp"
#include "halo2/player_training.hpp"
#include "halo2/biped_landing.hpp"
#include "halo2/cache.hpp"
#include "halo2/kernels.hpp"
#include "halo2/original_data.hpp"
#include "halo2/weapon_error.hpp"
#include "halo2/smg.hpp"
#include "halo2/weapon_trigger.hpp"
#include "halo2/damage_amount.hpp"
#include "halo2/damage_feedback.hpp"
#include "halo2/spawn.hpp"
#include "halo2/unit_zoom.hpp"
#include "halo2/source_control.hpp"
#include "halo2/native_weapon_projectile.hpp"
#include "halo2/weapon_barrel_timing.hpp"
#include "halo2/weapon_energy.hpp"
#include <unordered_map>
namespace halo2 {
struct PropSandboxDefinition;
struct StaticSceneryCollisionDefinition;
struct PropAreaDamageDefinition;
struct SourceDeviceWorldDefinition;
constexpr std::uint32_t null_datum = 0xffffffffu;
struct CollisionSurfaceIdentity {
  std::int32_t bsp=-1, record=-1, surface=-1, instance=-1;
  std::uint16_t plane=0xffff;
};
struct CollisionTriangle {
  Vec3 a, b, c;
  std::uint16_t material{};
  std::uint8_t flags{};
  // Retained across polygon fan expansion and instance placement.
  CollisionSurfaceIdentity source;
  Vec3 source_normal; // Authored collision plane, including surface bit15.
};
struct NavigationPortal {
  std::size_t sector{};
  Vec3 midpoint;
  std::uint16_t flags{};
};
struct NavigationSector {
  std::uint16_t flags{};
  std::vector<Vec3> polygon;
  std::vector<NavigationPortal> portals;
};
struct BspContent {
  std::vector<CollisionTriangle> collision;
  std::vector<NavigationSector> navigation;
};
struct PredictedResource {
  int type{}, index{};
  std::uint32_t datum{};
};
struct DamageSection {
  std::uint32_t name{}, flags{};
  float vitality{};
};
struct VitalityDefinition {
  float health{}, shield{}, shield_delay{}, shield_rate{}, body_rate{};
  float body_delay{}, body_max = 1;
  float minimum_shield_stun_damage{}, shield_damaged_threshold{};
  int shield_material = -1;
  float minimum_body_stun_damage{};
  int body_material = -1, default_section = -1;
  std::uint32_t damage_flags{};
  std::vector<DamageSection> sections{};
  // hlmt damage-info tag references. These are separate from the ordinary
  // hit shimmer's runtime shader inputs and overlay admission.
  std::uint32_t shield_damaged_first_person_shader = null_datum;
  std::uint32_t shield_damaged_shader = null_datum;
  std::uint32_t shield_damaged_effect = null_datum;
  std::uint32_t shield_depleted_effect = null_datum;
  std::uint32_t shield_recharging_effect = null_datum;
  std::int32_t damage_info_count{}; // Signed hlmt+60 admission, independent of shield capacity.
  std::uint32_t overshield_first_person_shader = null_datum;
  std::uint32_t overshield_shader = null_datum;
};
struct BipedDefinition {
  std::uint32_t datum = null_datum;
  std::uint32_t model = null_datum;
  Vec3 bounding_offset;
  float bounding_radius{};
  float height{}, crouch_height{}, radius{}, camera_height{},
      crouch_camera_height{}, crouch_velocity{}, jump_velocity{};
  std::uint32_t physics_flags{};
  std::uint16_t object_flags{}; // Original object definition +2, including D1850 mode bit1.
  SourceLandingDefinition landing;
  std::uint32_t unit_flags{}; // Original unit definition +BC, scope-update bit10.
  float maximum_slope_cosine = .65f;
  SlopeDefinition slope;
  int team{};
  VitalityDefinition vitality;
  float camera_field_of_view = 1.22173047f; // Authored horizontal angle, radians.
};
struct WeaponFiringEffectDefinition {
  std::int16_t minimum{}, maximum{};
  // Literal107160 selectors0(normal),1(misfire),2(empty). References are
  // authored data, even when their datum is NONE; no clip name selects them.
  std::array<std::uint32_t,3> effects{}, damage_effects{};
};
struct WeaponDefinition {
  // Presentation metadata for authored barrel-less items whose gameplay is
  // owned by the retained source runtime (objectives and depleted weapons).
  bool source_runtime_only{};
  // Immutable projections of each actual authored barrel. Children retain
  // their own projectile/effect/damage operands and have no child views.
  std::vector<std::shared_ptr<const WeaponDefinition>> barrel_views;
  bool charged_cycle{};
  float barrel_effect_a_threshold{},barrel_effect_b_threshold{},barrel_effect_a_decay{};
  float barrel_rate_threshold{},barrel_charge_scale{};
  std::uint32_t datum = null_datum, projectile = null_datum,
                damage = null_datum, sound = null_datum;
  std::string name;
  std::uint32_t model = null_datum;
  std::uint32_t first_person_model = null_datum,
                first_person_animation = null_datum;
  struct FirstPersonModel {
    std::uint32_t model = null_datum, animation = null_datum;
  };
  std::vector<FirstPersonModel> first_person_models;
  Vec3 first_person_offset;
  Vec2 first_person_scope_size;
  ZoomWeaponDefinition zoom;
  float ready_seconds{}; // Authored weap+138; original1040F0 converts to ticks.
  std::uint32_t ready_effect=null_datum; // Authored weap+144, separate from graph events.
  float firing_camouflage_amount{},firing_camouflage_limit{}; // weap+27C/+280
  float damage_camouflage_amount{}; // Actual selected jpt!+30
  int distribution{};
  float distribution_angle{}, minimum_error{};
  WeaponErrorDefinition firing_error;
  std::optional<SmgDefinition> smg;
  struct BarrelTiming {
    WeaponBarrelTimingDefinition definition;
    WeaponBarrelTimingConstants constants;
    float rate_rise{},rate_fall{};
    std::int16_t index{};
    float effect_decay{}; // Authored selected barrel+CC, independent of rate fall.
  };
  std::optional<BarrelTiming> barrel_timing;
  struct PutAwayTiming {
    bool present{};
    std::int16_t frames{};
    std::optional<std::int16_t> event4,event0;
  };
  // Actual player representation2/3 and single/dual authored08000025 clips.
  std::array<std::array<PutAwayTiming,2>,2> put_away_timing;

  struct Energy {
    WeaponHeatDefinition heat;
    WeaponAgeDefinition age;
    float heat_per_shot{},spent_per_shot{},one{},random_scale{};
    float misfire_threshold{},misfire_chance{},detonation_threshold{},detonation_chance{};
  };
  std::optional<Energy> energy;
  std::optional<WeaponMagazineDefinition> magazine;
  struct BarrelAmmunition {
    std::uint32_t flags{};
    std::int16_t magazine{},rounds{},minimum{};
    WeaponBarrelTimingDefinition timing;
    WeaponErrorDefinition error;
    float rate_rise{},rate_fall{},effect_decay{};
  };
  std::vector<BarrelAmmunition> barrel_ammunition;
  struct FiringPresentation {float rate_rise{},rate_fall{};};
  // Original107160 fixed-repeat normal-effect bank. Visual/part
  // admission and complete trigger/reload/projectile ownership are separate.
  std::optional<FiringPresentation> firing_presentation;
  std::optional<SourceNeedlerProjectileDefinition> needler_projectile;
  std::vector<WeaponFiringEffectDefinition> firing_effects;
  bool random_firing_effects{};
  std::optional<DamageAmountDefinition> damage_amount;
  float rounds_per_second{}, reload_time{}, projectile_speed{},
      projectile_gravity{}, damage_min{}, damage_max{};
  float range{}, fire_recovery{};
  int burst{}, projectiles_per_shot{};
  bool instantaneous{};
  std::vector<WeaponTriggerDefinition> triggers; // All actual authored64-byte trigger records.
  std::int16_t trigger_behavior{}; // Single authored trigger+6; 1 releases before firing again.
  std::uint32_t damage_flags{};
  std::uint32_t damage_group{}, damage_group_b{};
  int damage_side_effect{}, damage_category{};
  int capacity{}, reserve{}, rounds_per_shot{}, minimum_rounds{},
      reload_rounds{};
  bool wastes_rounds{};
  Vec3 bounding_offset;
  std::int16_t weapon_type{}; // Literal weap+292, used by original ammunition reload pose.
  std::int16_t pickup_priority{}; // Original weap+290 special pickup class.
  std::uint32_t inventory_flags{}; // Complete authored weap+12C, including carry-limit bits3/4.
  bool dual_wieldable{}; // Authored weap+12C bit22.
  float dual_damage_scale = 1;
  int initial_rounds = -1;
  std::vector<std::byte> projectile_material_responses;
  std::uint32_t projectile_flags{};
  float projectile_dry_damage_range{}; // Original proj+16C -> dry projectile body+170.
};
struct CharacterDefinition {
  std::uint32_t datum = null_datum;
  BipedDefinition biped;
  float vision_distance{}, vision_angle{}, acknowledgement{};
  int actor_type = -1;
  std::optional<DamageFadeDefinition> damage_fade;
  std::uint32_t general_flags{}; // First inherited nonempty char general block.
};
struct ActorPlacement {
  Vec3 position;
  float facing{};
  std::uint32_t flags{};
  std::uint32_t character = null_datum, weapon = null_datum;
  std::string placement_script;
  std::int16_t reference_frame=-1; // actor start+10, consumed by retail210850.
  std::uint32_t starting_location_name{}; // Authored start+0 SID, retail203780.
};
struct SquadDefinition {
  std::string name;
  std::uint32_t flags{};
  int team{}, count{}, zone{}, bsp = -1;
  std::vector<ActorPlacement> starts;
  std::int16_t parent_group=-1; // scnr squad+26, retail2009D0.
};
struct ScriptNode {
  std::uint16_t salt{}, opcode{}, type{}, flags{};
  std::uint32_t next{}, string_offset{}, value{};
};
struct ScriptDefinition {
  std::string name;
  std::uint16_t type{}, return_type{};
  std::uint32_t root = null_datum;
};
struct ScriptGlobal {
  std::string name;
  std::uint16_t type{};
  std::uint32_t root = null_datum;
};
struct ScriptProgram {
  std::vector<ScriptNode> nodes;
  std::string strings;
  std::vector<ScriptDefinition> scripts;
  std::vector<ScriptGlobal> globals;
};
struct ScenarioObject {
  int netgame_row=-1;
  struct ModelAttachment {std::size_t parent{};std::uint32_t parent_marker{},child_marker{};};
  std::optional<ModelAttachment> model_attachment;
  int type{}, placement{}, name = -1, origin_bsp = -1, bsp_policy{};
  std::uint32_t definition = null_datum, flags{}, variant{};
  std::uint16_t manual_bsps{};
  Vec3 position, rotation;
  Vec3 bounding_offset;
  float scale = 1;
  int source = 1;
  bool damageable{};
  std::uint32_t unique_id = null_datum;
  std::int16_t lightmapping_policy{}; // Scenario scenery +4E, retained separately from tag policy.
  std::uint16_t valid_multiplayer_games{};
  std::vector<std::pair<std::int16_t,std::int16_t>> pathfinding_references; // Scenario scenery +5A, D5060.
  struct WeaponDatum {std::int16_t rounds_total{},rounds_loaded{};std::uint32_t flags{};};
  std::optional<WeaponDatum> weapon; // Actual scenario weapon+4C/+4E/+50, FDA00.
  float bounding_radius{}; // Original object tag+4, before current instance scale.
};
struct ObjectName {
  std::string name;
  int type = -1, placement = -1;
};
struct CutsceneFlag {
  std::string name;
  Vec3 position;
  float yaw{}, pitch{};
};
struct CameraPoint {
  std::string name;
  Vec3 position, rotation;
  std::uint16_t flags{};
  int type{};
};
struct TriggerVolume {
  std::string name;
  int object_name = -1;
  std::uint32_t node{};
  Vec3 forward, up, position, extents;
  float query_radius{}; // Authored scenario volume+3C,11C5F0 -> BB050.
};
// Original scenario+118/+11C, stride20. Preserve row identity and every
// authored byte; teleporter owners select type7/8 and signed channel+14.
struct NetgameFlag {
  std::uint32_t source_index{};
  Vec3 position;
  float facing{};
  std::int16_t type{}, selector12{}, channel{};
  std::array<std::byte,0x20> source_bytes{};
};
struct NetgameEquipment {
  std::uint32_t flags{};
  std::array<std::int16_t,4> modes{};
  std::int16_t team{}, respawn_seconds{}, empty_seconds{}, timer_start{};
  std::uint8_t classification{};
  Vec3 position, rotation;
  std::uint32_t collection_group{}, collection = null_datum;
  std::int16_t collection_delay{};
  struct Item {
    float weight{};
    std::uint32_t group{}, definition = null_datum, variant{};
  };
  std::vector<Item> items;
};
struct NativeUnitInteractionAssets;
struct LevelContent {
  std::shared_ptr<const NativeUnitInteractionAssets> unit_interactions;
  std::shared_ptr<const OriginalData> original_data;
  std::size_t bsp_count = 1;
  std::vector<CollisionTriangle> collision;
  std::vector<NavigationSector> navigation;
  BipedDefinition player;
  struct PlayerRepresentation {
    BipedDefinition biped;
    std::uint32_t hands = null_datum;
  };
  std::vector<PlayerRepresentation> player_representations;
  std::uint32_t player_representation_count{};
  std::vector<std::int16_t> player_start_representations;
  std::uint32_t first_person_hands = null_datum;
  MovementDefinition movement;
  Vec2 player_screen_offset{}; // matg+0xF0 control record+0x18/+0x1C (23C110).
  float auto_level_scale{};int auto_level_minimum_ticks{};
  float pickup_hold_seconds{}; // matg player-control record+7C, original187610.
  std::optional<BinocularZoomDefinition> binocular_zoom;
  std::optional<std::array<float,4>> projectile_difficulty_speed; // matg+F8 first record+A0, original1ED030 table10.
  std::vector<DamageGroup> damage_groups;
  struct DamageMaterial {
    std::uint32_t armor_a{}, armor_b{};
  };
  std::vector<DamageMaterial> damage_materials;
  std::vector<std::int16_t> material_parents;
  std::unordered_map<std::uint32_t, WeaponDefinition> weapons;
  std::unordered_map<std::uint32_t, CharacterDefinition> characters;
  std::vector<SquadDefinition> squads;
  std::vector<std::int16_t> squad_group_parents; // scnr+158 groups, parent+20.
  struct CommandPoint {Vec3 position;std::int16_t reference_frame=-1;std::int32_t surface=-1;};
  std::vector<std::vector<CommandPoint>> command_points;
  struct CommandFrame {std::int16_t frame{};std::size_t placement{};unsigned node{};std::int8_t axis{};std::uint8_t order{};};
  std::vector<std::vector<CommandFrame>> command_frames;
  std::vector<CampaignTitleDefinition> cinematic_titles;
  ScriptProgram scripts;
  std::vector<ObjectName> object_names;
  std::vector<ScenarioObject> objects;
  std::vector<SourceDeviceGroup> device_groups;
  std::vector<std::shared_ptr<const SourceDeviceWorldDefinition>> devices;
  // Same placement order as objects; absent for non-prop/unresolved poses.
  std::vector<std::shared_ptr<const PropSandboxDefinition>> prop_sandbox;
  std::vector<std::shared_ptr<const StaticSceneryCollisionDefinition>> static_scenery_collision;
  std::unordered_map<std::uint32_t,std::shared_ptr<const PropAreaDamageDefinition>> prop_area_damage;
  std::vector<CutsceneFlag> cutscene_flags;
  std::vector<CameraPoint> camera_points;
  std::vector<TriggerVolume> trigger_volumes;
  std::vector<NetgameFlag> netgame_flags;
  float teleporter_cooldown_seconds{}; // mulg runtime+534 -> constants+4.
  std::vector<unsigned> kill_triggers;
  struct BspTransition { std::uint32_t pair; int volume; };
  std::vector<BspTransition> bsp_transitions;
  std::uint32_t starting_weapon = null_datum;
  std::optional<std::uint32_t> multiplayer_starting_equipment_flags;
  struct StartingEquipment {
    std::uint32_t flags{};
    std::array<std::int16_t,4> modes{};
  };
  // Retain scenario starting-equipment row order for the selected engine.
  std::vector<StartingEquipment> multiplayer_starting_equipment;
  std::optional<SpawnSettings> multiplayer_spawn;
  // Original scnr+120/+124 order is also the timer phase/object row identity.
  std::vector<NetgameEquipment> netgame_equipment;
  // Variant inputs used to resolve cold class1/2 templates. The original
  // netgame factory still owns each birth, controller and physical state.
  std::array<std::int32_t,2> light_vehicle_options{};
  // Original15CDF0 inputs for cold class6/7 templates; source owns birth.
  std::array<std::int32_t,2> turret_vehicle_options{};
  std::vector<std::string> limitations;
  std::string describe() const;
};
struct EncodedSound {
  std::string name;
  unsigned sample_rate{}, channels{}, codec{}, samples{};
  std::vector<std::byte> data;
  unsigned sound_class = 54;
  // Owned compressed chunks retain the MCC decoder's packet boundaries and
  // state-release flag. `pcm_bytes` is the authored unrounded decoded extent.
  struct PacketChunk { std::uint32_t bytes{}, pcm_bytes{}; bool reset_after{}; };
  std::vector<PacketChunk> packet_chunks;
};
struct LoopingSound {
  struct Track {
    std::uint32_t flags{}, start = null_datum, loop = null_datum,
                           end = null_datum, alternate = null_datum,
                           alternate_end = null_datum;
    float gain_db{}, fade_in{}, fade_out{};
  };
  std::string name;
  std::uint32_t flags{};
  std::vector<Track> tracks;
};
} // namespace halo2
