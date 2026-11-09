#pragma once

#include "halo2/native_tu15_island_activation.hpp"
#include "halo2/native_tu15_retained_store_observer.hpp"

namespace halo2 {

// An input vector may be retained DATA or a typed result from the force owner.
// Each lane is requested at the original read phase, and no fourth lane is read.
using NativeTu15VelocityWords = std::function<std::uint32_t(unsigned)>;

enum class NativeTu15PhantomVelocityKind { linear, angular };

// Actual1D9080/1D9100 through315C90, conditional native315CB0, and the
// selected3239F0/323A00 virtuals. No Source stack image or virtual executor.
void native_tu15_set_phantom_body_velocity(
    SourcePhysicsContactMemory &, std::uint32_t component, std::uint32_t slot,
    NativeTu15PhantomVelocityKind, const NativeTu15VelocityWords &,
    NativeTu15PhysicsFrameFp &, const NativeTu15IslandActivationServices &,
    const NativeTu15RetainedStoreObserver & = {}, NativeTu15IslandActivationStats * = nullptr);

// Actual1D92C0. Use the current motion or authored zero vector and preserve the
// ordered ADDSS operands before reselecting the body through native1D9080.
void native_tu15_add_phantom_body_linear_velocity(
    SourcePhysicsContactMemory &, std::uint32_t component, std::uint32_t slot,
    const NativeTu15VelocityWords &, NativeTu15PhysicsFrameFp &, unsigned incoming_x87_depth,
    const NativeTu15IslandActivationServices &, const NativeTu15RetainedStoreObserver & = {},
    NativeTu15IslandActivationStats * = nullptr);

} // namespace halo2
