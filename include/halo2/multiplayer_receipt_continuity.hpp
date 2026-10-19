#pragma once
#include "halo2/effect_presentation_history.hpp"

namespace halo2 {
// The mutable values actually consumed by the windowed receipt/effect caller.
// Prepare copies the exact privately authenticated history entry; publication
// composes its CPU effects, deferred sounds and runtime frontiers. This is a
// subtransaction: scene/HUD/input/playing audio/native activation remain with
// the enclosing complete adopter. All borrowed owners outlive this object.
class MultiplayerReceiptContinuity {
    struct PreparedData;
public:
    struct Targets {
        MultiplayerRuntimeReceiptValues &values;
        MultiplayerState &current,&previous;
        MultiplayerSnapshotOrigin &current_origin;
    };
    MultiplayerReceiptContinuity(Targets,EffectPresentationContinuity &,NativeEffectSounds &,MultiplayerPoseContinuity &);
    MultiplayerReceiptContinuity(const MultiplayerReceiptContinuity &)=delete;
    MultiplayerReceiptContinuity &operator=(const MultiplayerReceiptContinuity &)=delete;
    class Prepared {
    public:
        ~Prepared();
        Prepared(Prepared &&)noexcept;
        Prepared &operator=(Prepared &&)noexcept;
    private:
        friend class MultiplayerReceiptContinuity;
        Prepared();std::unique_ptr<PreparedData> data_;
    };
    Prepared prepare(const EffectPresentationHistory::RuntimeCheckpoint &,
        ParticleCollisionQuery muzzle_collision,ParticleCollisionQuery world_collision)const;
    bool ready(const Prepared &,const ValidatedAuthorityCut &)const noexcept;
    // Swaps only, on the creating executor after the complete enclosing
    // preflight. The consumed token retains replaced states/queues/effects;
    // destroy it only after leaving the native/session publication hook.
    bool commit(Prepared &&,const ValidatedAuthorityCut &)noexcept;
private:
    Targets targets_;
    EffectPresentationContinuity &effects_;
    NativeEffectSounds &sounds_;
    MultiplayerPoseContinuity &poses_;
    const std::thread::id executor_=std::this_thread::get_id();
    const std::shared_ptr<const int> owner_=std::make_shared<const int>(0);
};
}
