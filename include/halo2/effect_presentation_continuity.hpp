#pragma once
#include "halo2/source_map_effects.hpp"
#include "halo2/source_combat_attachments.hpp"
#include "halo2/peer_authority_transfer.hpp"
#include "halo2/multiplayer_snapshot_origin.hpp"
#include "halo2/effect_audio_auxiliary.hpp"
#include <thread>

namespace halo2 {
// Executor-local presentation portion of an accepted native cut. The caller
// authenticates/validates the native cut and owns the completed-frame barrier.
// This groups existing presentation state, not a network checkpoint or election.
class EffectPresentationContinuity {
    struct PreparedData;
    struct SnapshotData;
public:
    EffectPresentationContinuity(NativeMuzzleEffects &,NativeWorldEffects &,
        SourceMapEffects &,SourceCombatAttachments &,SourceEffectRandom);
    ~EffectPresentationContinuity();
    EffectPresentationContinuity(const EffectPresentationContinuity &)=delete;
    EffectPresentationContinuity &operator=(const EffectPresentationContinuity &)=delete;
    class Snapshot {
    public:
        std::optional<std::uint64_t> tick()const noexcept;
    private:
        friend class EffectPresentationContinuity;
        std::shared_ptr<const SnapshotData> data_;
    };
    class Checkpoint {
    public:
        const ValidatedAuthorityCut &cut()const;
    private:
        friend class EffectPresentationContinuity;
        struct Data;std::shared_ptr<const Data> data_;
    };
    class PreparedRestore {
    public:
        ~PreparedRestore();
        PreparedRestore(PreparedRestore &&)noexcept;
        PreparedRestore &operator=(PreparedRestore &&)noexcept;
    private:
        friend class EffectPresentationContinuity;
        PreparedRestore();std::unique_ptr<PreparedData> data_;
    };
    // Capture before asynchronous native validation finishes. All active
    // owners share this completed tick. A never-synchronized cache is cold.
    Snapshot capture(const AuthorityTransferContext &)const;
    // Normal peer presentation uses the receipt of the state just consumed.
    // This snapshot can be bound only through the matching native host proof.
    Snapshot capture(const MultiplayerSnapshotOrigin &)const;
    // Binding does not recapture the now-later world. Context/tick agreement
    // is necessary, not proof of native phase equality: the caller establishes
    // that association independently before supplying the validated seal.
    Checkpoint bind(const Snapshot &,const ValidatedAuthorityCut &)const;
    Checkpoint bind(const Snapshot &,const MultiplayerSnapshotProof &)const;
    // Synchronous convenience over capture/bind; same caller proof required.
    Checkpoint checkpoint(const ValidatedAuthorityCut &)const;
    PreparedRestore prepare_restore(const Checkpoint &,const ValidatedAuthorityCut &expected,
        ParticleCollisionQuery muzzle_collision,ParticleCollisionQuery world_collision)const;
    bool restore_ready(const PreparedRestore &,const ValidatedAuthorityCut &expected)const noexcept;
    // Minted only after the actual four-owner preflight for this exact cut.
    // The agreement retains no Effects owner or replacement storage: the
    // enclosing aggregate keeps this object, its words and wrapper alive.
    EffectAudioAuxiliary audio_auxiliary(const PreparedRestore &,
        const ValidatedAuthorityCut &expected)const;
    // After every enclosing GPU/device/native preparation has succeeded.
    // No allocation or callback occurs between the four swaps and RNG restore.
    // All referenced owners/words outlive this object and remain quiescent.
    bool commit_restore(PreparedRestore &&,const ValidatedAuthorityCut &expected)noexcept;
private:
    void own()const;
    Snapshot capture_context(const AuthorityTransferContext &,std::optional<MultiplayerSnapshotOrigin>)const;
    Checkpoint bind_cut(const Snapshot &,const ValidatedAuthorityCut &)const;
    NativeMuzzleEffects &muzzle_;NativeWorldEffects &world_;
    SourceMapEffects &map_;SourceCombatAttachments &combat_;
    SourceEffectRandom random_;
    const std::thread::id executor_=std::this_thread::get_id();
    std::shared_ptr<const int> owner_=std::make_shared<const int>(0);
    std::shared_ptr<EffectAudioAuxiliary::Owner> audio_auxiliary_owner_;
};
}
