#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
namespace halo2 {
struct NativeTu15ConstraintTagValueStats {
    std::uint64_t calls{},disabled_skips{},invalid_inputs{},iterator_queries{},candidates{},
        context_matches{},reference_matches{},class_matches{},publications{};
};
// ActualC6120 with native16EA60. The current runtime tag table supplies the
// actual definition addresses; this is not a cache/provider replacement.
void native_tu15_publish_constraint_tag_value(SourcePhysicsContactMemory&,
    std::uint32_t reference,std::uint32_t context,std::uint32_t scalar_bits,
    NativeTu15PhysicsFrameFp&,const SourcePhysicsContactRecordObserver& = {},
    NativeTu15ConstraintTagValueStats* = nullptr);
} // namespace halo2
