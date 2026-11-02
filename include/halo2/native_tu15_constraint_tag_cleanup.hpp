#pragma once
#include "halo2/native_tu15_constraint_runtime_handles.hpp"
namespace halo2 {
// Actual17A030's shared native semantic entry. Record is ordinary retained
// DATA; no original caller ABI or volatile-register postimage is accepted.
void native_tu15_cleanup_constraint_handle_tagged_values(SourcePhysicsContactMemory&,
    std::uint32_t record,NativeTu15PhysicsFrameFp&,
    const NativeTu15ConstraintRuntimeHandleServices&,
    NativeTu15ConstraintRuntimeHandleStats* = nullptr);
} // namespace halo2
