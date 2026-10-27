#pragma once
#include "halo2/physics_live_world_tu15.hpp"

namespace halo2 {
class Cache;
using SourceFirstPersonPoseReadObserverTu15=
    std::function<void(std::uint32_t,std::size_t)>;

// Read the completed original3D2B0 per-view cache, exactly the input exposed
// by3D4E0 to the first-person collector. The producer must have run for this
// actual view before ordinary WORLD collectors mutate their SIM node banks.
std::vector<SourceFirstPersonModelTu15> source_cached_first_person_models_tu15(
    const Cache &,const SourcePhysicsContactMemory &,std::uint32_t expected_unit);

// Complete 16A4DD composition and 168DED model-order submission over coherent
// source controls, retained transition locals and admitted MCC resources.
// The input bank is read-only. Every native store, including caller scratch,
// sampled locals, camera adjustment and matrices, belongs to private storage.
// No animation clock, event delivery, inventory or resource-I/O owner runs.
// cpu supplies real instruction/estimate/timestamp services; callee dispatch
// is closed over the actual compiled sampler/math owners in this module.
// The native floating-point ABI is 0x027f/0x1f80, identical to the live world's
// retained startup/execution policy. Supplied CPU estimates remain real inputs.
std::vector<SourceFirstPersonModelTu15> source_sample_first_person_pose_tu15(
    const Cache &,const SourcePhysicsContactMemory &,
    const SourcePhysicsWorldRuntimeAbi &caller,unsigned view,
    Vec3 position,Vec3 forward,Vec3 up,
    const SourcePhysicsWorldRuntimeServices &cpu,
    const SourceFirstPersonPoseReadObserverTu15 & = {});
}
