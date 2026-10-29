#pragma once
#include "halo2/physics_world_runtime.hpp"

namespace halo2 {
class SourceRuntimeMemory;
// New subsystem storage only. Existing live writable owners are retained;
// shared immutable operands must match before any uncovered span is mounted.
void mount_native_tu15_input_collection_data(SourceRuntimeMemory &);
bool source_input_collection_tu15_owns_entry(std::uint32_t);
// Complete1889D0/1898A0 collection and continuation, with their original
// UI/mask/selector/zoom/look gates.193A4C/21E650/21EF20 retain the original
// profile-to-preset and physical-to-mapped input processing. The caller owns
// actual device/profile/sample history, local roster and preceding camera.
// No returned target, command, scope decision or callee result is supplied.
void source_run_input_collection_tu15(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
}
