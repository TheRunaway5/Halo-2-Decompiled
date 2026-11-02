#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
namespace halo2 {
// ActualAA200, AA270 and B1720. Inputs and boolean returns are semantic DATA;
// native x87/SSE preserve the original finite/tolerance checks and FP effects.
bool native_tu15_vector_near_unit(SourcePhysicsContactMemory&,std::uint32_t vector,
    NativeTu15PhysicsFrameFp&,unsigned actual_preserved_x87_depth);
bool native_tu15_scalar_near(SourcePhysicsContactMemory&,std::uint32_t first_bits,
    std::uint32_t second_bits,NativeTu15PhysicsFrameFp&,unsigned actual_preserved_x87_depth);
bool native_tu15_vectors_orthonormal(SourcePhysicsContactMemory&,
    std::uint32_t first,std::uint32_t second,NativeTu15PhysicsFrameFp&,
    unsigned actual_preserved_x87_depth);
}
