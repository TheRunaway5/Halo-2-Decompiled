#pragma once
#include "halo2/model.hpp"
#include "halo2/transform.hpp"
namespace halo2 {
struct AnimationFrameWindow {float previous{},current{},fraction=1;};
// Blend completed LOCAL bone poses, then compose the hierarchy once. This
// also handles a controller wrapping from its final frame to its loop frame.
std::vector<Vertex> pose_model_world_between_frames(const RenderModel &,const Transform &,
    const AnimationClip &,AnimationFrameWindow);
std::vector<Transform> pose_model_nodes_world_between_frames(const RenderModel &,const Transform &,
    const AnimationClip &,AnimationFrameWindow);
}
