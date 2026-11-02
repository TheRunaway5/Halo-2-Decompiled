#pragma once
#include "halo2/native_tu15_contact_producer.hpp"
namespace halo2 {
// Actual30D3F0: signed triangle products, source normal orientation and paired
// vertex/weight swaps. Inputs and outputs are current simplex/point DATA; no
// CPU register file, caller control frame or supplied result is accepted.
std::uint32_t native_tu15_orient_simplex_triangle(SourcePhysicsContactMemory&,
    std::uint32_t simplex,std::uint32_t point,std::uint32_t vertices,
    std::uint32_t orient_when_inside,NativeTu15PhysicsFrameFp&,
    unsigned incoming_x87_depth,const SourcePhysicsContactRecordObserver& = {});
} // namespace halo2
