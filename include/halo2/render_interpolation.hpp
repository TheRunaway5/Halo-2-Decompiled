#pragma once
#include "halo2/scene.hpp"
#include "halo2/transform.hpp"
#include "halo2/source_world_pose.hpp"
#include <span>

namespace halo2 {
// Native presentation between completed game states. These helpers never
// advance a source controller, dispatch events or change simulation storage.
void validate_render_fraction(float);
Vec3 render_lerp(Vec3 previous,Vec3 current,float fraction);
float render_angle(float previous,float current,float fraction);
float render_loop(float previous,float current,float period,float fraction);
Transform render_transform(const Transform &previous,const Transform &current,float fraction);
Transform scene_object_transform(const SceneObjects::Object &);
// Retained WORLD poses for bodies outside SceneObjects, including detached
// dead bipeds. Full source salts identify lifetimes; no animation is advanced.
class SourceWorldInterpolation {
public:
    void reset(std::uint64_t tick,std::span<const SourceWorldPose>);
    void capture(std::uint64_t tick,std::span<const SourceWorldPose>);
    // Attachment and death transitions retain the salted object but replace
    // its pose owner. Their completed nodes must not blend with standing nodes.
    void capture(std::uint64_t tick,std::span<const SourceWorldPose>,
        std::span<const std::uint32_t> discontinuities);
    // A newer accepted body can replace this completed tick without a new
    // simulation step. Retain the earlier endpoint only for surviving owners.
    void refresh(std::uint64_t tick,std::span<const SourceWorldPose>,
        std::span<const std::uint32_t> discontinuities={});
    std::vector<Transform> sample(std::uint32_t object,float fraction)const;
private:
    std::uint64_t tick_{};
    bool initialized_{},paired_{};
    std::vector<SourceWorldPose> previous_,current_;
};
class SceneInterpolation {
public:
    // Capture EVERY completed step, including steps between submitted frames.
    // reset is required for a checkpoint restore, teleport or BSP replacement.
    void reset(std::uint64_t tick,std::span<const SceneObjects::Object>);
    void capture(std::uint64_t tick,std::span<const SceneObjects::Object>);
    void refresh(std::uint64_t tick,std::span<const SceneObjects::Object>);
    Transform sample(const SceneObjects::Object &,float fraction)const;
    std::vector<Transform> sample_nodes(const SceneObjects::Object &,float fraction)const;
private:
    std::uint64_t tick_{};
    bool initialized_{},paired_{};
    std::vector<SceneObjects::Object> previous_,current_;
};
}
