#pragma once
#include "halo2/native_tu15_physics_frame.hpp"

namespace halo2 {
// Native point geometry shared by the campaign navigation owners. Addresses
// identify retained data; none identifies an executable policy or CPU frame.
// SIMD arithmetic owns MXCSR only and leaves the caller's live x87 untouched.
void native_campaign_ai_permute_point_tu15(SourcePhysicsContactMemory &,
    std::uint32_t source,std::uint32_t destination,std::int16_t axis,
    std::uint8_t variant,const SourcePhysicsContactRecordObserver & = {});
void native_campaign_ai_transform_point_tu15(SourcePhysicsContactMemory &,
    std::uint32_t matrix,std::uint32_t source,std::uint32_t destination,
    NativeTu15PhysicsFrameFp &,const SourcePhysicsContactRecordObserver & = {});
// A missing object/node copies the source XYZ and returns false. Frame NONE
// copies XYZ and succeeds. Neither outcome is a pathability decision.
bool native_campaign_ai_resolve_frame_tu15(SourcePhysicsContactMemory &,
    std::uint16_t frame,std::uint32_t source,std::uint32_t destination,
    NativeTu15PhysicsFrameFp &,const SourcePhysicsContactRecordObserver & = {});
void native_campaign_ai_project_point_tu15(SourcePhysicsContactMemory &,
    std::uint32_t framed_point,std::uint32_t destination,NativeTu15PhysicsFrameFp &,
    const SourcePhysicsContactRecordObserver & = {});
// Private intermediate projection for native algorithms. Retained input reads
// are identical; coordinates stay typed host values instead of guest scratch.
std::array<std::uint32_t,3> native_campaign_ai_project_point_value_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t framed_point,NativeTu15PhysicsFrameFp &);
// Returns the exact REAL bits. Equal frame IDs use their local coordinates;
// differing IDs project both points before their distinct source association.
std::uint32_t native_campaign_ai_point_distance_squared_tu15(
    SourcePhysicsContactMemory &,std::uint32_t first,std::uint32_t second,
    NativeTu15PhysicsFrameFp &);
}
