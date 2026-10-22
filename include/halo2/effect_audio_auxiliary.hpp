#pragma once
#include "halo2/peer_authority_transfer.hpp"
#include <atomic>
#include <memory>
#include <thread>
#include <utility>

namespace halo2 {
class EffectPresentationContinuity;
// Agreement with the actual Effects writer's prepared auxiliary word. This
// is not full Effects readiness or a native/device/cut-association proof.
// The aggregate keeps Effects, its borrowed words and its PreparedRestore
// alive, and performs the complete four-owner preflight before publication.
class EffectAudioAuxiliary {
public:
    EffectAudioAuxiliary()=default;
    // Prepared: compares the saved target, not the still-live old RNG value.
    // Committed: additionally requires that exact current publication and
    // its actual auxiliary word. Creator-only; no allocation, lock, callback
    // or native/device operation. False for cancelled/stale/dead owners.
    bool ready(const std::uint32_t *binding,std::uint32_t saved_value)const noexcept;
    // Only the real four swaps followed by the Effects RNG write can satisfy
    // this witness. Audio checks it before its first playback-state swap.
    bool committed(const std::uint32_t *binding,std::uint32_t saved_value)const noexcept;
    // Immutable metadata only; callers must still establish exact receipt /
    // native-cut association. Reading the descriptor does not prove readiness.
    const ValidatedAuthorityCut &cut()const;
private:
    enum class Phase {Prepared,Committing,Committed,Cancelled};
    struct Target;
    struct Owner {
        explicit Owner(std::thread::id t):executor(t){}
        const std::thread::id executor;
        std::atomic_bool alive{true};
        std::uint64_t generation{}; // Only the actual Effects executor writes.
        std::shared_ptr<Target> current;
    };
    struct Target {
        std::weak_ptr<Owner> owner;
        const std::thread::id executor;
        const std::uint32_t *const binding;
        const std::uint32_t saved_value;
        const ValidatedAuthorityCut cut;
        const std::uint64_t generation;
        std::atomic<Phase> phase{Phase::Prepared};
        Target(const std::shared_ptr<Owner>&o,const std::uint32_t *b,
            std::uint32_t value,const ValidatedAuthorityCut&c)
            :owner(o),executor(o->executor),binding(b),saved_value(value),
             cut(c),generation(o->generation){}
    };
    std::shared_ptr<Target> target_;
    explicit EffectAudioAuxiliary(std::shared_ptr<Target> target):target_(std::move(target)){}
    bool witness(const std::uint32_t*,std::uint32_t,bool committed_only)const noexcept;
    friend class EffectPresentationContinuity;
};
}
