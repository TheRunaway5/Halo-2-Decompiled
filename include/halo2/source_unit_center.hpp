#pragma once
#include "halo2/cache.hpp"
#include <functional>

namespace halo2 {
struct SourceUnitHorizontalBasis { Vec2 forward, left; };
struct SourceUnitHeadOffset {
    std::uint8_t initialized{}; // Actual object+3C8, retained if already set.
    Vec3 local;                // Actual object+3CC.
};
struct SourceUnitEyeSeat {
    std::uint32_t marker{}, camera_marker{}; // Definition seat+8/+60.
};
struct SourceUnitEyeDefinition {
    float standing_height{}, crouching_height{}; // +218/+21C.
    float pitch_minimum{}, pitch_maximum{};       // +224/+228.
    Vec3 correction_scale;                       // +22C/+230/+234.
    float minimum_forward{};                     // +238.
    std::span<const SourceUnitEyeSeat> seats;
};
struct SourceUnitHeadOffsetChannel {
    std::uint32_t datum=0xffffffffu;
    std::int16_t index=-1;
    std::uint8_t state{}, flags{};
};
struct SourceUnitEyeState {
    std::uint32_t object=0xffffffffu, parent=0xffffffffu;
    std::uint32_t related=0xffffffffu; // +24C: owner's seat selects a marker.
    std::uint32_t player=0xffffffffu; // +13C: DE190 requires a player owner.
    std::uint8_t type{}, flags10a{}, state3dc{}, flag3f8{};
    std::uint16_t flags348{};
    std::int16_t seat=-1;
    float scale=1.f, crouch{};
    Vec3 forward, up, node_zero_position; // Actual posed world node0+28.
    SourceUnitHeadOffset head;
    // Actual object+signed(+33E), channels+9C/+BC, fields+0/+6/+11/+12.
    std::array<SourceUnitHeadOffsetChannel,2> offset_channels;
    SourceUnitEyeDefinition definition;
};
struct SourceUnitEyeMarker {
    std::uint16_t count{};
    Vec3 position; // B8D30 result WORLD translation, including count-zero result.
};
struct SourceUnitEyeContact {
    bool hit{};
    float fraction{};
    Vec3 point; // Actual1697C0 result point, also required on a miss.
};
struct SourceUnitEyeServices {
    std::function<const SourceUnitEyeState &(std::uint32_t)> object;
    std::function<Vec3(std::uint32_t)> root; // B9DD0, parent transforms resolved.
    std::function<Vec3(std::uint32_t)> attachment_position; // Complete B9EF0.
    // B8D30 with maximum1, false. Never substitute a root for a missing marker.
    std::function<SourceUnitEyeMarker(std::uint32_t,std::uint32_t)> marker;
    // Complete1697C0 flags04808C2D, both excluded owners NONE. The source
    // caller initializes packet+24's signed short to NONE before the first call.
    std::function<SourceUnitEyeContact(Vec3,Vec3)> contact;
    std::function<Vec3(std::uint32_t,Vec3)> clamp_aim; // CBA50, looking=false.
};
// Retail DE080 and11D090; finite operands, nearest non-FTZ arithmetic.
SourceUnitHorizontalBasis source_unit_horizontal_basis(Vec3 forward,Vec3 up);
Vec3 source_unit_perpendicular_up(Vec3 forward);
bool source_unit_head_offset_suppressed(std::span<const SourceUnitHeadOffsetChannel,2>); //10F820.
// Complete DE190 scalar scheduling/marker fallback/smoothing, with10F820.
// Marker posing and aim clamp must be supplied by their real owners.
void source_update_unit_head_offset(SourceUnitEyeState &,const SourceUnitEyeServices &);
// DF380. Curve is the actual1024-byte curve5 at46D148 in the base disc XBE.
// The ignored A74C0 calls only evaluate an orthonormal-pair predicate.
Vec3 source_unit_observer_position(const SourceUnitEyeState &,Vec3 position,
    Vec3 forward,Vec3 up,std::span<const std::uint8_t,1024> curve);
// Complete CAFC0 scalar branches, including ordinary eye height, observer
// correction, both world contacts, related-seat marker and parented camera.
// The external services above are explicit; this API is not a camera adapter.
Vec3 source_unit_center(const SourceUnitEyeState &,const SourceUnitEyeServices &,
    std::span<const std::uint8_t,1024> curve);
}
