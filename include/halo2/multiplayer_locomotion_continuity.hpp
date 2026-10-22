#pragma once
#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_snapshot_origin.hpp"
#include <array>
#include <memory>
#include <optional>
#include <thread>

namespace halo2 {
class NativeLocomotion;

// Actual native presentation adapters and their full salted slot metadata at
// one accepted receipt. A dormant/disconnected slot is retained exactly as it
// exists; this owner does not select a gait, advance a clock or emit an event.
// Physical locomotion, source authority and complete migration stay separate.
class MultiplayerLocomotionContinuity {
    struct Impl;
public:
    using Slots=std::array<std::unique_ptr<NativeLocomotion>,multiplayer_capacity>;
    using Flags=std::array<bool,multiplayer_capacity>;
    using Handles=std::array<std::uint32_t,multiplayer_capacity>;
    struct Targets {
        Slots &gait;
        Flags &dual;
        Handles &units;
        Handles &weapons;
        Flags &unavailable;
    };
    struct Statistics {std::size_t occupied{};};
private:
    struct SnapshotData {
        std::weak_ptr<const void> owner;
        std::thread::id executor;
        MultiplayerSnapshotOrigin origin;
        std::shared_ptr<const void> payload;
        Statistics statistics;
    };
    struct PreparedData;
public:
    // All five actual fixed array variables outlive this owner and every
    // prepared publication. Every occupied adapter must use an owning Cache.
    explicit MultiplayerLocomotionContinuity(Targets);
    ~MultiplayerLocomotionContinuity();
    MultiplayerLocomotionContinuity(const MultiplayerLocomotionContinuity &)=delete;
    MultiplayerLocomotionContinuity &operator=(const MultiplayerLocomotionContinuity &)=delete;
    MultiplayerLocomotionContinuity(MultiplayerLocomotionContinuity &&)=delete;
    MultiplayerLocomotionContinuity &operator=(MultiplayerLocomotionContinuity &&)=delete;
    // Announce each writer batch through retained aliases. Child operations
    // also have their own executor/busy/revision guards. All aliases remain
    // quiescent throughout capture, preparation, full preflight and commit.
    void mutating();
    class Snapshot {
    public:
        bool valid()const noexcept{return data_&&!data_->owner.expired();}
        bool matches(const MultiplayerSnapshotOrigin &origin)const noexcept{return valid()&&data_->origin.same(origin);}
        std::optional<Statistics> statistics()const noexcept{return valid()?std::optional{data_->statistics}:std::nullopt;}
    private:
        friend class MultiplayerLocomotionContinuity;std::shared_ptr<const SnapshotData> data_;
    };
    Snapshot capture(const MultiplayerSnapshotOrigin &)const;
    bool owns_snapshot(const Snapshot &)const noexcept;
    class PreparedRestore {
    public:
        ~PreparedRestore();PreparedRestore(PreparedRestore &&)noexcept;
        PreparedRestore &operator=(PreparedRestore &&)noexcept;
    private:
        friend class MultiplayerLocomotionContinuity;PreparedRestore();std::unique_ptr<PreparedData> data_;
    };
    PreparedRestore prepare_restore(const Snapshot &,const MultiplayerSnapshotProof &)const;
    bool ready(const PreparedRestore &,const MultiplayerSnapshotProof &)const noexcept;
    // Every slot and current metadata array is checked before the first
    // allocation-free, no-throw swap. Keep the consumed token until old frame
    // manager spans/clip pointers and retired metadata may safely retire.
    // This prerequisite does not publish a role, transport or full Ready.
    bool commit(PreparedRestore &)noexcept;
private:
    bool ready(const PreparedRestore &)const noexcept;
    std::unique_ptr<Impl> impl_;
};
}
