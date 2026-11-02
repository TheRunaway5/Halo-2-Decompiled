#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
namespace halo2 {
struct NativeTu15VectorNormalizeStats {std::uint64_t calls{},normalized{},fallbacks{};};
// Actual122B60. Native x87 computes the source-associated extended length;
// native SSE publishes the vector. The return is actual XMM0's scalar DATA.
// Its five temporary x87 operands admit preserved incoming depths0..3.
std::uint32_t native_tu15_normalize_vector(SourcePhysicsContactMemory&,
    std::uint32_t vector,std::uint32_t fallback,NativeTu15PhysicsFrameFp&,
    unsigned actual_preserved_x87_depth,const SourcePhysicsContactRecordObserver& = {},
    NativeTu15VectorNormalizeStats* = nullptr,bool* normalized_path = nullptr);
} // namespace halo2
