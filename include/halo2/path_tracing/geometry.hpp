#pragma once
#include "halo2/model_skin_submission.hpp"

namespace halo2::path_tracing {
// Own a WORLD-space copy for CPU BVH construction. GPU-deferred ranges use
// their actual current palettes and the shared native skin/FP placement graph;
// the caller's source bank and immutable submissions remain unchanged.
std::vector<Vertex> posed_vertices(std::span<const Vertex>,
    std::span<const ModelSkinSubmission>);
}
