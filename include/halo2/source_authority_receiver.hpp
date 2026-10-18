#pragma once
#include "halo2/multiplayer_world.hpp"
#include "halo2/online_match_session.hpp"
#include "halo2/source_replay_batch.hpp"
#include "halo2/source_match_authority.hpp"
#include "halo2/multiplayer_network.hpp"
#include <future>

namespace halo2 {
class SourcePeerHostTakeover;
struct SourceAuthorityReceiverState {
    bool decoding{},warmed{},workspace_warmed{};
    std::uint64_t received{},prepared{},decode_microseconds{},warm_microseconds{},prepare_microseconds{};
    std::uint64_t workspace_constructions{},workspace_microseconds{};
    bool delta{};
    std::optional<ValidatedAuthorityCut> ready;
    std::string error;
    std::uint64_t replay_batches{},replayed_phases{},replay_sequence{},replay_microseconds{};
    bool replaying{},replay_paused{},host_ready{};
};
// Owning recovery input only. The source executor validates this exact cut and
// its independent roster before export. It is not an election certificate or
// permission to replace the accepted host/membership. No native view, callback,
// device or executor pointer crosses this boundary.
struct SourceAuthorityRetainedWorld {
    ValidatedAuthorityCut cut;
    AcceptedSourcePlayers admissions;
    std::vector<unsigned> producer_views,local_views;
    SourceMatchAuthority::Checkpoint checkpoint;
    // Present only after paired host/world validation at this exact cut.
    std::optional<LanHost::Continuation> host;
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
    // Admit/decode bounded pending cuts without executing a native replay
    // phase. Full-cut construction/preparation may still update this private
    // candidate. This neither freezes the live host nor changes authority.
    void prefetch();
    SourceAuthorityReceiverState state()const;
    // Read-only, executor-local inspection. Expires on cancellation, context
    // change, replacement or take_world(); never retain borrowers across update.
    const MultiplayerPreparedWorld *world()const;
    SourceMatchAuthority::Checkpoint checkpoint(const ValidatedAuthorityCut &expected)const;
    // Permanently stop taking new transfers. Already decoding/replaying input
    // drains through its normal complete validation before export. Null means
    // that work is still active; a quiescent missing/invalid world throws.
    // This must precede receiver destruction on a recoverable host loss.
    std::optional<SourceAuthorityRetainedWorld> retain_validated_world();
    // Transfer this exact validated world to an external continuation owner.
    // This world-only API drops any host sidecar; use retention for both owners.
    // This does not promote a host or rebind peers. The application must first
    // certify its chosen cut/generation, then perform role/hardware rebinding.
    std::unique_ptr<MultiplayerPreparedWorld> take_world(const ValidatedAuthorityCut &expected);
    // Replay candidates retain the producer's views until a separately
    // certified handoff. This stages the recipient view/profile mapping on a
    // private executor and transfers only the exact validated cut requested.
    std::unique_ptr<MultiplayerPreparedWorld> take_world_for_local_views(const ValidatedAuthorityCut &expected);
    // Consume the paired candidate only after this session independently
    // learns the exact replacement decision. Rebinds native couch views,
    // connection/input state and cameras on this executor; media stays fenced
    // until the returned owner is explicitly activated. Requires Replay mode.
    std::unique_ptr<SourcePeerHostTakeover> prepare_peer_host_takeover(const PeerMigrationDecision &);
    // Permanently stop this receiver and discard queued encoder output/cache.
    // Does not revoke an authority generation or stop peer transport/voice.
    void cancel();
private:
    void update(bool advance_replay);
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
    // Applies between native phases. state().replay_paused acknowledges the
    // applied scheduling mode; no native executor/world borrower is exported.
    // Receipt/decoding continue. A paused retention drains after resumption.
    bool pause_replay(bool);
    // At most one outstanding export. Rejects a stale/unvalidated cut; this
    // does not freeze, promote or transfer the native executor. A certified
    // continuation must import this value into its own quiescent executor.
    std::future<SourceMatchAuthority::Checkpoint> checkpoint(ValidatedAuthorityCut);
    // One-shot, mutually exclusive with checkpoint export. Stops new receives,
    // drains the authenticated in-flight batches and returns their final validated
    // world plus independent admissions for a later certified takeover. The
    // owning result survives worker destruction. No host is elected here.
    std::future<SourceAuthorityRetainedWorld> retain_validated_world();
    // Nonblocking permanent cancellation. Destruction joins after any current
    // original native call returns and destroys the world on its own thread.
    void cancel();
    // True only after the native receiver and both worlds have been destroyed
    // on the worker executor. Poll after cancel() before foreground retirement.
    bool stopped()const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
