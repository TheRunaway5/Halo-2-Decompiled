#pragma once
#include "halo2/source_bsp_ranges.hpp"
#include "halo2/source_transparent_sort.hpp"
#include <functional>

namespace halo2 {
// Shared4BA138/4C0B84/4C0ED6/4C1228/4C152C state. These banks belong to the
// ordered frame, not to an owner, material or player.2CB30 leaves arena storage
// and its cursor alone; the query owner resets that separate allocation bank.
struct SourceDrawUnitState {
    std::array<SourceCullDrawUnit,850> units{};
    std::array<std::uint8_t,850> coverage{},override_slots{};
    std::array<std::uint32_t,192> overrides{};
    std::array<SourceTransparentSection,32> sections{};
    std::uint32_t main_prefix{},draw_count{},override_count{},section_highwater{};
    SourceBspRangeArena arena;
};
// Literal2CBF0 entry ABI. Stack words retain pointer/token identity and exact
// float bits. Entries0..20 are ESP+4..54 at entry, respectively:
// flags,unit+4,unit+8,part,context,association,alpha,transform,override,
// coverage,sort-section,section-kind,outer-rectangle,inner-rectangle,
// fade-selector,fade-A,fade-B,section-depth,region-byte,center-pointer,radius.
struct SourceDrawUnitInput {
    std::uint32_t geometry{},instance{},distance_byte{}; // EAX,ECX,EDX.
    std::array<std::uint32_t,21> stack{};
};
struct SourceDrawUnitGeometry {
    std::uint16_t classification{},subpart_count{}; // Metadata+14/+24.
};
struct SourceDrawUnitServices {
    // Actual44940,2D000 and449E0 results. Metadata presence cannot supply the
    // resource-success return. Consumers observe the current already-written
    // unit and source selector, in original call order.
    std::function<SourceDrawUnitGeometry(std::uint16_t,std::uint32_t,
        const SourceCullDrawUnit&)> geometry;
    std::function<std::uint32_t(std::uint32_t,std::uint32_t,std::uint32_t)> lighting;
    std::function<bool(std::uint16_t,std::uint32_t,bool,
        const SourceCullDrawUnit&)> resident;
    std::function<std::span<std::byte>(std::uint32_t)> context;
    std::function<Vec3(std::uint32_t)> center;
    //30290 uses source_sphere_rectangle with the actual render bank/eye/short
    // viewport. The fixed outer/inner source branches do not consume it.
    std::function<std::array<float,4>(std::uint32_t,float)> rectangle;
};
struct SourceDrawUnitContext {
    std::uint32_t mode{},mask_arena_address{};
    bool force_outer{},force_inner{}; // Actual509410/509411 bytes.
};
// Complete2CB30 reset/prefix contract; no draw/resource service is consumed.
void source_draw_units_begin(SourceDrawUnitState&,std::uint32_t mode);
//2CBF0 through its actual return, retaining all bank writes and consumed
// service order. Returns signedAX: NONE on either allocation or resource
// failure. Resource failure rolls back only draw_count; overrides/sections/
// context writes survive. Missing consumed services reject, never imply success.
std::int16_t source_draw_unit_append(SourceDrawUnitState&,const SourceDrawUnitInput&,
    const SourceDrawUnitContext&,const SourceDrawUnitServices&);
}
