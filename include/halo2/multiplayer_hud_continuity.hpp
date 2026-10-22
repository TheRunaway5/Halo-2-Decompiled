#pragma once
#include "halo2/multiplayer_snapshot_origin.hpp"
#include <array>
#include <map>
#include <memory>
#include <optional>
#include <thread>

namespace halo2 {
class MultiplayerHud;
class MultiplayerScoreboard;

// Exact CPU values of the complete, fixed resident HUD registry at one actual
// accepted receipt. Materials/radar images, GPU work, device bindings, cameras
// and audio remain separate owners in the complete enclosing transaction.
class MultiplayerHudContinuity {
    struct Impl;
public:
    using DefinitionRegistry=std::map<std::pair<unsigned,std::uint32_t>,std::unique_ptr<MultiplayerHud>>;
    using UnitRegistry=std::map<unsigned,std::unique_ptr<MultiplayerHud>>;
    struct Targets {
        const DefinitionRegistry &primary;
        const DefinitionRegistry &offhand;
        const DefinitionRegistry &vehicle;
        const UnitRegistry &unit;
        const DefinitionRegistry &parent;
        const std::unique_ptr<MultiplayerScoreboard> &scoreboard;
    };
    struct Statistics {
        // Primary, offhand, vehicle, unit, parent; includes dormant owners.
        std::array<std::size_t,5> huds{};
        bool scoreboard{};
    };
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
    // Construct only after all resident HUD/font/material slots are prepared.
    // The referenced maps/unique_ptr variable and Mesh outlive this owner.
    explicit MultiplayerHudContinuity(Targets);
    ~MultiplayerHudContinuity();
    MultiplayerHudContinuity(const MultiplayerHudContinuity &)=delete;
    MultiplayerHudContinuity &operator=(const MultiplayerHudContinuity &)=delete;
    MultiplayerHudContinuity(MultiplayerHudContinuity &&)=delete;
    MultiplayerHudContinuity &operator=(MultiplayerHudContinuity &&)=delete;
    // Announce the completed-frame writer batch before changing child values.
    // Children independently guard mutations; all aliases must stay quiescent
    // through preparation, complete preflight and publication.
    void mutating();
    class Snapshot {
    public:
        bool valid()const noexcept{return data_&&!data_->owner.expired();}
        bool matches(const MultiplayerSnapshotOrigin &origin)const noexcept{return valid()&&data_->origin.same(origin);}
        std::optional<Statistics> statistics()const noexcept{return valid()?std::optional{data_->statistics}:std::nullopt;}
    private:
        friend class MultiplayerHudContinuity;std::shared_ptr<const SnapshotData> data_;
    };
    Snapshot capture(const MultiplayerSnapshotOrigin &)const;
    bool owns_snapshot(const Snapshot &)const noexcept;
    class PreparedRestore {
    public:
        ~PreparedRestore();PreparedRestore(PreparedRestore &&)noexcept;
        PreparedRestore &operator=(PreparedRestore &&)noexcept;
    private:
        friend class MultiplayerHudContinuity;PreparedRestore();std::unique_ptr<PreparedData> data_;
    };
    PreparedRestore prepare_restore(const Snapshot &,const MultiplayerSnapshotProof &)const;
    bool ready(const PreparedRestore &,const MultiplayerSnapshotProof &)const noexcept;
    // Full census and all-child preflight precede the first no-throw swap.
    // Keep the consumed token until displaced child values may retire outside
    // the enclosing publication. No runtime adoption/readiness is enabled here.
    bool commit(PreparedRestore &)noexcept;
private:
    bool ready(const PreparedRestore &)const noexcept;
    std::unique_ptr<Impl> impl_;
};
}
