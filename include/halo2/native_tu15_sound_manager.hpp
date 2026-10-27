#pragma once
#include "halo2/physics_contact_record.hpp"
#include "halo2/physics_contact_runtime.hpp"
#include <cstdint>
#include <array>
#include <functional>
#include <optional>
#include <string>

namespace halo2 {
// Native C++ kernels for TU1.5 sound manager routines. Each one replaces a
// lifted routine at its retail entry: the caller's call instruction has
// pushed the return address, and the kernel returns with the same registers,
// x87/SSE state and memory stores as the original function. EFLAGS after the
// return is not reproduced; compiled callers never read it across a call.
bool native_tu15_sound_manager_owns_entry(std::uint32_t entry);
// Services::required_owner runs any original routine a kernel calls that
// is not native yet.
void native_tu15_sound_manager_run(SourcePhysicsContactMemory &, std::uint32_t entry,
    SourcePhysicsContactRuntimeAbi &, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRecordObserver & = {}, const SourcePhysicsContactRuntimeServices & = {});
// True when the kernel calls a routine that is not native; the oracle then
// runs only the reference (its prediction would execute that callee twice).
bool native_tu15_sound_manager_calls_out(std::uint32_t entry);

// Parity oracle: predicts the native result over a copy-on-write view, runs
// the reference (lifted) routine on the real state, then compares registers,
// x87/SSE state and every store outside the callee's dead stack scratch.
// Returns a description of the first difference, or an empty string.
std::string native_tu15_sound_manager_compare(SourcePhysicsContactMemory &, std::uint32_t entry,
    SourcePhysicsContactRuntimeAbi &, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRecordObserver &,
    const std::function<void(SourcePhysicsContactMemory &, SourcePhysicsContactRuntimeAbi &,
        const SourcePhysicsContactRecordObserver &)> &reference,
    const SourcePhysicsContactRuntimeServices & = {});

struct SourceObjectAudioViewTu15;
// The native object-audio frame (TU1.5 18EE50) with the local camera views;
// available once the whole object looping-sound group is native.
void native_tu15_object_audio_frame(SourcePhysicsContactMemory &, SourcePhysicsContactRuntimeAbi &,
    SourcePhysicsFactoryFloatingPoint, const SourcePhysicsContactRuntimeServices &,
    const std::array<std::optional<SourceObjectAudioViewTu15>,4> *views);

// The oracle step for any native routine: run predicts over a copy-on-write
// view of m, then reference runs on m; owed registers per bitmask.
std::string native_tu15_sound_compare_with(SourcePhysicsContactMemory &, SourcePhysicsContactRuntimeAbi &,
    const SourcePhysicsContactRecordObserver &,
    const std::function<void(SourcePhysicsContactMemory &, SourcePhysicsContactRuntimeAbi &)> &run,
    const std::function<void(SourcePhysicsContactMemory &, SourcePhysicsContactRuntimeAbi &,
        const SourcePhysicsContactRecordObserver &)> &reference,
    std::uint8_t gp_full, std::uint8_t gp_low16, std::uint8_t xmm, std::uint32_t entry);
// Object-loop routine at an original address, native where converted (tests).
void native_tu15_object_loop_run(SourcePhysicsContactMemory &, std::uint32_t address,
    SourcePhysicsContactRuntimeAbi &, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRuntimeServices &,
    const std::array<std::optional<SourceObjectAudioViewTu15>,4> *views);

// While a scope is open on this thread, the live world routes every sound
// entry to its lifted reference (an oracle's reference run).
struct NativeTu15SoundReferenceScope {
    NativeTu15SoundReferenceScope();~NativeTu15SoundReferenceScope();
    NativeTu15SoundReferenceScope(const NativeTu15SoundReferenceScope &)=delete;
};
bool native_tu15_sound_reference_active();

// Campaign audio routine at an original address, native where converted (tests).
void native_tu15_campaign_audio_run(SourcePhysicsContactMemory &, std::uint32_t address,
    SourcePhysicsContactRuntimeAbi &, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRuntimeServices &);

enum class NativeTu15SoundManagerMode { native, reference, oracle };
// HALO2_REFERENCE_SOUND_MANAGER=1 restores the lifted routines;
// HALO2_SOUND_MANAGER_ORACLE=1 runs both and throws on the first difference.
NativeTu15SoundManagerMode native_tu15_sound_manager_mode();
// HALO2_SOUND_ENTRY_CENSUS=1: count every sound entry crossing the live-world
// dispatcher and print the counts at exit (which routines a game executes).
bool native_tu15_sound_census_enabled();
void native_tu15_sound_census(std::uint32_t entry,bool native);
} // namespace halo2
