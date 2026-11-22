#pragma once

#include <cstddef>
#include <cstdint>

namespace halo2::native_light_view {
struct RenderView {
    std::uint32_t visibility_bank;
    std::uint32_t location;
    std::int16_t cluster;
    std::byte before_auxiliary[10950];
    std::uint32_t auxiliary;
};
static_assert(offsetof(RenderView, cluster) == 8 && offsetof(RenderView, auxiliary) == 10960);

enum class VisibilityKind : std::uint32_t { portal_traversal, context_reuse, projection };

template<class Frame> void collect(Frame& frame) {
    frame.begin();
    frame.setup_view();
    frame.reset_queues();
    switch(frame.visibility_kind()) {
    case VisibilityKind::portal_traversal:
        frame.traverse_portals(offsetof(RenderView, visibility_bank), offsetof(RenderView, cluster));
        if(const auto auxiliary = frame.auxiliary(offsetof(RenderView, auxiliary))) {
            frame.merge_auxiliary(auxiliary, offsetof(RenderView, visibility_bank));
        }
        break;
    case VisibilityKind::context_reuse:
        frame.reuse_context();
        break;
    case VisibilityKind::projection:
        frame.project_visibility(offsetof(RenderView, visibility_bank));
        break;
    default:
        break;
    }
    frame.begin_bsp();
    frame.admit_bsp();
    frame.begin_cluster_contents();
    frame.collect_cluster_contents();
    frame.finish();
}
} // namespace halo2::native_light_view
