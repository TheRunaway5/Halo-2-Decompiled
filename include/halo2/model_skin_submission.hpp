#pragma once
#include "halo2/model.hpp"

namespace halo2 {
// Same1376C0 WORLD/inverse-bind/compound bank as skin_model_world_nodes,
// without executing the final per-vertex deformation kernel.
std::vector<std::array<float,12>> model_skin_world_palette(const RenderModel&,
                                                        std::span<const Transform>);
// Prepared at the original final CPU-kernel boundary, after identical palette
// arithmetic and vertex copying. Unsupported numeric/host/diagnostic cases run
// the original CPU kernel using that same palette, without rebuilding it.
struct ModelSkinWorldPose {
    std::vector<Vertex> vertices;
    std::vector<std::array<float,12>> palette;
    bool gpu{};
};
struct ModelSkinWorldPoseView {
    std::span<const Vertex> vertices;
    std::span<const std::array<float,12>> palette;
    bool gpu{};
};
ModelSkinWorldPose model_skin_world_pose(const RenderModel&,std::span<const Transform>,bool allow_gpu);
// GPU WORLD deformation is the default presentation route. The original CPU
// kernel stays selectable with HALO2_REFERENCE_CPU_SKINNING or
// HALO2_ENABLE_GPU_SKINNING=0; per-model admission still applies either way.
bool model_skin_gpu_requested()noexcept;
// One complete source owner/LOD. Expanded ordinals survive section repeats
// and shader rejection; bindings and vertices have the same ordered extent.
// Palette arithmetic and source callbacks remain on their CPU owners.
struct ModelSkinGeometry {
    std::uint32_t owner{},definition{},render_model{};
    unsigned lod{};
    std::vector<Vertex> vertices;
    std::vector<SkinBinding> bindings;
    std::vector<std::uint32_t> source_vertices;
    std::vector<std::array<float,12>> palette;
    // First-person append's original identity transform runs after skinning.
    // It still changes signed zeros and must not run on the local input bank.
    bool post_identity_placement{};
    // Set only by a producer whose records are unmodified local vertex and
    // binding copies of a complete bank that passed arithmetic admission with
    // this exact palette; per-record admission then holds for every subset.
    bool arithmetic_admitted{};
};
// First names the final local-space records in ModelView::vertices. The
// renderer resolves every record through its actual ordered grouping runs;
// immutable source ordinals are not inferred by matching float positions.
struct ModelSkinSubmission {
    std::size_t first{};
    std::shared_ptr<const ModelSkinGeometry> geometry;
};
}
