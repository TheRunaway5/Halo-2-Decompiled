#pragma once
#include "halo2/multiplayer_world.hpp"
#include "halo2/online_match_session.hpp"
#include "halo2/source_replay_batch.hpp"
#include "halo2/source_match_authority.hpp"
#include "halo2/multiplayer_network.hpp"
#include <future>

namespace halo2 {
class SourcePeerHostTakeover;
class SourceAuthorityEndpoint;
// Owning native replay/takeover recipe. Retains the admitted session and
// gameplay selections, without copying renderer/device callbacks, desktop,
// prepared worlds or other live presentation borrowers.
MultiplayerOptions source_authority_options(const MultiplayerOptions &);
// Immutable evidence of one independently admitted, natively validated paired
// world/host cut. Only the receiver creates this value. It owns the host table,
// not a native executor or world borrower, so it can outlive worker advancement
// and destruction. This proves historical snapshot association only: it does
// not assert current-world readiness, elect a host or authorize activation.
class SourceAuthorityHostProof {
public:
    const ValidatedAuthorityCut &cut()const;
    MultiplayerSnapshotProof prove_snapshot(const LanClient &,const MultiplayerSnapshotOrigin &)const;
private:
    friend class SourceAuthorityReceiver;
    SourceAuthorityHostProof(ValidatedAuthorityCut,LanHost::Continuation);
    struct Data;
    std::shared_ptr<const Data> data_;
};
struct SourceAuthorityReceiverState {
    bool decoding{},warmed{},workspace_warmed{};
    std::uint64_t received{},prepared{},decode_microseconds{},warm_microseconds{},prepare_microseconds{};
    std::uint64_t workspace_constructions{},workspace_microseconds{};
    bool delta{};
    std::optional<ValidatedAuthorityCut> ready;
    std::string error;
    std::uint64_t replay_batches{},replayed_phases{},replay_sequence{},replay_microseconds{};
    bool replaying{},replay_paused{},host_ready{};
    // Published atomically with ready/host_ready in worker state(). Absent
    // during unvalidated replay, cancellation and native-only streams. Copies
    // remain historical evidence; use the live checkpoint API for adoption.
    std::optional<SourceAuthorityHostProof> host_proof;
    // Exact delivered archive/replay cut that failed real decode/native
    // validation, never an unscoped old diagnostic or export-copy failure.
    // Historical until successful validation or this context retires; it
    // cannot establish readiness, shrink membership or authorize a takeover.
    std::optional<ValidatedAuthorityCut> rejected;
    // Optional terminal qualification only. This owning pair is captured by
    // the receiver at the real validated boundary, before replay advances.
    // It remains historical evidence while ready is cleared for later input.
    std::shared_ptr<const SourceAuthorityEndpoint> endpoint;
    std::uint64_t endpoint_copies{},endpoint_copy_microseconds{},endpoint_bank_bytes{},endpoint_budget_skips{},endpoint_copy_failures{};
    // Optional local retention failures never change acknowledged replay/error.
    // Opt-in qualification reports this separately; historical pins stay valid.
    std::string endpoint_error;
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
// Immutable paired native checkpoint and independently checked host proof.
// Only the receiver may construct one; no executor or memory borrower escapes.
class SourceAuthorityEndpoint {
public:
    const SourceAuthorityRetainedWorld &world()const noexcept{return *world_;}
    const SourceAuthorityHostProof &proof()const noexcept{return proof_;}
private:
    friend class SourceAuthorityReceiver;
    SourceAuthorityEndpoint(SourceAuthorityHostProof proof,std::shared_ptr<const SourceAuthorityRetainedWorld> world)
        :proof_(std::move(proof)),world_(std::move(world)){}
    SourceAuthorityHostProof proof_;
    std::shared_ptr<const SourceAuthorityRetainedWorld> world_;
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
    // Exact currently validated paired value, without stopping receipt/replay.
    // Rejects stale/foreign/unpaired cuts; no later world can be relabeled as
    // expected. The owning result contains the validated host table, independent
    // admissions and producer/recipient view mapping, but no executor borrower.
    // It is historical evidence, not readiness, election or promotion authority.
    SourceAuthorityRetainedWorld checkpoint_paired(const ValidatedAuthorityCut &expected)const;
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
    void retain_endpoint();
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
    // Nonterminal exact paired export on the worker's native executor. Shares
    // the single outstanding-export slot with checkpoint/terminal retention;
    // success leaves receiving, replay scheduling and later exports enabled.
    std::future<SourceAuthorityRetainedWorld> checkpoint_paired(ValidatedAuthorityCut);
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
