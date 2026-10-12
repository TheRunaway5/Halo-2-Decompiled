#pragma once
#include "halo2/source_bsp_admission.hpp"
#include "halo2/source_model_fade.hpp"
#include <optional>

namespace halo2 {
// Actual lookup fields read by134240. Missing optionals mean unresolved input;
// a false presence value is the literal null pointer checked by that routine.
struct SourceObjectRenderQueueOwner {
    std::uint32_t identity;
    std::optional<bool> body_present;
    std::optional<std::uint32_t> definition; // body+0
};
struct SourceObjectRenderQueueDefinition {
    std::uint32_t identity;
    std::optional<bool> present;
    std::optional<std::uint32_t> model; // object definition+38, hlmt datum
};
struct SourceObjectRenderQueueModel {
    std::uint32_t identity;
    std::optional<bool> present;
    std::optional<std::uint32_t> render_model; // hlmt+4, mode datum
};
struct SourceObjectRenderQueueGeometry {
    std::uint32_t identity;
    std::optional<std::int32_t> prt_info_count; // mode+74, signed count
};
//133720→134240 promotes current models with positive PRTInfo count by swapping
// the complete queue row with the next prefix slot (163D50). The displaced
// non-PRT suffix is NOT stable. Parent/root model selection is not performed.
// Inputs must be valid actual source lookup banks; a missing mode pointer is
// invalid source state, whereas owner/definition/hlmt null pointers are tested.
// This consumes an already admitted queue, without creating registrations,
// visibility, cache records or geometry. No native activation is implied.
void source_object_render_queue_order(std::span<SourceBspAdmissionRecord>,
    std::span<const SourceObjectRenderQueueOwner>,
    std::span<const SourceObjectRenderQueueDefinition>,
    std::span<const SourceObjectRenderQueueModel>,
    std::span<const SourceObjectRenderQueueGeometry>);
//133790 adds lowbit0 when complete4BAF0's FIRST output byte is zero. Existing
// bits are retained, including an existing lowbit0 for a nonzero fade. Callers
// must compute the fade from actual133790 metric/4BA019/current/root/model
// inputs; the shadow-candidate fade separately consumes the SECOND output.
std::uint16_t source_object_render_queue_fade_flags(std::uint16_t,
    SourceModelFadeBytes);
}
