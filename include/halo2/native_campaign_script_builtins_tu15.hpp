#pragma once
#include "halo2/physics_contact_record.hpp"
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string_view>
#include <array>
#include <vector>

namespace halo2 {
// Native semantic kernels for TU1.5 campaign HS builtins that were absent from
// the native script host. Each kernel replicates the post-argument tail of its
// original evaluator (address noted per entry) over the live TU1.5 world bank:
// the ScriptVm owns argument evaluation/conversion and hs_return. No original
// instruction is executed; reads use SourcePhysicsContactMemory and every
// store is published through the caller's journaled writer with the original
// store width and order. A null writer stores through memory.write().
using NativeCampaignScriptWriter=std::function<void(std::uint32_t,std::span<const std::byte>)>;

// Live floating-point controls of the world executing the script. x87 control
// selects precision/rounding of 2AC760 and the FISTP conversions; MXCSR
// selects SSE rounding plus DAZ/FTZ for MULSS/CVTSI2SS.
struct NativeCampaignScriptFloatingPoint {
  std::uint16_t x87_control_word=0x027f;
  std::uint32_t mxcsr=0x1f80;
};

class NativeCampaignScriptBank {
public:
  NativeCampaignScriptBank(SourcePhysicsContactMemory& memory,NativeCampaignScriptWriter writer={},
      NativeCampaignScriptFloatingPoint floating_point={})
      : memory_(memory),writer_(std::move(writer)),floating_point_(floating_point) {}
  std::uint8_t u8(std::uint32_t address)const;
  std::uint16_t u16(std::uint32_t address)const;
  std::uint32_t u32(std::uint32_t address)const;
  void store8(std::uint32_t address,std::uint8_t value);
  void store16(std::uint32_t address,std::uint16_t value);
  void store32(std::uint32_t address,std::uint32_t value);
  // One 8-byte publication (MOVHPS/MOVLPS stores).
  void store64(std::uint32_t address,std::uint32_t low,std::uint32_t high);
  const NativeCampaignScriptFloatingPoint& floating_point()const{return floating_point_;}
  const SourcePhysicsContactMemory& memory()const{return memory_;}
  // One ordered publication of `bytes` through the writer (journal).
  void store(std::uint32_t address,std::span<const std::byte> bytes);
private:
  SourcePhysicsContactMemory& memory_;
  NativeCampaignScriptWriter writer_;
  NativeCampaignScriptFloatingPoint floating_point_;
};

// A read-through view of a world's memory with a private native scratch
// extent. Kernels that only write caller-owned output (marker records) run
// against it without publishing anything into the world.
class NativeCampaignScriptScratch final : public SourcePhysicsContactMemory {
public:
  static constexpr std::uint32_t base=0xfff00000u;
  NativeCampaignScriptScratch(const SourcePhysicsContactMemory& world,std::size_t bytes)
      : world_(world),bytes_(bytes) {}
  void read(std::uint32_t address,std::span<std::byte> output)const override;
  void write(std::uint32_t address,std::span<const std::byte> input)override;
  std::span<const std::byte> bytes()const{return bytes_;}
private:
  const SourcePhysicsContactMemory& world_;
  std::vector<std::byte> bytes_;
};

namespace native_campaign_script_tu15 {
inline constexpr std::uint32_t none=0xffffffffu;

// Object datum helpers shared by the evaluators below.
// BCDF0: salted lookup filtered by the object-type mask; 0 when absent.
std::uint32_t object_try_and_get_and_verify_type(const NativeCampaignScriptBank&,std::uint32_t object,std::uint32_t type_mask);
// B9390: marks the object and its parent chain for update unless already marked.
// 10C180: authored device-group ordinal -> full salted group handle, NONE
// outside the scenario's group block or for rows flagged deleted (bit 4).
std::uint32_t device_group_from_ordinal(const NativeCampaignScriptBank&,std::uint16_t ordinal);
void object_mark_for_update(NativeCampaignScriptBank&,std::uint32_t object);

// Pure scalar and primary-random builtins.
// COMISS honors MXCSR.DAZ; the selected operand's original bits are returned.
std::uint32_t pin(std::uint32_t value,std::uint32_t lower,std::uint32_t upper,std::uint32_t mxcsr); // 2AA9A0, REAL bits
std::uint16_t random_range(NativeCampaignScriptBank&,std::uint16_t lower,std::uint16_t upper); // 2AC6E0, SHORT bits
std::uint32_t real_random_range(NativeCampaignScriptBank&,std::uint32_t lower,std::uint32_t upper); // 2AC760, REAL bits

// Global banks.
void garbage_collect(NativeCampaignScriptBank&,bool unsafe);                       // 2ABD70 / 2ABD90
void physics_set_gravity(NativeCampaignScriptBank&,std::uint32_t scale);           // 2AC840
void physics_set_velocity_frame(NativeCampaignScriptBank&,std::uint32_t i,std::uint32_t j,std::uint32_t k); // 2AC890
void physics_disable_character_ground_adhesion_forces(NativeCampaignScriptBank&,std::uint32_t seconds); // 2AC8F0
void player_effect_set_max_rotation(NativeCampaignScriptBank&,std::uint32_t yaw,std::uint32_t pitch,std::uint32_t roll); // 2B49D0
void player_effect_start(NativeCampaignScriptBank&,std::uint32_t maximum_intensity,std::uint32_t attack_seconds); // 2B4A90
void player_effect_stop(NativeCampaignScriptBank&,std::uint32_t decay_seconds);    // 2B4B10
void pvs_set_object(NativeCampaignScriptBank&,std::uint32_t object);               // 2B2B20
void pvs_clear(NativeCampaignScriptBank&);                                          // 2B2BD0

// Objects.
std::uint32_t object_get_parent(const NativeCampaignScriptBank&,std::uint32_t object);          // 2AB8E0
void object_set_phantom_power(NativeCampaignScriptBank&,std::uint32_t object,std::uint8_t enabled); // 2AB560
void object_set_deleted_when_deactivated(NativeCampaignScriptBank&,std::uint32_t object);       // 2ABB30
void object_set_shield_stun_infinite(NativeCampaignScriptBank&,std::uint32_t object);           // 2AC370
std::uint32_t object_get_shield(const NativeCampaignScriptBank&,std::uint32_t object);          // 2AB7A0, REAL bits
std::uint32_t object_get_health(const NativeCampaignScriptBank&,std::uint32_t object);          // 2AB730, REAL bits
// 2AC290: shield = maximum shield (+E8) * clamp(fraction, 0, 1).
void object_set_shield(NativeCampaignScriptBank&,std::uint32_t object,std::uint32_t fraction);
void object_clear_all_function_variables(NativeCampaignScriptBank&,std::uint32_t object);       // 2AB4D0 -> 10FD90
void unit_kill(NativeCampaignScriptBank&,std::uint32_t unit,bool silent);                       // 2AD040 / 2AD0B0

// Devices.
void device_operates_automatically_set(NativeCampaignScriptBank&,std::uint32_t device,std::uint8_t enabled); // 2AE7C0 -> 10C640
void device_closes_automatically_set(NativeCampaignScriptBank&,std::uint32_t device,std::uint8_t enabled);   // 2AE810 -> 10C690
void device_one_sided_set(NativeCampaignScriptBank&,std::uint32_t device,std::uint8_t enabled);              // 2AE740
void device_set_never_appears_locked(NativeCampaignScriptBank&,std::uint32_t device,std::uint8_t enabled);   // 2AE3D0
void device_group_change_only_once_more_set(NativeCampaignScriptBank&,std::uint32_t group,std::uint8_t enabled); // 2AE860 -> 10C6E0
std::uint32_t device_get_position(const NativeCampaignScriptBank&,std::uint32_t device); // 2AE570
std::uint32_t device_group_get(const NativeCampaignScriptBank&,std::uint32_t group); // 2AE630
void device_group_set_immediate(NativeCampaignScriptBank&,std::uint32_t group,std::uint32_t position); // 10C4E0
void device_set_position_immediate(NativeCampaignScriptBank&,std::uint32_t device,std::uint32_t position); // 10C420

// Units and vehicles.
bool unit_in_vehicle(const NativeCampaignScriptBank&,std::uint32_t unit);                       // 2ADA00
void unit_only_takes_damage_from_players_team(NativeCampaignScriptBank&,std::uint32_t unit,std::uint8_t enabled); // 2AD7A0 -> 11DB40
// The caller supplies the HS list's members in original reference order;
// iteration stops at the first NONE member exactly like 1E7130 consumers.
void unit_doesnt_drop_items(NativeCampaignScriptBank&,std::span<const std::uint32_t> members); // 2AE200 -> 11CD60
bool unit_is_emitting(const NativeCampaignScriptBank&,std::uint32_t unit);                      // 2AD120 -> 11D040
std::uint32_t vehicle_driver(const NativeCampaignScriptBank&,std::uint32_t unit);               // 2ADF90

// Scenario interpolators: returns the LONG index of the started row or -1.
std::uint32_t interpolator_start(NativeCampaignScriptBank&,std::uint32_t name,std::uint32_t target,std::uint32_t seconds); // 2B56D0 -> 137BF0

// Presentation and object-state owners reached from campaign builtins
// (native_campaign_script_presentation_tu15.cpp). Arguments are the original
// routine inputs after its script evaluator; REAL values travel as bits.
void object_cinematic_lod(NativeCampaignScriptBank&,std::uint32_t object,std::uint8_t active);          // BDF70
void unit_set_enterable_by_player(NativeCampaignScriptBank&,std::uint32_t unit,std::uint8_t enabled);   // 11DB00
void object_clear_function_variable(NativeCampaignScriptBank&,std::uint32_t object,std::uint32_t name); // 10FD50
void texture_camera_off(NativeCampaignScriptBank&);                                                     // 255C0
void cinematic_shadow_control(NativeCampaignScriptBank&,std::uint8_t enabled,std::uint32_t object,
    std::uint32_t marker,std::uint32_t radius);                                                         // 30E10
// Seven REAL bits: near/far/blur triples of the original 1FC10 inputs.
void cinematic_depth_of_field(NativeCampaignScriptBank&,std::span<const std::uint32_t,7>);            // 1FC10
void camera_set_field_of_view(NativeCampaignScriptBank&,std::uint32_t degrees,std::uint16_t ticks);     // 16F5A0
void cinematic_stop(NativeCampaignScriptBank&);                                                         // 13F560
// 10FB70: interpolated value (REAL bits) and slot of an object's scripted
// function variable; nullopt when no slot holds (object, name).
struct NativeCampaignFunctionValue {std::uint32_t value{},slot{};};
std::optional<NativeCampaignFunctionValue> object_function_value(const NativeCampaignScriptBank&,
    std::uint32_t object,std::uint32_t name);
// 10FC30: frames are the REAL frame count before the original 1/30 scale.
void object_set_function_variable(NativeCampaignScriptBank&,std::uint32_t object,std::uint32_t name,
    std::uint32_t value,std::uint32_t frames);
// BACD0: object whose model supplies markers (parent inheritance, held items).
std::uint32_t object_marker_owner(const NativeCampaignScriptBank&,std::uint32_t object);
// BAD60: 0x70-byte marker records (node, local 0x34 matrix, world 0x34
// matrix, radius) for a marker name, written at `output`. Returns the
// original SHORT count; falls back to the node-0 transform.
std::uint16_t object_get_markers(NativeCampaignScriptBank&,std::uint32_t object,std::uint32_t marker,
    std::int16_t count,bool owner_resolved,std::uint32_t output);
// Trigger volumes (scenario +10C rows): 11EBA0 point test over the 11EAB0
// frame (attached volumes use their object's named node), the 2AAB50 object
// test and 2A93D0 any/all list tests (members stop at the first NONE).
bool trigger_volume_test_point(const NativeCampaignScriptBank&,std::int16_t volume,std::uint32_t point);
bool volume_test_object(const NativeCampaignScriptBank&,std::int16_t volume,std::uint32_t object);
bool volume_test_objects(const NativeCampaignScriptBank&,std::int16_t volume,std::span<const std::uint32_t> members,bool all);
// Spatial queries behind volume_return_objects(_by_type) (11ED20).
// 14CDB0: collision-BSP leaf containing a point (NONE outside).
std::uint32_t bsp_leaf(const NativeCampaignScriptBank&,std::uint32_t bsp,std::array<std::uint32_t,3> point);
// 11E600: current BSP index, leaf and cluster of a point.
struct NativeCampaignLocation {std::uint32_t leaf{};std::uint16_t cluster{},bsp{};};
NativeCampaignLocation scenario_location(const NativeCampaignScriptBank&,std::array<std::uint32_t,3> point);
// BD080: objects whose cluster references overlap the sphere, publishing the
// original query counters, flags and per-cluster/per-object visit marks.
std::vector<std::uint32_t> objects_in_sphere(NativeCampaignScriptBank&,std::uint32_t list_flags,
    std::uint32_t type_mask,const NativeCampaignLocation&,std::array<std::uint32_t,3> center,
    std::uint32_t radius,std::int16_t maximum,std::uint32_t buffer=0xffffffffu);
// 11ED20 after its list allocation: members in original append order.
std::vector<std::uint32_t> volume_return_objects(NativeCampaignScriptBank&,std::int16_t volume,std::uint32_t type_mask);
// View tests: CE790 (head marker, normalized direction, aim dot > cos), and
// the 2A9680 flag / 2A9560->2A94B0 object list evaluators.
bool unit_can_see_point(const NativeCampaignScriptBank&,std::uint32_t viewer,std::array<std::uint32_t,3> point,std::uint32_t angle);
bool objects_can_see_flag(const NativeCampaignScriptBank&,std::span<const std::uint32_t> viewers,std::int16_t flag,std::uint32_t degrees);
bool objects_can_see_object(const NativeCampaignScriptBank&,std::span<const std::uint32_t> viewers,std::uint32_t target,std::uint32_t degrees);
// AI references (tag 0 squad, 1 encounter group, 2 actor, 3 starting
// location). 27C880 resolution and the evaluator owners of
// native_campaign_script_ai_tu15.cpp.
std::uint32_t ai_actor_index(const NativeCampaignScriptBank&,std::uint32_t reference);
std::uint32_t ai_vehicle_get_from_starting_location(const NativeCampaignScriptBank&,std::uint32_t reference);
std::uint32_t ai_vehicle_get(const NativeCampaignScriptBank&,std::uint32_t reference);
std::uint32_t object_get_ai(const NativeCampaignScriptBank&,std::uint32_t object);
std::uint16_t ai_spawn_count(const NativeCampaignScriptBank&,std::uint32_t reference);
std::uint16_t ai_combat_status(const NativeCampaignScriptBank&,std::uint32_t reference);
void ai_disposable(NativeCampaignScriptBank&,std::uint32_t reference,std::uint8_t);
void ai_set_blind(NativeCampaignScriptBank&,std::uint32_t reference,std::uint8_t);
void ai_set_deaf(NativeCampaignScriptBank&,std::uint32_t reference,std::uint8_t);
void ai_suppress_combat(NativeCampaignScriptBank&,std::uint32_t reference,std::uint8_t);
void ai_magically_see_object(NativeCampaignScriptBank&,std::uint32_t reference,std::uint32_t object);
bool ai_vitality_pinned(const NativeCampaignScriptBank&,std::uint32_t reference);
std::uint32_t character_equipment_row(const NativeCampaignScriptBank&,std::uint32_t character,std::uint16_t type);
void ai_renew(NativeCampaignScriptBank&,std::uint32_t reference);
// 27CAA0/27CB30 as a pull walk: each call yields the next actor row of the
// reference (squad, encounter group, actor or starting location), 0 at the
// end. The bank must outlive the walk. (27EAB0 ai_vehicle_exit's iterator.)
std::function<std::uint32_t()> ai_reference_actors(const NativeCampaignScriptBank&,std::uint32_t reference);
// Preserve27CB30's full chain datum as well as its row. It is not recomputed
// from the row salt: callers such as2801D0 consume the iterator's identity.
struct AiReferenceActor {std::uint32_t row{},datum{0xffffffffu};};
std::function<AiReferenceActor()> ai_reference_actor_datums(const NativeCampaignScriptBank&,std::uint32_t reference);
struct VehicleSeat {std::uint32_t object;std::uint16_t seat;std::uint32_t row;};
std::vector<VehicleSeat> vehicle_seats(const NativeCampaignScriptBank&,std::uint32_t object);
bool vehicle_test_seat(const NativeCampaignScriptBank&,std::uint32_t vehicle,std::uint32_t seat_name,std::uint32_t unit);
bool vehicle_test_seat_list(const NativeCampaignScriptBank&,std::uint32_t vehicle,std::uint32_t seat_name,
    std::span<const std::uint32_t> members);
bool ai_vehicle_reserve(NativeCampaignScriptBank&,std::uint32_t vehicle,std::uint8_t reserve);
bool ai_vehicle_reserve_seat(NativeCampaignScriptBank&,std::uint32_t vehicle,std::uint32_t seat_name,std::uint8_t reserve);
void player_profile_normalize(NativeCampaignScriptBank&,std::uint32_t record);
void weapon_hold_trigger(NativeCampaignScriptBank&,std::uint32_t weapon,std::uint32_t trigger,std::uint8_t hold);
void team_nav_point(NativeCampaignScriptBank&,bool activate,std::uint16_t type,std::uint16_t team,std::uint32_t kind,
    std::uint32_t target,std::uint32_t offset);
void unit_set_active_camo(NativeCampaignScriptBank&,std::uint32_t unit,std::uint8_t enable,std::uint32_t seconds);
void biped_ragdoll(NativeCampaignScriptBank&,std::uint32_t unit);
void units_set_maximum_vitality(NativeCampaignScriptBank&,std::span<const std::uint32_t> objects,
    std::uint32_t body_vitality,std::uint32_t shield_vitality);
bool player_flashlight_on(const NativeCampaignScriptBank&);
std::uint8_t ice_cream_flavor_available(const NativeCampaignScriptBank&,std::uint32_t flavor);
void ai_set_active_camo(NativeCampaignScriptBank&,std::uint32_t reference,std::uint8_t enable);
void ai_berserk(NativeCampaignScriptBank&,std::uint32_t reference,std::uint8_t berserk);
std::uint32_t flock_find(const NativeCampaignScriptBank&,std::uint32_t name);
bool flock_set_active(NativeCampaignScriptBank&,std::uint32_t name,bool active);
void ai_magically_see(NativeCampaignScriptBank&,std::uint32_t viewers,std::uint32_t targets);
std::uint16_t object_model_targets_destroyed(const NativeCampaignScriptBank&,std::uint32_t object,std::uint32_t target);
std::uint32_t object_at_marker(const NativeCampaignScriptBank&,std::uint32_t object,std::uint32_t marker);
std::uint32_t sound_effect_reference(const NativeCampaignScriptBank&,std::uint32_t name);
std::uint32_t sound_effect_collection(const NativeCampaignScriptBank&,std::uint32_t reference);
void ai_command_context(NativeCampaignScriptBank&,std::optional<std::uint32_t> unit);
// constant: original .data value the world image does not map (read-only).
struct NativeCampaignBackedGlobal {std::uint32_t address;std::uint16_t type;std::optional<std::uint16_t> constant;};
std::optional<NativeCampaignBackedGlobal> backed_global(std::uint32_t xbox_index);
void ai_target_preference(NativeCampaignScriptBank&,std::span<const std::uint32_t> objects,std::uint8_t enable,bool prefer);
// 158560: scripted camera control for all four local users. Returns nothing;
// the inactive path classifies each user's seat (158340) and rebinds the
// original camera mode (158900 with its flag clear).
void camera_control(NativeCampaignScriptBank&,std::uint8_t active);
// 32A4E0: the original word-optimized strncpy, reproducing its stores.
void original_strncpy(NativeCampaignScriptBank&,std::uint32_t destination,std::uint32_t source,std::uint32_t count);
// 255E0: object (NONE clears), marker string id and field of view bits.
void texture_camera_set_object_marker(NativeCampaignScriptBank&,std::uint32_t object,std::uint32_t marker,std::uint32_t fov);
// 10D6B0/10D720 -> 10CB60: device position/overlay velocity profiles. REAL
// bits: target position, total, acceleration and deceleration seconds.
void device_animate_position(NativeCampaignScriptBank&,std::uint32_t device,std::uint32_t position,
    std::uint32_t seconds,std::uint32_t acceleration,std::uint32_t deceleration,std::uint8_t keep_velocity);
void device_animate_overlay(NativeCampaignScriptBank&,std::uint32_t device,std::uint32_t position,
    std::uint32_t seconds,std::uint32_t acceleration,std::uint32_t deceleration);
// 111080/1110D0: TU1.5 emotion name id, weight and transition seconds (bits).
void unit_set_emotion(NativeCampaignScriptBank&,std::uint32_t unit,std::uint32_t emotion,
    std::uint32_t weight,std::uint32_t seconds);

// Object lists.
std::uint16_t list_count_not_dead(const NativeCampaignScriptBank&,std::span<const std::uint32_t> members); // 2AADE0 -> 1E7260
}

// Builtins without any state effect in TU1.5 (2AC980 returns void at once).
bool native_campaign_script_noop_builtin_tu15(std::string_view name);
}
