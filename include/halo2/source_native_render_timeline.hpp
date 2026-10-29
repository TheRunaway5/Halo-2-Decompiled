#pragma once
#include "halo2/multiplayer_snapshot_origin.hpp"
#include "halo2/render_interpolation.hpp"
#include <memory>

namespace halo2 {
class AcceptedShadowCut;
class LanClient;
class MultiplayerMatch;
class MultiplayerObjectRuntime;
class Simulation;
class SourceShadowPresentationOwner;
struct MultiplayerState;

// Numeric policy only. Passing this predicate never grants a native borrow or
// constructs the private sparse-pair permit; the timeline validates receipts.
struct SourceNativeRenderContinuity {
    bool same_generation{},same_world{};
    std::uint64_t previous_serial{},current_serial{},previous_tick{},current_tick{};
    std::uint32_t previous_sequence{},current_sequence{};
    double previous_seconds{},current_seconds{};
};
bool source_native_render_continuous(const SourceNativeRenderContinuity&)noexcept;
bool source_native_render_fresh(std::uint64_t native_tick,std::uint64_t ordinary_tick,
    std::uint64_t actual_front_tick)noexcept;
enum class SourceNativeRenderSuspension {
    none,missing_cut,foreground_clock,connection_or_ack,native_owner,ordinary_body,
    native_ahead,native_stale,local_identity
};
const char* source_native_render_suspension_name(SourceNativeRenderSuspension)noexcept;

// One synchronous renderer's immutable two-cut presentation. The two body
// receipts/interpolations stay alive when the timeline captures its successor.
// Native world/Match pointers remain a foreground borrow from the presentation
// owner: do not adopt/reset/destroy that owner until this draw has completed.
class SourceNativeRenderFrameContext {
public:
    const MultiplayerState& previous()const;
    const MultiplayerState& current()const;
    const MultiplayerSnapshotOrigin& previous_origin()const;
    const MultiplayerSnapshotOrigin& origin()const;
    std::uint64_t serial()const;
    std::uint64_t generation()const;
    std::uint64_t world_epoch()const;
    const std::optional<AuthorityTransferContext>& authority_context()const;
    float blend()const noexcept{return blend_;}
    MultiplayerObjectRuntime* world()const noexcept{return world_;}
    const MultiplayerMatch& match()const;
    const Simulation& simulation()const;
    const SourceWorldInterpolation& world_interpolation()const;
    const SceneInterpolation& scene_interpolation()const;
    std::vector<Transform> sample_nodes(std::uint32_t,const SourceWorldInterpolation::ModelProvider&)const;
private:
    friend class SourceNativeRenderTimeline;
    struct Data;
    std::shared_ptr<const Data> data_;
    MultiplayerObjectRuntime *world_{};
    const MultiplayerMatch *match_{};
    float blend_=1;
    SourceNativeRenderFrameContext(std::shared_ptr<const Data>,MultiplayerObjectRuntime*,const MultiplayerMatch*,float);
};

class SourceNativeRenderTimeline {
public:
    // Must follow actual native adoption AND successful client lane ACK.
    // Duplicate capture is idempotent and never restarts its render clock.
    void capture(const LanClient&,const SourceShadowPresentationOwner&,const AcceptedShadowCut&,double seconds);
    std::optional<SourceNativeRenderFrameContext> frame(const LanClient&,const SourceShadowPresentationOwner&,
        const MultiplayerSnapshotOrigin& latest_ordinary,const MultiplayerState& latest_state,double seconds,SourceNativeRenderSuspension* = nullptr)const;
    void reset()noexcept;
private:
    std::shared_ptr<const SourceNativeRenderFrameContext::Data> current_;
};
}
