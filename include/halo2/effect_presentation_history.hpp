#pragma once
#include "halo2/effect_presentation_continuity.hpp"
#include "halo2/multiplayer_runtime_receipt.hpp"
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
    EffectPresentationHistoryState history;
    std::optional<std::uint64_t> world_tick,muzzle_tick;
    std::optional<ValidatedAuthorityCut> bound;
    // Runtime frontier and effect owners were inserted/bound as one entry.
    // This observes only that boundary, not full presentation/audio readiness.
    bool runtime_frontier_retained{},runtime_frontier_bound{};
    std::size_t pending_sounds{},pending_weapon_sounds{},pending_effect_sounds{};
};
// Completed CPU presentation and exact accepted receipts on the game executor.
// Capacity is a native retention policy, not an original Xbox constant or a
// byte-budget claim. One new capture may be staged transiently. Caller-owned
// checkpoints returned by bind() have their own lifetime outside this history.
class EffectPresentationHistory {
public:
    EffectPresentationHistory(EffectPresentationContinuity &,std::size_t capacity);
    EffectPresentationHistory(EffectPresentationContinuity &,NativeEffectSounds &,std::size_t capacity);
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
    // Capture both owners before one insertion/eviction. Rejection leaves the
    // completed history untouched; duplicate receipts never recapture values.
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
        const MultiplayerState &current()const;
        const MultiplayerState &previous()const;
        const MultiplayerRuntimeReceiptFrontier &frontier()const;
    private:
        friend class EffectPresentationHistory;
        struct Data;std::shared_ptr<const Data> data_;
    };
    std::optional<RuntimeCheckpoint> bind_runtime(const SourceAuthorityHostProof &,const LanClient &)const;
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
    std::optional<MultiplayerSnapshotOrigin> runtime_current_,runtime_previous_;
    const std::thread::id executor_=std::this_thread::get_id();
    EffectPresentationHistoryState state_;
    std::deque<Entry> entries_;
};
}
