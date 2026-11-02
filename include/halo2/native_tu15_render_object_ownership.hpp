#pragma once
#include "halo2/native_tu15_physics_frame.hpp"

namespace halo2 {
// Complete TU1.5 BCFB0: follow the real object-parent chain, preserving the
// root's full salted identity. NONE returns without touching the object bank.
// Corrupt cycles are rejected after the65536 possible object indices; the
// original loops forever in that case. Valid trees retain original behavior.
std::uint32_t native_tu15_render_root_owner(const SourcePhysicsContactMemory&,
    std::uint32_t object);
// Complete BAC70: signed16 attachment index into the object's definition.
// The source resolves the definition even when the index is negative.
std::uint32_t native_tu15_render_attachment_marker(const SourcePhysicsContactMemory&,
    std::uint32_t object, std::int16_t attachment);
// Complete158290: classify the current authored camera-control callback and
// blend value. The ordered SSE comparison preserves DAZ and NaN exceptions.
std::uint32_t native_tu15_render_view_category(const SourcePhysicsContactMemory&,
    std::uint32_t view, NativeTu15PhysicsFrameFp&);
// Complete3E9B0, including158290 and BCFB0. All admission reads retain their
// original lazy order; this does not construct an object or grant authority.
bool native_tu15_render_owner_has_view_priority(const SourcePhysicsContactMemory&,
    std::uint32_t object, NativeTu15PhysicsFrameFp&);
}
