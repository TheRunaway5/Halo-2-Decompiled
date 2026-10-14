#pragma once
#include "halo2/cache.hpp"
#include "halo2/multiplayer_netgame_postprocess.hpp"
#include "halo2/full_scoreboard.hpp"
#include "halo2/unit_scope_camera.hpp"
#include "halo2/death_camera.hpp"
#include "halo2/observer_motion.hpp"
#include "halo2/observer_world.hpp"
#include "halo2/full_scoreboard_providers.hpp"
#include "halo2/hud_bitmap_geometry.hpp"
#include "halo2/unit_animation_constants.hpp"
#include "halo2/native_grenade_constants.hpp"
#include "halo2/native_melee_animation.hpp"
#include "halo2/fp_sound_request.hpp"
#include "halo2/sound_object_request.hpp"
#include "halo2/unit_action_tables.hpp"
#include "halo2/unit_constructor_tail.hpp"
#include "halo2/source_game_over.hpp"
#include "halo2/source_result_statistics.hpp"
#include "halo2/source_result_capture.hpp"
#include "halo2/sound_driver_parameters.hpp"
#include "halo2/sound_sdk_spatial_data.hpp"
#include "halo2/sound_startup.hpp"
#include "halo2/sound_cache_startup.hpp"
#include "halo2/sound_map_activation.hpp"
#include "halo2/sound_driver_startup_operands.hpp"
#include "halo2/sound_driver_listener.hpp"
#include "halo2/sound_submit_operands.hpp"
#include "halo2/sound_promotion.hpp"
#include "halo2/sound_create.hpp"
#include "halo2/sound_driver_stream.hpp"
#include "halo2/sound_driver_master_gain.hpp"
#include "halo2/sound_admission.hpp"
#include "halo2/object_construction_tables.hpp"
#include "halo2/object_construction.hpp"
#include "halo2/source_scenario_request.hpp"
#include "halo2/sound_looping.hpp"
#include "halo2/object_preparation_pressure.hpp"
#include "halo2/physics_component_construction.hpp"
#include "halo2/physics_component_rebuild.hpp"

namespace halo2 {
struct FullScoreboardPaintConstants;
// Data supplied by the user, separate from native algorithms. A future asset
// adapter can provide the same in-memory tables without changing their users.
struct OriginalData {
  enum class Image { Base, MapPack11, TitleUpdate15 } image = Image::Base;
  std::filesystem::path source;
  std::array<Vec3, 1026> weapon_directions{};
  std::array<std::uint8_t, 1024> fade_curve{};
  std::array<std::array<std::int16_t,3>,6> collision_projection{};
  // Raw float words in first-encounter order of original 164750 / 165B30
  // constant reads, excluding runtime pointer globals. The base image has no
  // patch. Addresses/order are frozen in MP_ORIGINAL_DATA_U5_RESEARCH.md.
  std::optional<std::array<std::uint32_t,19>> multiplayer_balance_constants;
  // Exact source operands for 11DF00 / 11F060. Base has no patch;
  // MapPack11 has no Relic branch or manufactured Relic constants.
  std::optional<MultiplayerNetgamePostprocessConstants> multiplayer_netgame_postprocess_constants;
  // Original full-roster layout/fade words; presentation and local player
  // scheduling still have separate owners. Base-image addresses are unverified.
  std::optional<FullScoreboardConstants> full_scoreboard_constants;
  // Verified original static-initializer operands and observer FOV constants.
  // Runtime BSS destinations are deliberately not read as authored literals.
  std::optional<UnitScopeCameraConstants> unit_scope_camera_constants;
  std::optional<SourceObserverConstants> observer_constants;
  std::optional<SourceObserverWorldConstants> observer_world_constants;
  std::optional<DeathCameraConstants> death_camera_constants;
  std::optional<FullScoreboardNameMappings> full_scoreboard_name_mappings;
  std::optional<FullScoreboardColorBits> full_scoreboard_palette_fallback;
  std::shared_ptr<const FullScoreboardPaintConstants> full_scoreboard_paint_constants;
  std::optional<HudBitmapGeometryConstants> hud_bitmap_geometry_constants;
  // Original player animation operands are verified for disc, 1.1 and 1.5.
  // Each image supplies its own immutable words; no values are substituted.
  std::optional<UnitAnimationConstants> unit_animation_constants;
  std::optional<NativeGrenadeConstants> grenade_constants;
  std::optional<NativeMeleeConstants> melee_constants;
  std::optional<std::uint32_t> unit_locomotion_deadzone_bits;
  std::optional<std::array<std::uint32_t,3>> unit_facing_constants;
  // Immutable marker basis and callback identities, verified for 1.1/1.5.
  // Current BSP, object transforms, listener and source voices are live owners.
  std::optional<FirstPersonSoundConstants> first_person_sound_constants;
  std::optional<SoundObjectRequestConstants> sound_object_request_constants;
  // Actual60 action callbacks and verified biped/weapon/projectile allocation
  // records from the selected image. Base-image tables remain unverified.
  std::optional<SourceUnitActionTables> unit_action_tables;
  std::optional<UnitConstructorTailConstants> unit_constructor_tail_constants;
  std::optional<SourceGameOverConstants> game_over_constants;
  std::optional<SourceResultStatDescriptors> result_stat_descriptors;
  std::optional<SourceResultCaptureConstants> result_capture_constants;
  std::optional<SoundDriverParameterConstants> sound_driver_parameter_constants;
  // Complete SDK literal/vector/kernel mappings verified across all three
  // supported images. Original identities are resolved to owned bytes only.
  std::optional<SoundSdkSpatialData> sound_sdk_spatial_data;
  // Exact immutable startup operands. Arena, clocks/RNG, cache, real device
  // construction and live map activation are separately owned services.
  std::optional<SoundStartupConstants> sound_startup_constants;
  std::optional<SoundCacheStartupConstants> sound_cache_startup_constants;
  // Mutable environment packets, saved routing and driver/listener bindings
  // remain live owners; only the two verified immutable operands are imported.
  std::optional<SoundMapActivationConstants> sound_map_activation_constants;
  // Driver's one word and two basis vectors only. Mutable routing plugin,
  // saved settings, runtime environment packet and device state are absent.
  std::optional<SoundDriverStartupImmutableOperands> sound_driver_startup_operands;
  std::optional<SoundDriverListenerConstants> sound_driver_listener_constants;
  std::optional<SoundSubmitImmutableOperands> sound_submit_operands;
  // Immutable arithmetic words only. Promotion counters, cooldowns and the
  // map clock remain retained mutable source owners.
  std::optional<SoundPromotionConstants> sound_promotion_constants;
  // Original gain/range/RNG arithmetic only. Mutable delay scale, class fades,
  // pools, local RNG state and cache admission remain actual runtime owners.
  std::optional<SoundCreateConstants> sound_create_constants;
  // Authenticated arithmetic, queue/method/callback identities only. Mutable
  // queues, format padding, driver storage and platform streams remain owned.
  std::optional<SoundDriverStreamConstants> sound_driver_stream_constants;
  // Only the three authenticated master-output arithmetic operands. Driver
  // allocation, output globals, preferences and voice state remain mutable.
  std::optional<SoundDriverMasterGainConstants> sound_driver_master_gain_constants;
  // One extra immutable occlusion operand plus the existing create constants.
  // Mutable availability, listener, scene, RNG and request backing stay owned.
  std::optional<SoundAdmissionConstants> sound_admission_constants;
  // All13 authenticated type pointers/groups/extents, inheritance slots and methods.
  // Current tag-group lookups and complete method bodies remain live owners.
  std::shared_ptr<const SourceObjectConstructionTables> object_construction_tables;
  // Literal two-byte construction sentinel and shared one operand only.
  // Object, unit, action, world and allocation storage remain live owners.
  std::optional<ObjectConstructionOperands> object_construction_operands;
  std::optional<ObjectCreationRequestOperands> object_creation_request_operands;
  std::optional<SourceScenarioRequestOperands> scenario_request_operands;
  // Authenticated class names/selectors and arithmetic/method identities.
  // No mutable engine, pool, driver, fade or allocator state is imported.
  std::optional<SoundLoopingConstants> sound_looping_constants;
  // Authenticated immutable pressure thresholds, clock arithmetic and indexed
  // physical rebuild tables only. Heap/pool/clock/world state stays owned.
  std::optional<SourceObjectPressureOperands> object_pressure_operands;
  std::optional<PhysicsComponentConstructionConstants> physics_component_construction_constants;
  std::optional<PhysicsComponentRebuildImmutableOperands> physics_component_rebuild_operands;
  // Compiled, typed immutable operands recovered from the TU1.5 reference.
  // This performs no executable/ISO I/O and supplies no mutable world state.
  static std::shared_ptr<const OriginalData> native_tu15();
  static std::shared_ptr<const OriginalData> load_xbe(const std::filesystem::path &);
};
} // namespace halo2
