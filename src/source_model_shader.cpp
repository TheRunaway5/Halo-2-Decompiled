#include "halo2/source_model_shader.hpp"

namespace halo2 {
SourceModelShaderCollector source_model_shader_collector(
    Vec3 center,float radius,const SourceModelShaderView &view) {
    const auto coverage=source_shader_object_coverage(center,radius,
        view.object_coverage_multiplier,view.camera);
    return {coverage,source_shader_coverage_byte(coverage)};
}
SourceModelShaderPart source_model_shader_part(const SourceModelShaderCollector &collector,
    float scale,const SourceShaderLodDefinition &definition,const SourceModelShaderView &view) {
    const auto metric=source_shader_part_metric(collector.coverage_byte,scale,
        view.part_shader_multiplier);
    return {metric,source_shader_lod_choice(definition.thresholds,definition.bias,
        metric,view.shader,view.view_modifier)};
}
}
