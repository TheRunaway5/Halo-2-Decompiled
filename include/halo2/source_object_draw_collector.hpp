#pragma once
#include "halo2/source_draw_unit.hpp"
#include "halo2/source_sphere_rectangle.hpp"

namespace halo2 {
struct SourceDrawResourceWorld;
// Actual4B5D0 output consumed by133F70. The retained raw layout includes all
// five model/context rows, selected section bytes and region words. This does
// not manufacture that upstream object's cache, nodes, LOD or registrations.
struct SourceObjectDrawCollector {
    std::array<std::byte,0x1e4> bytes{};
    std::uint32_t address{};
};
struct SourceObjectDrawOwner {
    std::uint8_t kind{}; // Actual object body+AA.
    std::uint32_t definition{}; // Actual object body+0.
};
struct SourceObjectDrawServices {
    SourceDrawUnitServices draw;
    std::function<SourceObjectDrawOwner(std::uint32_t)> owner;
    std::function<bool(std::uint32_t)> weapon_section_policy; // Definition+290 !=0.
};
struct SourceObjectDrawContext {
    SourceDrawUnitContext draw;
    SourceViewProjectionBank rendering;
    Vec3 eye;
    std::array<std::int16_t,4> viewport{};
    // Actual temporary collector identity when the weapon policy consumes a
    // copy. It is separate from the persistent record; only its center token
    // is forwarded to2CBF0. Callers own this address/token namespace.
    std::uint32_t temporary_address{};
};
// Resolve original direct object/body/weapon fields and the shared draw
// resources from current owned banks. The caller still supplies mutable
// render contexts and upstream4B5D0 collector production.
SourceObjectDrawServices source_object_draw_resource_services(const SourceDrawResourceWorld&);
// Complete133F70, including low-byte owner filtering, the conditional weapon
// collector copy, original model/section order and every2CBF0 request. Calls
// the shared native allocator and native30290. Missing consumed owner/resource
// banks reject. Does not reset frame state or supply4B5D0/133AB0 production.
void source_object_collect_draw_units(SourceDrawUnitState&,
    const SourceObjectDrawCollector&,std::uint32_t filter_owner,
    const SourceObjectDrawContext&,const SourceObjectDrawServices&);
}
