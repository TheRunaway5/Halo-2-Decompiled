#pragma once
#include "halo2/source_shader_lod.hpp"
#include "halo2/math.hpp"

namespace halo2 {
// Actual render-view globals. Shader detail and geometry detail are independent.
// The caller supplies the source camera/outer rectangle producers; no native
// viewport or projected-model-bounds estimate is admitted here.
struct SourceModelShaderView {
    SourceShaderCoverageCamera camera;
    SourceShaderLodView shader;
    float object_coverage_multiplier{}; // 2C490 -> 4BA028
    float part_shader_multiplier{}; // 2C490 -> 4670E4
    bool view_modifier{}; // 2F970 -> 4BA019
};
struct SourceModelShaderCollector {
    float coverage{}; // 133790 whole collector+1B4
    std::uint8_t coverage_byte{}; // 133F70 -> 2CBF0 -> parallel4C0B84
};
// Use the actual collector OWNER's retained object+30/+3C sphere. 133AB0
// copies this coverage to attached child collectors; it does not replace it
// with each rendered child's sphere. First-person uses this same producer.
SourceModelShaderCollector source_model_shader_collector(
    Vec3 cached_owner_center,float cached_owner_radius,const SourceModelShaderView &);
struct SourceModelShaderPart {
    float metric{}; // 41040 byte * resolved45CE0 scale * 4670E4 * 4
    SourceShaderLodChoice choice;
};
// 41040 -> 1BF50 and 40E30 -> 1BF50 select the same shader bank for ordinary
// model and first-person paths. The first-person geometry cap4 is separate.
// Shader overrides, actual part admission and lighting implementation remain
// explicit caller services; an empty bank's literal -1/true is not a draw.
SourceModelShaderPart source_model_shader_part(const SourceModelShaderCollector &,
    float resolved_part_scale,const SourceShaderLodDefinition &,const SourceModelShaderView &);
}
