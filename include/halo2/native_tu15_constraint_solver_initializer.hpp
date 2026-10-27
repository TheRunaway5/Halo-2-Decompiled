#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
namespace halo2 {
struct NativeTu15ConstraintSolverInitializerServices {
    // Actual37C5C0 returns the real32-bit time value consumed by3268D0.
    // The adapter owns any still-unconverted platform clock dependencies.
    std::function<std::uint32_t(NativeTu15PhysicsFrameFp&)> read_time_seconds;
    // Actual2E3850 cdecl message-pointer argument; no success is returned.
    std::function<void(std::uint32_t message_address,NativeTu15PhysicsFrameFp&)> log_failure;
};
struct NativeTu15ConstraintSolverInitializerStats {
    std::uint64_t calls{},client_hashes{},time_calls{},log_calls{},client_ready{},evaluation_ready{};
};
// Actual326640: bounded copied input bytes, signed-byte mixing and unsigned
//32-bit arithmetic. No key text is logged, and no floating state is touched.
std::uint32_t native_tu15_constraint_solver_client_hash(
    SourcePhysicsContactMemory&,std::uint32_t key_pointer);
// Actual3268D0 plus its closed hash. The ready byte is written only after the
// source's actual comparison/date conditions; no dependency supplies success.
// Clock and failure logging remain named real dependencies. The initializer
// itself neither reads the ready byte nor treats an existing1 as an earlyout.
void native_tu15_initialize_constraint_solver(SourcePhysicsContactMemory&,
    NativeTu15PhysicsFrameFp&,const NativeTu15ConstraintSolverInitializerServices&,
    const SourcePhysicsContactRecordObserver& = {},
    NativeTu15ConstraintSolverInitializerStats* = nullptr);
} // namespace halo2
