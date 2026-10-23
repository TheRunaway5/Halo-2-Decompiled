#pragma once
#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_snapshot_origin.hpp"
#include "halo2/first_person_ik.hpp"
#include "halo2/model_node_interpolation.hpp"
#include "halo2/native_player_animation.hpp"
#include "halo2/render_interpolation.hpp"
#include "halo2/unit_scope_camera.hpp"
#include <map>
#include <thread>

namespace halo2 {
// Actual local pose/channel histories at a completed accepted-receipt barrier.
// Includes the local scope producer/observer polynomial and zoom lifetimes.
// Native locomotion executors, resident resources, scene adapter caches, HUD,
// native vehicle/death cameras, physical input and playing audio remain independently owned. This
// restores existing completed values; it neither advances a channel nor
// reconstructs a cold pose from a decoded state or advertises full readiness.
class MultiplayerPoseContinuity {
public:
    using FpPoseKey=std::array<std::uint32_t,3>;
    struct Values {
        SceneInterpolation scene_interpolation;
        SourceWorldInterpolation source_body_interpolation;
        std::map<std::pair<std::uint32_t,std::uint32_t>,ModelNodeInterpolation> source_render_interpolation;
        std::array<FirstPersonViewBinding,4> client_fp_views,client_fp_origins;
        std::array<bool,4> client_fp_clock_available{},client_secondary_clock_available{};
        std::array<std::uint32_t,4> client_fp_reported_units{null_datum,null_datum,null_datum,null_datum};
        std::array<std::map<FpPoseKey,ModelNodeInterpolation>,4> source_fp_interpolation;
        std::array<std::array<std::uint32_t,5>,4> source_fp_lifetimes{};
        std::array<bool,4> source_observer_ready{};
        std::array<FirstPersonAimBlend,multiplayer_capacity> aim_blends,body_aim_blends;
        std::array<bool,multiplayer_capacity> aim_alive{};
        std::array<NativePlayerTransition,multiplayer_capacity> body_transitions;
        // Actual preceding-render camera channels at this accepted receipt.
        // Keep derivatives, remaining time and all polynomial coefficients;
        // rebuilding only a target FOV changes the continuation curve.
        std::array<UnitScopeCameraProducer,4> scope_producers{};
        std::array<UnitScopeCameraState,4> scope_observers{};
        std::array<std::int16_t,4> view_zoom{-1,-1,-1,-1};
        std::array<std::uint32_t,4> view_zoom_unit{null_datum,null_datum,null_datum,null_datum};
        std::array<std::uint32_t,4> view_zoom_reset{};
        std::array<bool,4> view_was_dead{};
    };
    MultiplayerPoseContinuity()=default;
    MultiplayerPoseContinuity(const MultiplayerPoseContinuity &)=delete;
    MultiplayerPoseContinuity &operator=(const MultiplayerPoseContinuity &)=delete;
    // Inline Values and field swaps preserve references to the actual field
    // objects. A caller retaining mutable aliases must call mutating() before
    // every receipt/render mutation batch and keep them quiescent throughout
    // preparation/readiness/commit. state() is itself a mutation admission.
    Values &state();
    const Values &state()const;
    void mutating();
    class Snapshot {
    public:
        bool valid()const noexcept;
        bool matches(const MultiplayerSnapshotOrigin &)const noexcept;
    private:
        friend class MultiplayerPoseContinuity;
        struct Data;std::shared_ptr<const Data> data_;
    };
    Snapshot capture(const MultiplayerSnapshotOrigin &)const;
    // Stable ownership does not expire when a later receipt or frame mutates
    // this owner. Historical cuts remain restorable on the same executor.
    bool owns_snapshot(const Snapshot &)const noexcept;
    class PreparedRestore {
    public:
        ~PreparedRestore();
        PreparedRestore(PreparedRestore &&)noexcept;
        PreparedRestore &operator=(PreparedRestore &&)noexcept;
    private:
        friend class MultiplayerPoseContinuity;
        PreparedRestore();struct Data;std::unique_ptr<Data> data_;
    };
    // The native proof binds the exact accepted connection/sequence/body and
    // complete cut. All copying/allocation occurs here, before publication.
    PreparedRestore prepare_restore(const Snapshot &,const MultiplayerSnapshotProof &)const;
    bool ready(const PreparedRestore &,const MultiplayerSnapshotProof &)const noexcept;
    // Caller retains the consumed token and its retired containers until the
    // enclosing publication/lock has ended. No allocation, callback, channel
    // evaluation, GPU/device operation or container retirement occurs here.
    bool commit(PreparedRestore &)noexcept;
private:
    void own()const;
    bool ready(const PreparedRestore &)const noexcept;
    Values values_;
    const std::thread::id executor_=std::this_thread::get_id();
    const std::shared_ptr<const void> owner_=std::make_shared<const unsigned char>(0);
    std::uint64_t revision_{};
};
}
