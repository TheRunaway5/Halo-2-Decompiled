#pragma once
#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_snapshot_origin.hpp"
#include "halo2/source_device_world.hpp"
#include "halo2/effect_sound.hpp"
#include <map>
#include <set>

namespace halo2 {
struct MultiplayerRuntimeFiringOwner {
    std::uint32_t weapon=null_datum,presentation=null_datum,source=null_datum;
    bool operator==(const MultiplayerRuntimeFiringOwner &)const=default;
};
// Owned values at the accepted CPU-effects boundary, before deferred frame
// audio/prop processing and rendering. The current decoded state belongs to
// the enclosing authentic receipt; previous is its actual completed-tick
// interpolation endpoint, not a caller-supplied same-tick display snapshot.
// This is one prerequisite of complete adoption, not a renderer/device/playing
// audio checkpoint or permission to advertise migration readiness.
struct MultiplayerRuntimeReceiptFrontier {
    MultiplayerSnapshotOrigin previous;
    std::vector<unsigned> local_slots;
    bool client_state_bound{},joined{};
    std::uint64_t world_impact_serial{},world_melee_serial{},world_detonation_serial{},
        world_unit_effect_serial{},world_voice_serial{},rumble_serial{};
    std::uint32_t client_effect_serial{},world_weapon_effect_serial{};
    bool world_effect_baseline{},client_effect_baseline{},client_prop_effect_baseline{};
    NativeDeviceEventCursor device_events{false};
    std::set<std::uint32_t> heard_prop_effects,attached_effect_objects;
    std::map<std::pair<std::uint16_t,std::uint32_t>,MultiplayerRuntimeFiringOwner> firing_instances;
    std::array<std::uint32_t,multiplayer_capacity> heard_shots{};
    std::optional<std::uint64_t> world_effect_tick;
    std::optional<std::uint32_t> muzzle_pose_tick;
    std::vector<std::array<std::uint32_t,3>> muzzle_pose_owners;
    // These requests have not reached the local audio device at this boundary.
    // In particular, elapsed time must not be advanced once per receipt when
    // a render frame consumes several received snapshots.
    std::vector<SoundEvent> sounds;
    std::vector<std::pair<std::uint32_t,std::uint32_t>> weapon_sounds;
    // Filled by the history's fresh capture of its bound sound owner. Input
    // frontiers leave this empty; stale same-owner checkpoints are rejected.
    NativeEffectSounds::Checkpoint pending_effect_sounds;
};
}
