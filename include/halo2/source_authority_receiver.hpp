#pragma once
#include "halo2/multiplayer_world.hpp"
#include "halo2/online_match_session.hpp"
#include "halo2/source_replay_batch.hpp"
#include "halo2/source_match_authority.hpp"
#include <future>

namespace halo2 {
struct SourceAuthorityReceiverState {
    bool decoding{},warmed{},workspace_warmed{};
    std::uint64_t received{},prepared{},decode_microseconds{},warm_microseconds{},prepare_microseconds{};
    std::uint64_t workspace_constructions{},workspace_microseconds{};
    bool delta{};
    std::optional<ValidatedAuthorityCut> ready;
    std::string error;
    std::uint64_t replay_batches{},replayed_phases{},replay_sequence{},replay_microseconds{};
    bool replaying{};
};
// One client executor's private complete-world candidate. The worker decodes
// owning bytes only; native construction/preparation stays on this executor.
// Sole consumer of the session's received_authority() queue. A prepared world
// survives host connection loss; neither metadata nor preparation elects it.
// Every public call is restricted to the constructing executor thread.
class SourceAuthorityReceiver {
public:
    explicit SourceAuthorityReceiver(const MultiplayerOptions&,
        std::shared_ptr<PhysicsLiveClockSource> candidate_clock={},
        std::shared_ptr<PhysicsLiveClockSource> workspace_clock={});
    SourceAuthorityReceiver(const MultiplayerOptions&,SourceAuthorityStreamMode,
        std::shared_ptr<PhysicsLiveClockSource> candidate_clock={},
        std::shared_ptr<PhysicsLiveClockSource> workspace_clock={});
    ~SourceAuthorityReceiver();
    SourceAuthorityReceiver(const SourceAuthorityReceiver &)=delete;
    SourceAuthorityReceiver &operator=(const SourceAuthorityReceiver &)=delete;
    // Construct at most one of the candidate/workspace executors per call at
    // an externally quiescent boundary. True means the candidate exists; inspect
    // workspace_warmed for the pair. Requires independent identities; no receipt.
    bool warm();
    void update();
    SourceAuthorityReceiverState state()const;
    // Read-only, executor-local inspection. Expires on cancellation, context
    // change, replacement or take_world(); never retain borrowers across update.
    const MultiplayerPreparedWorld *world()const;
    SourceMatchAuthority::Checkpoint checkpoint(const ValidatedAuthorityCut &expected)const;
    // Transfer this exact validated world to an external continuation owner.
    // This does not promote a host or rebind peers. The application must first
    // certify its chosen cut/generation, then perform role/hardware rebinding.
    std::unique_ptr<MultiplayerPreparedWorld> take_world(const ValidatedAuthorityCut &expected);
    // Replay candidates retain the producer's views until a separately
    // certified handoff. This stages the recipient view/profile mapping on a
    // private executor and transfers only the exact validated cut requested.
    std::unique_ptr<MultiplayerPreparedWorld> take_world_for_local_views(const ValidatedAuthorityCut &expected);
    // Permanently stop this receiver and discard queued encoder output/cache.
    // Does not revoke an authority generation or stop peer transport/voice.
    void cancel();
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
// Background replay has its own native executor from construction through
// destruction. Only owning status/checkpoint values cross this boundary;
// native worlds, borrowed views and hardware never change executor threads.
class SourceAuthorityReplayWorker {
public:
    explicit SourceAuthorityReplayWorker(const MultiplayerOptions&,
        std::shared_ptr<PhysicsLiveClockSource> candidate_clock={},
        std::shared_ptr<PhysicsLiveClockSource> workspace_clock={});
    ~SourceAuthorityReplayWorker();
    SourceAuthorityReplayWorker(const SourceAuthorityReplayWorker &)=delete;
    SourceAuthorityReplayWorker &operator=(const SourceAuthorityReplayWorker &)=delete;
    SourceAuthorityReceiverState state()const;
    // At most one outstanding export. Rejects a stale/unvalidated cut; this
    // does not freeze, promote or transfer the native executor. A certified
    // continuation must import this value into its own quiescent executor.
    std::future<SourceMatchAuthority::Checkpoint> checkpoint(ValidatedAuthorityCut);
    // Nonblocking permanent cancellation. Destruction joins after any current
    // original native call returns and destroys the world on its own thread.
    void cancel();
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
