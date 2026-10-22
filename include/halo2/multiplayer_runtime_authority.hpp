#pragma once
#include "halo2/multiplayer_world.hpp"
#include "halo2/source_authority_receiver.hpp"
#include "halo2/effect_presentation_history.hpp"
#include "halo2/peer_snapshot_candidate.hpp"
#include <future>

namespace halo2 {
class LanHost;
class LanClient;
class SourceAuthorityReplayWorker;
class SourcePeerHostTakeover;
struct SourceAuthorityRetainedWorld;

// Executor-local lifetime of the live Match, transport role and native shadow
// stream. The initial world remains resident for immutable renderer resources.
// A completed frame is the only place where a staged role may be adopted.
// Rendering/input adapters prepare their world-dependent owners before
// activation, and publish through its commit hook before the transport epoch.
// Staging alone never changes the live world or session.
class MultiplayerRuntimeAuthority {
    struct Impl;
public:
    MultiplayerRuntimeAuthority(const MultiplayerOptions &,
        std::shared_ptr<MultiplayerPreparedWorld>,std::unique_ptr<LanHost>,
        std::unique_ptr<LanClient>,std::unique_ptr<SourceAuthorityReplayWorker> = {});
    ~MultiplayerRuntimeAuthority();
    MultiplayerRuntimeAuthority(const MultiplayerRuntimeAuthority &)=delete;
    MultiplayerRuntimeAuthority &operator=(const MultiplayerRuntimeAuthority &)=delete;
    MultiplayerPreparedWorld &world();
    MultiplayerMatch &match();
    LanHost *host();
    LanClient *client();
    // Service publication/receiver status between frames. Admission may also
    // run after native hello processing within a frame, before its next cut.
    void service();
    void receive();
    void admit_players();
    void suspend_publication();
    class Frame {
    public:
        ~Frame();
        bool finish()noexcept;
        Frame(const Frame &)=delete;
        Frame &operator=(const Frame &)=delete;
    private:
        friend class MultiplayerRuntimeAuthority;
        struct Impl;std::unique_ptr<Impl> impl_;
        explicit Frame(MultiplayerRuntimeAuthority &);
    };
    // Holds the live world even when no publisher is recording. Null means the
    // session fenced its generation before the frame began. Nested frames,
    // publication service and role changes are rejected until finish/destruction.
    // This runtime must outlive every returned Frame, on the same executor.
    std::unique_ptr<Frame> begin_frame();
    // Replaces a normal gameplay host broadcast. In negotiated candidate mode
    // the selected genuine broadcast closes this frame, captures its exact
    // paired native/host values before any further native call, then starts a
    // new frame. The caller must honor a null replacement generation fence.
    // Unconfigured/unarmed gameplay keeps its ordinary broadcast behavior.
    void broadcast_candidate_frame(std::unique_ptr<Frame> &);
    struct SnapshotCandidateCut {
        PeerSnapshotToken token;
        AuthorityCutSeal seal;
        bool operator==(const SnapshotCandidateCut &)const=default;
    };
    // Exact producer-local lineage only. Recipients still need the original
    // independently decoded HostProof and authentic marked receipt.
    std::optional<SnapshotCandidateCut> snapshot_candidate_cut()const;
    // One-shot, nonblocking drain. Null means the worker is still completing
    // authenticated phases. The owning result survives worker destruction.
    std::shared_ptr<const SourceAuthorityRetainedWorld> recover();
    void cancel_recovery();
    bool has_receiver()const;
    std::optional<SourceAuthorityReceiverState> shadow_state()const;
    // One independently validated exact native export. Only this runtime can
    // create a pin; it owns both replay/world and host proof beyond worker
    // advancement/destruction, without retaining an executor borrower.
    class NativeCutPin {
    public:
        const ValidatedAuthorityCut &cut()const;
        const SourceAuthorityHostProof &host_proof()const;
        const SourceAuthorityRetainedWorld &retained_world()const;
    private:
        friend struct MultiplayerRuntimeAuthority::Impl;
        NativeCutPin(SourceAuthorityHostProof,std::shared_ptr<const SourceAuthorityRetainedWorld>);
        SourceAuthorityHostProof proof_;
        std::shared_ptr<const SourceAuthorityRetainedWorld> retained_;
    };
    std::shared_ptr<const NativeCutPin> pending_native_cut()const;
    // Completed-frame association requires the privately bound foreground
    // checkpoint for this exact stored native pin. Both owners are retained
    // before publishing association. Commit/Apply/Release concern recovery-pin
    // ownership only; they never activate a world or advertise full readiness.
    bool associate_common_cut(const std::shared_ptr<const NativeCutPin> &,
        const EffectPresentationHistory::RuntimeCheckpoint &);
    // Committed and pending strong pairs live outside ordinary history
    // eviction. Only the exact admitted generation/seal can retrieve one.
    std::optional<EffectPresentationHistory::RuntimeCheckpoint> common_cut_checkpoint(
        const AuthorityTransferContext &,const AuthorityCutSeal &)const;
    // The caller keeps ownership if staging is rejected. A pending takeover
    // remains private while audio retirement or adapter rebinding is incomplete.
    void stage_takeover(std::unique_ptr<SourcePeerHostTakeover> &&);
    // At a completed frame, retire the replay worker and prepare/stage this
    // session's exact learned replacement from its retained owning value.
    // False means not elected or still draining. True leaves one private
    // pending owner for adapter rebinding and explicit activation. Repeated
    // calls do not rebuild it. Native reconstruction itself is synchronous.
    // Does not advertise readiness, elect a host or open gameplay/media.
    bool prepare_takeover(std::shared_ptr<PhysicsLiveClockSource> = {});
    SourcePeerHostTakeover *pending_takeover();
    // After complete decision/native/admission/audio validation and successor
    // allocation, publish prepared presentation owners before changing this
    // runtime's role or opening gameplay/media. A callback returning false
    // must leave its owners unchanged; rejection preserves native/transport
    // ownership and the same pending role can retry.
    // The callback is executor-local and noexcept. Successful publication must
    // not allocate, block, do device I/O, mutate native/transport state, or call
    // this runtime/session. Capture every required owner/token beforehand and
    // preflight all of them before the first irreversible swap. This hook does
    // not prepare adapters or advertise readiness on the caller's behalf.
    using PresentationCommit=bool(*)(void *)noexcept;
    bool activate_takeover(PresentationCommit=nullptr,void *owner=nullptr);
    // Surviving client counterpart. Stage the locally learned decision without
    // changing its native endpoint. The same presentation hook runs before
    // native rebind and transport-epoch publication, under the same contract.
    bool prepare_client_rebind();
    // Exact surviving-client state, scene/device/prop and composed collision
    // counterpart. Snapshot proof and local replica checkpoint must name the
    // same authentic receipt at the learned decision's cut. This stages both
    // native owners together; presentation adoption remains separately required.
    bool prepare_client_rebind(const MultiplayerSnapshotProof &,const MultiplayerSnapshotOrigin &,
        const Simulation::ReplicaCheckpoint &);
    // Discard only an unpublished client preparation, releasing its replica
    // staging barrier outside session locks. Native state/endpoint stay live.
    bool cancel_client_rebind();
    std::optional<PeerMigrationDecision> pending_client_rebind()const;
    bool activate_client_rebind(PresentationCommit=nullptr,void *owner=nullptr);
    // Promoted native camera bank; the ordinary initial role keeps its own
    // camera. The returned pointer stays executor-local and world-owned.
    NativeVehicleCamera *promoted_camera();
    bool local_input_pending()const;
    // Within a native frame, deliver retained held controls/pending serials
    // before fresh hardware is admitted. True owns this tick's local seats;
    // callers keep hardware input suspended until local_input_pending clears.
    // A neutral retained bank still owns the first continuation tick.
    // Repeated polls in that same simulation tick return the same controls
    // without consuming another serial. Hardware resumes after the step.
    bool continue_local_input(std::array<Controls,multiplayer_capacity> &);
private:
    bool prepare_client_rebind_from(const MultiplayerSnapshotProof *,const MultiplayerSnapshotOrigin *,
        const Simulation::ReplicaCheckpoint *);
    std::unique_ptr<Impl> impl_;
};
}
