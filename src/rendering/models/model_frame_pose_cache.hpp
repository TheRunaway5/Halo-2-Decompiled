#pragma once
#include "rendering/models/model_world_pose_cache.hpp"
#include <map>

namespace halo2 {
// One completed render frame owns this bank. Sections and compatible views
// reuse a full skin; each draw still selects its own sections and shaders.
class ModelFramePoseCache {
    std::map<std::pair<std::uint32_t,unsigned>,ModelWorldPoseCache> poses_;
    unsigned builds_{},reuses_{};
public:
    std::span<const Vertex> update(std::uint32_t owner,unsigned lod,
        const RenderModel &model,std::span<const Transform> nodes) {
        const auto result=poses_[{owner,lod}].update(model,nodes);
        if(result.reused)++reuses_;else ++builds_;
        return result.vertices;
    }
    unsigned builds() const noexcept{return builds_;}
    unsigned reuses() const noexcept{return reuses_;}
};
}
