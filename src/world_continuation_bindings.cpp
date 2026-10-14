#include "halo2/prop_sandbox.hpp"
#include "halo2/static_scenery_collision.hpp"
#include "halo2/source_device_world.hpp"
#include "halo2/source_runtime_resources.hpp"
#include "world_continuation_bindings.hpp"
#include "halo2/original_data.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/native_unit_interaction.hpp"
#include <openssl/sha.h>
#include <bit>
#include <tuple>
#include <type_traits>

namespace halo2::world_continuation {
namespace {
template<class T>struct Fields;
class Binding {
public:
    template<class... T>void operator()(const T &...v){(add(v),...);}
    std::array<std::byte,32> finish()const {
        std::array<std::byte,32> result{};
        if(!SHA256(bytes_.data(),bytes_.size(),reinterpret_cast<unsigned char *>(result.data())))
            throw FormatError("Cannot bind native world continuation");
        return result;
    }
private:
    std::vector<unsigned char> bytes_;
    void integer(std::uint64_t v){for(unsigned i=0;i<8;++i)bytes_.push_back(static_cast<unsigned char>(v>>(8*i)));}
    template<class T>void add(const T &v) {
        if constexpr(std::is_same_v<T,char>)integer(static_cast<unsigned char>(v));
        else if constexpr(std::is_integral_v<T> || std::is_enum_v<T>)integer(std::uint64_t(v));
        else if constexpr(std::is_same_v<T,float>)integer(std::bit_cast<std::uint32_t>(v));
        else if constexpr(requires {v.has_value();*v;}){integer(bool(v));if(v)add(*v);}
        else if constexpr(requires {typename T::element_type;v.get();}){integer(bool(v));if(v)add(*v);}
        else if constexpr(requires {std::tuple_size<T>::value;})std::apply([&](const auto &...items){(*this)(items...);},v);
        else if constexpr(requires {v.size();v.begin();v.end();}){integer(v.size());for(const auto &item:v)add(item);}
        else Fields<T>::write(*this,v);
    }
};
#define BIND(Type,...) template<>struct Fields<Type>{static void write(Binding &w,const Type &v){w(__VA_ARGS__);}};
BIND(Vec2,v.x,v.y)
BIND(Vec3,v.x,v.y,v.z)
BIND(Transform,v.scale,v.basis,v.position)
BIND(BonePose,v.translation,v.rotation,v.scale)
BIND(ModelBone,v.name,v.parent,v.bind,v.inverse,v.inverse_scale)
BIND(SourceModelCompoundNode,v.nodes,v.weights)
// These components consume hierarchy only; mesh/skin/render resources belong
// to other owners and the enclosing complete cache/schema binding.
BIND(RenderModel,v.name,v.bones,v.compound_nodes,v.checksum,v.animation)
BIND(CollisionPlane,v.normal,v.distance)
BIND(CollisionNode3,v.plane,v.children)
BIND(CollisionLeaf,v.flags,v.count,v.first)
BIND(CollisionReference2,v.plane,v.node)
BIND(CollisionNode2,v.normal,v.distance,v.children)
BIND(CollisionSurface,v.plane,v.first_edge,v.flags,v.breakable,v.material)
BIND(CollisionEdge,v.vertices,v.next,v.surfaces)
BIND(RawCollisionBsp,v.node,v.nodes,v.planes,v.leaves,v.references,v.nodes2,v.surfaces,v.edges,v.vertices,v.vertex_edges)
BIND(ModelCollision::Material,v.damage_section,v.global_material)
BIND(ModelCollision::Region,v.collision,v.permutations)
BIND(ModelCollision,v.model,v.collision,v.materials,v.model_regions,v.regions,v.authored_inverse)
BIND(SourceInitialModelState,v.permutation,v.properties,v.damage,v.probability,v.looping_effect,v.looping_marker)
BIND(SourceInitialVariantPermutation,v.permutation,v.weight,v.states)
BIND(SourceInitialVariantRegion,v.permutations)
BIND(SourceInitialModelVariant,v.name,v.regions,v.definitions)
BIND(SourceInitialModelRegion,v.name,v.permutation_flags)
BIND(SourceInitialModelDefinition,v.model,v.render_model,v.default_variant,v.variants,v.regions,v.render_sections)
BIND(SourceInitialModelRegionState,v.variant_permutation,v.damage,v.properties)
BIND(SourceInitialModelSelection,v.variant_index,v.permutations,v.states,v.unresolved_rng_regions)
BIND(ObjectAttachmentMarker,v.region,v.permutation,v.node,v.local,v.authored_scale)
BIND(DamageSection,v.name,v.flags,v.vitality)
BIND(VitalityDefinition,v.health,v.shield,v.shield_delay,v.shield_rate,v.body_rate,v.body_delay,v.body_max,
    v.minimum_shield_stun_damage,v.shield_damaged_threshold,v.shield_material,v.minimum_body_stun_damage,v.body_material,
    v.default_section,v.damage_flags,v.sections,v.shield_damaged_first_person_shader,v.shield_damaged_shader,
    v.shield_damaged_effect,v.shield_depleted_effect,v.shield_recharging_effect,v.damage_info_count,
    v.overshield_first_person_shader,v.overshield_shader)
BIND(SourceLandingDefinition,v.soft_seconds,v.hard_seconds,v.soft_speed,v.hard_speed,v.maximum_speed)
BIND(SlopeDefinition,v.downhill_scale,v.uphill_scale,v.downhill_full,v.downhill_start,v.uphill_start,v.uphill_full)
BIND(BipedDefinition,v.datum,v.model,v.bounding_offset,v.bounding_radius,v.height,v.crouch_height,v.radius,
    v.camera_height,v.crouch_camera_height,v.crouch_velocity,v.jump_velocity,v.physics_flags,v.object_flags,
    v.landing,v.unit_flags,v.maximum_slope_cosine,v.slope,v.team,v.vitality,v.camera_field_of_view)
BIND(PropDamageResponse,v.flags,v.effect,v.damage,v.marker,v.delay_effect,v.state,v.region,v.threshold,v.delay,
    v.skip_fraction,v.total_threshold,v.effect_marker)
BIND(PropDamageSection,v.flags,v.vitality,v.responses)
BIND(PropSandboxDefinition,v.datum,v.model,v.collision,v.region_definition,v.initial_regions,v.markers,v.vitality,
    v.sections,v.uniform_impact_material,v.uniform_impact_region_absorption,v.impact_region_absorbs)
BIND(ArmorModifier,v.name,v.multiplier)
BIND(DamageGroup,v.name,v.modifiers)
BIND(LevelContent::DamageMaterial,v.armor_a,v.armor_b)
BIND(StaticSceneryCollisionDefinition,v.collision,v.hierarchy,v.permutations)
BIND(CollisionSurfaceIdentity,v.bsp,v.record,v.surface,v.instance,v.plane)
BIND(CollisionTriangle,v.a,v.b,v.c,v.material,v.flags,v.source,v.source_normal)
BIND(NavigationPortal,v.sector,v.midpoint,v.flags)
BIND(NavigationSector,v.flags,v.polygon,v.portals)
BIND(BspContent,v.collision,v.navigation)
BIND(NativeMeleeConstants,v.candidates,v.frame_seconds_bits,v.impact_scale_bits,v.moving_threshold_bits,v.special_seconds_bits)
BIND(SourceRuntimeResources::Identity,v.resident,v.shared)
BIND(ScriptNode,v.salt,v.opcode,v.type,v.flags,v.next,v.string_offset,v.value)
BIND(ScriptDefinition,v.name,v.type,v.return_type,v.root)
BIND(ScriptGlobal,v.name,v.type,v.root)
BIND(ScriptProgram,v.nodes,v.strings,v.scripts,v.globals)
BIND(CameraPoint,v.name,v.position,v.rotation,v.flags,v.type)
BIND(CutsceneFlag,v.name,v.position,v.yaw,v.pitch)
BIND(AnimationClip::FrameEvent,v.type,v.frame)
BIND(AnimationClip::Channel,v.times,v.keys)
BIND(AnimationClip,v.source_graph,v.source_index,v.name,v.frame_count,v.loop_frame,v.animation_type,v.frame_event_count,
    v.frame_events,v.nodes,v.parents,v.node_flags,v.frames,v.components,v.channels,v.authored_flags,v.movement,v.movement_yaw)
BIND(NativeVehicleSeatDefinition,v.flags,v.mode,v.marker,v.entry_marker,v.grenade_marker,v.entry_message,
    v.grenade_message,v.melee_message,v.boarding_seat,v.radius,v.facing,v.aim,v.maximum_velocity)
BIND(NativeUnitSeatAnimations::Entry,v.clip,v.initial)
BIND(NativeUnitSeatAnimations,v.entries,v.idle,v.board,v.exit)
BIND(NativeUnitInteractionDefinition,v.unit_flags,v.vehicle_flags,v.seats,v.bound_offset,v.bound_radius,
    v.flip_message,v.flip_minimum,v.flip_maximum,v.seat_animations,v.hierarchy,v.permutations,v.markers,v.idle)
BIND(NativeUnitInteractionAssets,v.units,v.flipped_up_threshold,v.netgame_placements)
BIND(NetgameEquipment::Item,v.weight,v.group,v.definition,v.variant)
BIND(NetgameEquipment,v.flags,v.modes,v.team,v.respawn_seconds,v.empty_seconds,v.timer_start,v.classification,
    v.position,v.rotation,v.collection_group,v.collection,v.collection_delay,v.items)
BIND(UiVariant,v.name,v.engine,v.values)
BIND(SlayerRules,v.session_configuration,v.source_engine,v.name,v.score_limit,v.round_mode,v.life_limit,
    v.time_ticks,v.respawn_ticks,v.suicide_ticks,v.suicide_loss,v.death_loss,v.starting_grenades,v.maximum_one_grenade,
    v.team_mode,v.team_scoring,v.respawn_modifier,v.betrayal_ticks,v.maximum_players,v.starting_weapon,v.weapon_set,
    v.secondary_starting_weapon,v.custom,v.weapon_choices,v.primary_light_vehicle,v.primary_heavy_vehicle,v.weapon,
    v.random_seed,v.spawn_salt,v.king_flags,v.king_move_seconds,v.player_flags)
BIND(Spawn,v.position,v.facing,v.bsp,v.team,v.player_type,v.game_types,v.spawn_types)
BIND(SourceKingMarker,v.position,v.facing,v.type,v.selector12,v.channel,v.remainder)
BIND(SourceWeaponMagazineDefinition,v.initial_rounds,v.maximum_loaded,v.maximum_reserve)
BIND(ObjectFunction,v.flags,v.input,v.output,v.enable,v.threshold,v.curve,v.scale)
BIND(SourceWorldWeaponDefinition,v.name,v.definition,v.model,v.render_model,v.animation_graph,v.default_variant,
    v.item_flags,v.collision_model,v.object_flags,v.multiplayer_scale,v.campaign_scale,v.widget_groups,
    v.owner_function_count,v.magazines,v.functions)
BIND(SourceNetgameWeaponBirth,v.descriptor,v.definition,v.variant,v.position,v.forward,v.up,v.object_scale,v.levitated)
BIND(SourceNetgameWeaponRegistration,v.row,v.definition,v.birth,v.sphere_center,v.sphere_radius)
BIND(FirstPersonAnimationEvent,v.type,v.frame,v.tag,v.id,v.reference_flags,v.marker)
BIND(FirstPersonClockClip,v.activity,v.index,v.frames,v.loop_frame,v.events)
BIND(FirstPersonBindingDefinition,v.representation,v.weapon_definition,v.arms_model,v.weapon_model,v.graph,
    v.arms_nodes,v.weapon_nodes,v.graph_nodes,v.graph_node_flags,v.idle_index,v.record_present,
    v.continuous_fire_animation,v.clock_clips,v.dual_clock_clips,v.ammunition)
BIND(SourceGameOverConstants,v.seconds,v.fade_threshold_seconds,v.ambient_linear_gain,v.fade_seconds)
BIND(SourceResultStatDescriptor,v.minimum,v.maximum,v.remaining)
BIND(SourceDeviceGroup,v.value,v.flags)
BIND(SourceDeviceMotionDefinition,v.flags,v.power_rate,v.power_acceleration,v.position_rate,v.position_acceleration,
    v.unpowered_rate,v.unpowered_acceleration,v.delay,v.effects,v.delay_effect)
BIND(SourceMachineDoorDefinition,v.activation_radius,v.open_seconds)
BIND(SourceDeviceChannel,v.graph,v.animation,v.animation_type,v.clip)
BIND(SourceMachineDefinition,v.name,v.model,v.render_model,v.type,v.object_flags,v.machine_flags,
    v.powered_position_rate,v.unpowered_position_rate,v.door,v.initialized_idle,v.position,v.power)
BIND(SourceMachinePlacement,v.power_group,v.position_group,v.device_flags,v.machine_flags,
    v.initial_power,v.initial_position,v.power_group_flags,v.position_group_flags)
BIND(SourceControlDefinition,v.datum,v.model,v.action_string,v.type,v.triggers_when,v.call_value,v.on,v.off,v.deny)
BIND(ShadowCasterDefinition,v.name,v.bounding_radius,v.bounding_offset,v.mode,v.model,v.model_present,
    v.fade_end,v.fade_start,v.render_cutoffs,v.render_lod_count)
BIND(SourceDeviceWorldDefinition,v.motion,v.placement,v.machine,v.control,v.collision,v.control_bounds,v.control_nodes,
    v.control_hierarchy,v.control_pose,v.control_markers,v.control_permutations,v.door_hierarchy,v.door_permutations,v.one_sided_markers)
#undef BIND
}
std::array<std::byte,32> prop_definitions(const LevelContent &content) {
    Binding b;b(std::string("native-prop-definitions-v1"),content.prop_sandbox.size());
    for(const auto &p:content.prop_sandbox){b(bool(p));if(p)b(*p);}
    b(content.damage_groups,content.damage_materials);return b.finish();
}
std::array<std::byte,32> scenery_definitions(const LevelContent &content) {
    Binding b;b(std::string("native-scenery-collision-definitions-v1"),content.static_scenery_collision.size());
    for(const auto &p:content.static_scenery_collision){b(bool(p));if(p)b(*p);}
    return b.finish();
}
std::array<std::byte,32> device_definitions(const LevelContent &content) {
    Binding b;b(std::string("native-device-definitions-v2"),content.device_groups,content.devices,content.objects.size());
    for(const auto &o:content.objects)b(o.definition,o.type,o.bounding_radius,o.bounding_offset);
    return b.finish();
}
std::array<std::byte,32> bsp_geometry(const BspContent &geometry) {
    Binding b;b(std::string("native-bsp-geometry-v1"),geometry);return b.finish();
}
std::array<std::byte,32> biped_definition(const BipedDefinition &definition) {
    Binding b;b(std::string("native-biped-definition-v1"),definition);return b.finish();
}
std::array<std::byte,32> unit_interaction_definitions(const LevelContent &content) {
    // Shared ownership and allocation addresses are local. Bind every operand
    // consumed by seat admission, posed markers, animations and vehicle rows.
    Binding b;b(std::string("native-unit-interaction-definitions-v1"),content.unit_interactions,content.netgame_equipment,
        content.multiplayer_spawn.has_value(),content.weapons.size());
    for(const auto &[id,weapon]:content.weapons)b(id,weapon.inventory_flags,weapon.capacity);
    return b.finish();
}
std::array<std::byte,32> melee_binding(const Cache &cache,const NativeMeleeConstants &constants,float speed,
    const std::map<std::uint32_t,int> &representations,const std::map<std::uint32_t,std::uint32_t> &graphs) {
    SourceRuntimeResources resources(cache);return melee_binding(resources.identity(),constants,speed,representations,graphs);
}
std::array<std::byte,32> melee_binding(const SourceRuntimeResources::Identity &resources,
    const NativeMeleeConstants &constants,float speed,const std::map<std::uint32_t,int> &representations,
    const std::map<std::uint32_t,std::uint32_t> &graphs) {
    Binding b;b(std::string("native-melee-bindings-v1"),resources,constants,speed,representations,graphs);return b.finish();
}
std::array<std::byte,32> triangles(std::span<const CollisionTriangle> values) {
    Binding b;b(std::string("native-collision-triangles-v1"),values);return b.finish();
}
std::array<std::byte,32> script_program(const ScriptProgram &program) {
    Binding b;b(std::string("native-script-program-v1"),program);return b.finish();
}
std::array<std::byte,32> presentation_content(const LevelContent &content) {
    Binding b;b(std::string("native-presentation-content-v1"),content.camera_points,content.cutscene_flags,bool(content.original_data));
    if(content.original_data)b(content.original_data->image,content.original_data->fade_curve);
    return b.finish();
}
std::array<std::byte,32> grenade_binding(const Cache &cache,std::uint32_t unit,NativeGrenadeConstants constants) {
    const auto image=cache.original_data();
    if(!image || !image->unit_animation_constants || !image->sound_driver_startup_operands)
        throw FormatError("Grenade continuation has no selected-image operands");
    const auto bytes=cache.source_runtime_cache_bytes();std::array<std::byte,32> resident{};
    if(!SHA256(reinterpret_cast<const unsigned char *>(bytes.data()),bytes.size(),reinterpret_cast<unsigned char *>(resident.data())))
        throw FormatError("Cannot bind grenade resident cache");
    Binding b;b(std::string("native-grenade-binding-v1"),resident,unit,image->image,
        image->unit_animation_constants->one_bits,image->unit_animation_constants->frame_subtractor_bits,
        image->sound_driver_startup_operands->basis_up,constants.contact_up_threshold_bits,constants.rest_speed_squared_bits);
    return b.finish();
}
std::array<std::byte,32> grenade_unit(const RenderModel &model,std::span<const ObjectAttachmentMarker> markers,
    std::span<const std::uint8_t> permutations,float speed) {
    Binding b;b(std::string("native-grenade-unit-v1"),model,markers,permutations,speed);return b.finish();
}
std::array<std::byte,32> animation_clip(const AnimationClip &clip) {
    Binding b;b(std::string("native-animation-clip-v1"),clip);return b.finish();
}
std::array<std::byte,32> grenade_clips(const AnimationClip &ordinary,const std::optional<AnimationClip> &preferred) {
    Binding b;b(std::string("native-grenade-clips-v1"),ordinary,bool(preferred));if(preferred)b(*preferred);return b.finish();
}
}
namespace halo2 {
MultiplayerMatch::AuthorityDigest MultiplayerMatch::authority_configuration() const {
    world_continuation::Binding b;
    b(std::string("native-multiplayer-configuration-v1"),rules_,spawns_,king_markers_,world_weapon_definitions_,
      first_person_players_,first_person_definitions_,replicated_fp_definitions_,game_over_constants_,
      defer_initial_player_spawn_,world_weapon_configured_,weapon_pickups_configured_,bool(content_.original_data));
    if(content_.original_data)b(content_.original_data->image,content_.original_data->result_stat_descriptors);
    return b.finish();
}
}
