#pragma once
#include "halo2/native_tu15_contact_producer.hpp"
namespace halo2 {
// Only source-written numerical vectors/scalars and a current vertex cursor.
// No saved register, return address or CPU control metadata is accepted.
struct NativeTu15SimplexFaceSelectionScratch {std::uint32_t identity{},refinement{};};
// Actual30D1A0: reject overlapping face candidates and select a positive face.
std::uint32_t native_tu15_refine_simplex_face(SourcePhysicsContactMemory&,
    std::uint32_t vertices,std::uint32_t point,std::uint32_t scores,
    std::uint32_t scalar_data,NativeTu15PhysicsFrameFp&,
    unsigned incoming_x87_depth,const SourcePhysicsContactRecordObserver& = {});
// Actual30D5D0: classify the three faces adjacent to the fourth simplex point.
// Original EAX/EBX are typed vertices/point inputs; only the semantic index is
// returned. The ambiguous classification composes native30D1A0 directly.
std::uint32_t native_tu15_select_simplex_face(SourcePhysicsContactMemory&,
    std::uint32_t vertices,std::uint32_t point,std::uint32_t scale_bits,
    NativeTu15SimplexFaceSelectionScratch,NativeTu15PhysicsFrameFp&,
    unsigned incoming_x87_depth,const SourcePhysicsContactRecordObserver& = {});
} // namespace halo2
