#pragma once
#include "halo2/model.hpp"
#include "halo2/transform.hpp"

namespace halo2 {
// Original1421F0 consumes raw local quaternion/translation/scale directly.
// No quaternion normalization is performed by the cold16D940 caller.
float source_model_local_squared_norm(const BonePose &);
// Binary32 rounded reciprocal is the explicit software-oracle convention.
// This does not assert the physical Pentium III RCPSS approximation seed.
Transform source_model_local_transform(const BonePose &);
// The literal RCPSS result can instead be supplied as an observed input.
// The original Newton step and all subsequent SSE operations remain intact.
Transform source_model_local_transform(const BonePose &, float reciprocal_seed);

// Original BD090 node-order142A60 hierarchy. Parents must precede children;
// NONE(-1) roots remain direct locals unless an actual external root is given.
// Transform locals allow callers to retain explicitly observed RCPSS seeds.
std::vector<Transform> source_model_world_nodes(
    std::span<const Transform> locals, std::span<const int> parents,
    const Transform *root = nullptr);
std::vector<Transform> source_model_world_nodes(
    std::span<const BonePose> locals, std::span<const int> parents,
    const Transform *root = nullptr);
} // namespace halo2
