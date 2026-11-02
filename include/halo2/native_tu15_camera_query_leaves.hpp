#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
namespace halo2 {
// Actual16B940/16E360 pure flag mappings; complete32-bit results.
std::uint32_t native_tu15_camera_bsp_query_flags(std::uint32_t flags);
std::uint32_t native_tu15_camera_object_query_flags(std::uint32_t flags);
// ActualBA970/BA9E0. Iterator and payload identities are caller-owned DATA.
// Payload remains untouched when the source iterator is already empty.
std::uint32_t native_tu15_begin_spatial_object_query(SourcePhysicsContactMemory&,
    std::uint16_t cluster,std::uint32_t iterator,std::uint32_t payload,
    const SourcePhysicsContactRecordObserver& = {});
std::uint32_t native_tu15_next_spatial_object_query(SourcePhysicsContactMemory&,
    std::uint32_t iterator,std::uint32_t payload,
    const SourcePhysicsContactRecordObserver& = {});
// Actual120FA0. The caller observes AL only. Radius is an ordinary float-bit
// value; all three point/direction vector words are source DATA with lazy
// direction reads. Native SSE exceptions/control are preserved exactly.
bool native_tu15_segment_near_sphere(SourcePhysicsContactMemory&,
    std::uint32_t first_point,std::uint32_t center,std::uint32_t direction,
    std::uint32_t radius_bits,NativeTu15PhysicsFrameFp&);
} // namespace halo2
