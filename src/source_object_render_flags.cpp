#include "halo2/source_object_render_flags.hpp"

namespace halo2 {
SourceObjectRenderFlag31EventResult source_object_render_flag31_construction_event(
    std::uint32_t current_body_flags,
    const std::optional<ObjectLightContactDefinition> &resolved_definition,
    SourceObjectRenderAttributionOutputs caller_outputs) {
    if(!resolved_definition)throw FormatError("source BF9A0 object definition lookup unresolved");
    const auto &definition=*resolved_definition;
    const bool available=definition.model_attribution_available();
    // BF9A0 writes both out-parameters after the reference guards, including
    // the nonpositive signed-count path. Reuse its existing authored predicate.
    if(definition.model!=0xffffffffu && definition.render_model!=0xffffffffu &&
       definition.collision!=0xffffffffu)
        caller_outputs={definition.render_model,definition.collision};
    // B7B40 consumes AL alone and replaces precisely object+4 bit31.
    return {(current_body_flags&0x7fffffffu)|(available?0x80000000u:0u),available,caller_outputs};
}
} // namespace halo2
