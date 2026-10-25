#pragma once
#include "halo2/bsp_atmosphere.hpp"
#include "halo2/source_bsp_admission.hpp"
#include "halo2/source_bsp_ranges.hpp"
#include "halo2/source_shader_lod.hpp"
#include "halo2/source_draw_unit.hpp"

namespace halo2 {
// Explicit current44940/449E0 and2D000 services. Resident means the actual
//12DE70 resource request succeeded and geometry+50 is nonnull; an authored
//resource count alone does not establish residency.
struct SourceBspDrawGeometry {
    std::uint16_t geometry_class{},mask_subparts{}; // Metadata+14/+24.
    bool resident{};
};
struct SourceBspDrawInstance {
    std::uint16_t lighting_policy{}; // Literal instance+56, zero/nonzero.
    bool lightmap_resident{}; // Actual2D000→12DE70 result when consumed.
    std::uint32_t transform_address{}; // Actual instance+0 bank identity.
};
struct SourceBspDrawWorld {
    const SourceBspAdmissionBsp &bsp;
    std::span<const SourceBspDrawGeometry> clusters,definitions;
    std::span<const SourceBspDrawInstance> instances;
    bool lightmap_matches{}; // Actual2D000 presence, bitmap and checksum gates.
    // Optional live449D0 admission for an appended instance unit: with its
    // request selector, the actual130980 request, then the published+50
    // geometry. Absent, the definition's prepared residency applies.
    std::function<bool(unsigned definition,bool request)> definition_resident;
};
using SourceBspDrawState=SourceDrawUnitState;
struct SourceBspDrawContext {
    std::uint32_t mode{},mask_arena_address{};
    std::int8_t override_byte{}; // Actual context+2A60.
    BspFogSample fog;
    SourceShaderCoverageCamera camera;
};
// Exact BSP-owned2CB30→133390→133520→2CBF0 corridor. The full retained
//850-unit bank and caller-owned arena survive. Main mode resets the two
//prefix counters; secondary mode starts at the real main prefix. Dynamic
//133720 and its global owner tables remain a separate subsequent service.
// Zero-alpha133520 writes the original queue skipped bit before returning.
void source_bsp_collect_draw_units(SourceBspDrawState&,SourceBspAdmission&,
    const SourceBspDrawWorld&,const SourceBspDrawContext&);
}
