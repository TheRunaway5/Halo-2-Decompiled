#pragma once
#include "halo2/effect_presentation_continuity.hpp"
#include "halo2/multiplayer_runtime_receipt.hpp"
#include "halo2/peer_snapshot_candidate.hpp"
#include <deque>

namespace halo2 {
class SourceAuthorityHostProof;
class LanClient;
struct EffectPresentationHistoryState {
    std::size_t capacity{},retained{};
    std::uint64_t captured{},evicted{};
    std::optional<AuthorityTransferContext> context;
    std::optional<std::uint64_t> newest_tick;
    std::optional<std::uint32_t> newest_sequence;
};
struct EffectPresentationObservation {
    struct SourceResources {
        // Passive startup admission only. These values neither advance native
        // producers nor advertise complete presentation/migration readiness.
        bool migration_candidate{},particle_producer{},model_producer{};
        std::size_t particle_materials{},first_person_systems{},world_systems{},model_light_sections{};
        std::size_t materials_at_upload{},materials_now{};
        bool operator==(const SourceResources &)const=default;
    };
    EffectPresentationHistoryState history;
    std::optional<std::uint64_t> world_tick,muzzle_tick;
    std::optional<ValidatedAuthorityCut> bound;
    // Runtime frontier and effect owners were inserted/bound as one entry.
    // This observes only that boundary, not full presentation/audio readiness.
    bool runtime_frontier_retained{},runtime_frontier_bound{};
    std::size_t pending_sounds{},pending_weapon_sounds{},pending_effect_sounds{};
    SourceResources source_resources;
    // Actual scene graph capture in the same owning history entry. This is
    // receipt/storage evidence, never complete adopter or election readiness.
    bool scene_frontier_retained{},scene_frontier_bound{};
    std::optional<MultiplayerSceneContinuity::Statistics> scene_statistics;
    // CPU registry history only. Generated radar pixels/materials, GPU upload
    // and complete runtime adoption remain separately qualified owners.
    bool hud_frontier_retained{},hud_frontier_bound{};
    std::optional<MultiplayerHudContinuity::Statistics> hud_statistics;
    // Actual CPU gait group only; no physical locomotion, input/audio or full
    // readiness claim. The unavailable mask observes current runtime flags.
    bool locomotion_frontier_retained{},locomotion_frontier_bound{};
    std::optional<MultiplayerLocomotionContinuity::Statistics> locomotion_statistics;
    std::uint16_t current_locomotion_unavailable{};
};
// Completed CPU presentation and exact accepted receipts on the game executor.
// Capacity is a native retention policy, not an original Xbox constant or a
// byte-budget claim. One new capture may be staged transiently. Caller-owned
// checkpoints returned by bind() have their own lifetime outside this history.
class EffectPresentationHistory {
public:
    EffectPresentationHistory(EffectPresentationContinuity &,std::size_t capacity);
    EffectPresentationHistory(EffectPresentationContinuity &,NativeEffectSounds &,std::size_t capacity);
    EffectPresentationHistory(EffectPresentationContinuity &,NativeEffectSounds &,const Simulation &,std::size_t capacity);
    EffectPresentationHistory(const EffectPresentationHistory &)=delete;
    EffectPresentationHistory &operator=(const EffectPresentationHistory &)=delete;
    // Admit from independent session state, never from a receipt. Retire old-
    // generation entries even when the next capture cannot complete.
    void context(std::optional<AuthorityTransferContext>);
    // Optional initial current==previous baseline, obtained directly from the
    // currently adopted client receipt before consuming a newer batch. Later
    // predecessors are maintained internally across typed capture attempts.
    void begin_runtime(const LanClient &);
    // Returns false for the same owning receipt already retained; no recapture
    // or eviction occurs. Unadmitted/retired/reordered receipts and incomplete
    // cuts throw. Eviction never permits recapturing an old receipt later.
    bool remember(const MultiplayerSnapshotOrigin &);
    // Capture both effect owners and retain the caller's completed replica
    // before one insertion/eviction. Requires the Simulation-bound constructor;
    // missing/foreign/different-receipt replicas fail before receipt tracking.
    // Rejection leaves completed entries untouched; duplicates never recapture.
    bool remember(const MultiplayerSnapshotOrigin &,const MultiplayerRuntimeReceiptFrontier &);
    // Only independently natively validated paired evidence can bind a normal
    // receipt. Missing/evicted/wrong-connection snapshots return no checkpoint;
    // a matching tick alone never admits a different sequence/body.
    std::optional<EffectPresentationContinuity::Checkpoint> bind(
        const SourceAuthorityHostProof &,const LanClient &)const;
    class RuntimeCheckpoint {
    public:
        const ValidatedAuthorityCut &cut()const;
        const EffectPresentationContinuity::Checkpoint &effects()const;
        const MultiplayerSnapshotOrigin &origin()const;
        const MultiplayerSnapshotProof &proof()const;
        const MultiplayerState &current()const;
        const MultiplayerState &previous()const;
        const MultiplayerRuntimeReceiptFrontier &frontier()const;
    private:
        friend class EffectPresentationHistory;
        struct Data;std::shared_ptr<const Data> data_;
    };
    std::optional<RuntimeCheckpoint> bind_runtime(const SourceAuthorityHostProof &,const LanClient &)const;
    // An early offer may precede native replay by more than the ordinary
    // history window. Keep its exact receipt candidates, never a tick-only
    // substitute. At most two owning reservations and capacity entries per
    // reservation are admitted; overflow is explicit and cannot grow storage.
    class RuntimeReservation {
    public:
        bool valid()const;
        bool overflowed()const;
        std::size_t retained()const;
    private:
        friend class EffectPresentationHistory;
        struct Data;std::shared_ptr<Data> data_;
    };
    RuntimeReservation reserve_runtime(const AuthorityTransferContext &,const AuthorityCutSeal &);
    std::optional<RuntimeCheckpoint> bind_runtime(const SourceAuthorityHostProof &,const LanClient &,
        const RuntimeReservation &)const;
    // One provisional foreground owner, armed before the host chooses its
    // future snapshot/native seal. Only the client's privately marked receipt
    // can fill it. This is not a native checkpoint or migration readiness.
    class RuntimeCandidate {
    public:
        bool valid()const;
        bool captured()const;
        std::size_t retained()const;
        const PeerSnapshotToken &token()const;
    private:
        friend class EffectPresentationHistory;
        struct Data;std::shared_ptr<Data> data_;
    };
    // Call only after the complete current accepted batch has been projected.
    // The actual tail must already own a full typed history entry. All local
    // allocation precedes the client's adoption fence/Armed acknowledgement.
    RuntimeCandidate arm_runtime(LanClient &,std::shared_ptr<PeerSnapshotCandidate>,const PeerSnapshotToken &);
    // Failed closing-state preflight keeps the entry/delivery untouched. Once
    // preflight succeeds, release the actual provisional entry before emitting
    // Retired. A later refusal keeps the delivery/fence owned and the token
    // ineligible for binding, so explicit retirement can be retried safely.
    bool retire_runtime_candidate(const RuntimeCandidate &);
    std::optional<RuntimeCheckpoint> bind_runtime(const SourceAuthorityHostProof &,const LanClient &,
        const RuntimeCandidate &)const;
    EffectPresentationHistoryState state()const;
private:
    void own()const;
    bool remember_entry(const MultiplayerSnapshotOrigin &,const MultiplayerRuntimeReceiptFrontier *);
    struct Entry {
        MultiplayerSnapshotOrigin origin;
        EffectPresentationContinuity::Snapshot snapshot;
        std::shared_ptr<const MultiplayerRuntimeReceiptFrontier> runtime;
    };
    EffectPresentationContinuity &effects_;
    NativeEffectSounds *sounds_{};
    const Simulation *replica_{};
    std::optional<MultiplayerSnapshotOrigin> runtime_current_,runtime_previous_;
    const std::thread::id executor_=std::this_thread::get_id();
    EffectPresentationHistoryState state_;
    std::deque<Entry> entries_;
    const std::shared_ptr<const void> reservation_owner_=std::make_shared<unsigned char>(0);
    std::vector<std::weak_ptr<RuntimeReservation::Data>> reservations_;
    std::shared_ptr<RuntimeCandidate::Data> candidate_;
};
}
