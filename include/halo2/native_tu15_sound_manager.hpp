#pragma once
#include "halo2/physics_contact_record.hpp"
#include "halo2/physics_contact_runtime.hpp"
#include <cstdint>
#include <functional>
#include <string>

namespace halo2 {
// Native C++ kernels for TU1.5 sound manager routines. Each one replaces a
// lifted routine at its retail entry: the caller's call instruction has
// pushed the return address, and the kernel returns with the same registers,
// x87/SSE state and memory stores as the original function. EFLAGS after the
// return is not reproduced; compiled callers never read it across a call.
bool native_tu15_sound_manager_owns_entry(std::uint32_t entry);
void native_tu15_sound_manager_run(SourcePhysicsContactMemory &, std::uint32_t entry,
    SourcePhysicsContactRuntimeAbi &, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRecordObserver & = {});

// Parity oracle: predicts the native result over a copy-on-write view, runs
// the reference (lifted) routine on the real state, then compares registers,
// x87/SSE state and every store outside the callee's dead stack scratch.
// Returns a description of the first difference, or an empty string.
std::string native_tu15_sound_manager_compare(SourcePhysicsContactMemory &, std::uint32_t entry,
    SourcePhysicsContactRuntimeAbi &, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRecordObserver &,
    const std::function<void(SourcePhysicsContactMemory &, SourcePhysicsContactRuntimeAbi &,
        const SourcePhysicsContactRecordObserver &)> &reference);

enum class NativeTu15SoundManagerMode { native, reference, oracle };
// HALO2_REFERENCE_SOUND_MANAGER=1 restores the lifted routines;
// HALO2_SOUND_MANAGER_ORACLE=1 runs both and throws on the first difference.
NativeTu15SoundManagerMode native_tu15_sound_manager_mode();
} // namespace halo2
