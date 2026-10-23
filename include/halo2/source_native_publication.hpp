#pragma once
#include "halo2/peer_authority_transfer.hpp"
#include "halo2/source_match_authority.hpp"
#include <memory>
#include <thread>

namespace halo2 {
class Cache;
class PhysicsLiveWorldTu15;
class MultiplayerPreparedWorld;
class SourcePeerHostTakeover;
class MccSoundVariationProvider;
class SourceForegroundWorldHandoff;
class SourceLensPresentationHandoffTu15;

// Current-target native exclusion minted only by an actual owning takeover.
// It is neither a replacement bank nor a session/decision/readiness proof.
// The root must separately quiesce Match, Simulation, renderer, raw spans and
// writers of the target's public ownership slots throughout this scope.
// Acquire before timing and SDL holds; release SDL, timing, then this scope.
// Retention prevents normal outer-wrapper/drop teardown. It does not make an
// escaped target.source_objects.reset() or Cache-slot replacement safe.
class SourceNativePublication {
    struct Owner;
    struct State;
public:
    // A weak witness of one actual publication scope's sealed source boundary.
    // It owns no target and proves neither a decision nor complete readiness.
    // Reacquiring a different scope never revives a released witness.
    class Boundary {
    public:
        Boundary()=default;
        bool current()const noexcept;
        bool same_scope(const Boundary &)const noexcept;
    private:
        explicit Boundary(const std::shared_ptr<State> &);
        bool matches_prepared(const MultiplayerPreparedWorld &,
            const std::shared_ptr<const Cache> &,const ValidatedAuthorityCut &,
            const SourceMatchAuthority::ExecutionBoundary &)const noexcept;
        std::weak_ptr<State> state_;
        friend class SourceNativePublication;
        friend class SourceForegroundWorldHandoff;
        friend class SourceLensPresentationHandoffTu15;
    };
    // Destruction and final retained-state retirement belong to the creator.
    // Foreign destruction terminates before touching gates or storage. Drop
    // this wrapper only outside SDL; future timing holders must release their
    // timing mutex before releasing their retained native state.
    ~SourceNativePublication();
    SourceNativePublication(const SourceNativePublication &)=delete;
    SourceNativePublication(SourceNativePublication &&)=delete;
    SourceNativePublication &operator=(const SourceNativePublication &)=delete;
    SourceNativePublication &operator=(SourceNativePublication &&)=delete;

    // Pure held witnesses: no lock, native-memory read, allocation, callback,
    // VM entry or timing operation. Cache identity includes its control block.
    bool current()const noexcept;
    bool matches(const PhysicsLiveWorldTu15 &,
        const std::shared_ptr<const Cache> &)const noexcept;
    Boundary boundary()const;
private:
    explicit SourceNativePublication(std::shared_ptr<State>);
    // Only the trusted private factory path supplies this real shared target,
    // formed by moving its unique owner before exposing mutable references.
    static std::shared_ptr<Owner> make_owner(std::shared_ptr<MultiplayerPreparedWorld>,
        const ValidatedAuthorityCut &);
    static SourceNativePublication acquire(const std::shared_ptr<Owner> &);
    static void close_owner(const std::shared_ptr<Owner> &)noexcept;
    static bool owner_available(const std::shared_ptr<Owner> &)noexcept;
    static bool retained_current(const std::shared_ptr<State> &)noexcept;
    static bool retained_matches(const std::shared_ptr<State> &,
        const PhysicsLiveWorldTu15 &,const std::shared_ptr<const Cache> &)noexcept;
    static void require_provider(const std::shared_ptr<State>&,
        const MccSoundVariationProvider&,const std::shared_ptr<const Cache>&);
    const std::thread::id executor_=std::this_thread::get_id();
    std::shared_ptr<State> state_;
    friend class SourcePeerHostTakeover;
    // Only a creator-thread held timing/device scope may retain State. A
    // transferable checkpoint/preparation may retain at most a weak State.
    friend class MccSoundVariationProvider;
};
}
