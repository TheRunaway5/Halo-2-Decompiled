#pragma once
#include "halo2/scene.hpp"
#include "halo2/transform.hpp"
#include "halo2/source_world_pose.hpp"
#include <span>
#include <functional>
#include <optional>
#include <utility>
#include <vector>

namespace halo2 {
class SourceNativeRenderTimeline;
// Only the adopted/ACKed native timeline can admit a sparse completed pair.
// Ordinary simulation histories retain their consecutive-tick requirement.
class SourceNativeCutPermit {
    friend class SourceNativeRenderTimeline;
    friend class SourceWorldInterpolation;
    friend class SceneInterpolation;
    SourceNativeCutPermit(std::uint64_t before,std::uint64_t after):before_(before),after_(after){}
    std::uint64_t before_,after_;
};
// Native presentation between completed game states. These helpers never
// advance a source controller, dispatch events or change simulation storage.
void validate_render_fraction(float);
Vec3 render_lerp(Vec3 previous,Vec3 current,float fraction);
float render_angle(float previous,float current,float fraction);
float render_loop(float previous,float current,float period,float fraction);
Transform render_transform(const Transform &previous,const Transform &current,float fraction);
Transform scene_object_transform(const SceneObjects::Object &);
// Non-host movement prediction (NETCODE_ENHANCED_GOAL item 1): one own
// living, unseated body drawn at its predicted origin. `previous` and
// `current` are its authoritative origins at the two completed endpoints
// held by the interpolation; `predicted` is the origin the predicted camera
// is drawn from. Presentation only.
struct SourceWorldPredictedRoot {
    std::uint32_t object{0xffffffffu};
    Vec3 previous,current,predicted;
};
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
    void capture_native_cut(const SourceNativeCutPermit&,std::span<const SourceWorldPose>,
        std::span<const std::uint32_t> discontinuities={});
    // A newer accepted body can replace this completed tick without a new
    // simulation step. Retain the earlier endpoint only for surviving owners.
    void refresh(std::uint64_t tick,std::span<const SourceWorldPose>,
        std::span<const std::uint32_t> discontinuities={});
    std::vector<Transform> sample(std::uint32_t object,float fraction)const;
    // Native model-order sampling. Known attachments retain their source
    // parent-node anchors and recompose into the shown parent's same-fraction
    // frame. An unavailable legacy binding keeps its completed WORLD bank.
    std::vector<Transform> sample(std::uint32_t object,const RenderModel&,float fraction,
        std::optional<Transform> shown_parent_frame={})const;
    using ModelProvider=std::function<const RenderModel*(std::uint32_t object)>;
    // Resolves visible parent hierarchies recursively. If the native parent
    // has no published bank, its retained exact frame remains the explicit
    // interpolation owner; no marker/default skeleton is inferred.
    std::vector<Transform> sample(std::uint32_t object,float fraction,const ModelProvider&)const;
    // Non-host dead reckoning: a rigid world motion composed onto the newest
    // completed nodes of each listed object (the fraction is then unused).
    // An empty list restores ordinary interpolation. Presentation only.
    void present(std::span<const std::pair<std::uint32_t,Transform>> motions);
    // Own predicted bodies: every sample of a listed body, and of every
    // attachment its current bank hangs from it, is the ordinary result
    // (animation pose kept) translated by `predicted` minus the authoritative
    // origin that same sample shows: blended at its fraction when the body's
    // endpoints pair there, else the current one. The body's origin is then
    // `predicted` at any fraction. Attachments sampled into a shown parent
    // frame follow that frame. Dead-reckoned objects keep their motion. An
    // empty list restores ordinary sampling exactly. Presentation only.
    void predict(std::span<const SourceWorldPredictedRoot> roots);
private:
    std::vector<Transform> sample_ordinary(std::uint32_t object,float fraction)const;
    std::vector<Transform> sample_ordinary(std::uint32_t object,const RenderModel&,float fraction,
        std::optional<Transform> shown_parent_frame)const;
    std::optional<Vec3> predicted_offset(std::uint32_t object,float fraction,bool model_order)const;
    std::uint64_t tick_{},previous_tick_{};
    bool initialized_{},paired_{},native_pair_{};
    std::vector<SourceWorldPose> previous_,current_;
    std::vector<std::pair<std::uint32_t,Transform>> presented_;
    std::vector<SourceWorldPredictedRoot> predicted_;
};
class SceneInterpolation {
public:
    // Capture EVERY completed step, including steps between submitted frames.
    // reset is required for a checkpoint restore, teleport or BSP replacement.
    void reset(std::uint64_t tick,std::span<const SceneObjects::Object>);
    void capture(std::uint64_t tick,std::span<const SceneObjects::Object>);
    void capture_native_cut(const SourceNativeCutPermit&,std::span<const SceneObjects::Object>);
    void refresh(std::uint64_t tick,std::span<const SceneObjects::Object>);
    Transform sample(const SceneObjects::Object &,float fraction)const;
    std::vector<Transform> sample_nodes(const SceneObjects::Object &,float fraction)const;
    // Same presentation-only motion for replicated scene roots, by handle.
    void present(std::span<const std::pair<std::uint32_t,Transform>> motions);
private:
    std::uint64_t tick_{};
    bool initialized_{},paired_{};
    std::vector<SceneObjects::Object> previous_,current_;
    std::vector<std::pair<std::uint32_t,Transform>> presented_;
};
}
