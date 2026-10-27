#pragma once
#include "halo2/multiplayer_world.hpp"
#include "halo2/source_authority_receiver.hpp"
#include "halo2/effect_presentation_history.hpp"
#include "halo2/peer_snapshot_candidate.hpp"
#include "halo2/peer_prospective_role.hpp"
#include "halo2/peer_survivor_selection.hpp"
#include "halo2/mcc_sound_variation_provider.hpp"
#include "halo2/source_foreground_world_handoff.hpp"
#include "halo2/online_match_session.hpp"
#include <future>
#include <thread>

namespace halo2 {
class LanHost;
class LanClient;
class SourceAuthorityReplayWorker;
class SourcePeerHostTakeover;
class SourceLensPresentationHandoffTu15;
class SourceAdmittedCameraBank;
struct SourceAuthorityRetainedWorld;

// Executor-local lifetime of the live Match, transport role and native shadow
// stream. The initial world remains resident for immutable renderer resources.
// A completed frame is the only place where a staged role may be adopted.
// Rendering/input adapters prepare their world-dependent owners before
// activation, and publish through its commit hook before the transport epoch.
// Staging alone never changes the live world or session.
class MultiplayerRuntimeAuthority {
    struct CommonPair;
    struct Impl;
public:
    using MigrationPreparationOutcome=OnlineMatchSession::MigrationPreparationOutcome;
    // Automatic gameplay enrollment follows the actual legacy/peer continuity
    // contract. Explicit native preparation owners can opt in without enabling
    // election, advertising Ready, or changing transport capabilities.
    enum class RecoveryPreparation { Automatic,PreparationOnly };
    static bool recovery_preparation_enabled(const std::shared_ptr<OnlineMatchSession> &,
        RecoveryPreparation = RecoveryPreparation::Automatic);
    MultiplayerRuntimeAuthority(const MultiplayerOptions &,
        std::shared_ptr<MultiplayerPreparedWorld>,std::unique_ptr<LanHost>,
        std::unique_ptr<LanClient>,std::unique_ptr<SourceAuthorityReplayWorker> = {},
        RecoveryPreparation = RecoveryPreparation::Automatic);
    ~MultiplayerRuntimeAuthority();
    MultiplayerRuntimeAuthority(const MultiplayerRuntimeAuthority &)=delete;
    MultiplayerRuntimeAuthority &operator=(const MultiplayerRuntimeAuthority &)=delete;
    struct OuterRetirement {
        enum class Reason { Abort,ContextRetired,ClientRebindRetired,ProspectiveClientRetired,ProspectiveHostRetired,
            CommonPreparationCancelled };
        AuthorityTransferContext context;
        AuthorityCutSeal seal;
        Reason reason;
    };
    using OuterRetirementGate=bool(*)(void *,const OuterRetirement &)noexcept;
    class OuterRetirementGuard {
    public:
        ~OuterRetirementGuard();
        OuterRetirementGuard(const OuterRetirementGuard &)=delete;
        OuterRetirementGuard &operator=(const OuterRetirementGuard &)=delete;
    private:
        friend class MultiplayerRuntimeAuthority;
        explicit OuterRetirementGuard(Impl &,OuterRetirementGate,void *);
        Impl *owner_;
    };
    // Install one executor-local outer-owner retirement gate at a completed
    // frame, declared after every captured outer holder. This runtime and the
    // captured storage must outlive the guard; its creator-only destruction
    // clears registration before those holders retire. No active frame,
    // timing/SDL hold, or matching outer publication hold may span registration
    // or destruction. An unrelated retained private prospective scope remains
    // eligible while the gate retires another cut's outer borrowers.
    // The gate receives captured exact metadata after retirement is detected,
    // outside session locks and before matching native/staged owners or pins
    // are destroyed or acknowledged. False retains those owners and defers
    // retirement; true means all matching outer borrowers have actually gone.
    // It must not reenter this runtime/session, perform device I/O, acquire a
    // publication hold, or claim activation/readiness. Retire only matching
    // unpublished holders and preserve unrelated released preparations.
    // No registration is compatible only with callers having no such outer
    // borrowers; there is deliberately no caller-supplied boolean bypass.
    // ProspectiveClientRetired and ProspectiveHostRetired retire only the
    // named losing role. Both alternatives can share the same context/cut;
    // preserve the winning role's outer owners in these callbacks.
    // CommonPreparationCancelled retires every unpublished alternative for
    // that exact cut before retry. Ordinary recovery CommonPairs remain owned;
    // this callback neither aborts a Common round nor acknowledges retirement.
    std::unique_ptr<OuterRetirementGuard> hold_outer_retirement_gate(OuterRetirementGate,void *);
    MultiplayerPreparedWorld &world();
    MultiplayerMatch &match();
    LanHost *host();
    LanClient *client();
    // Quiescent ordinary raw-LAN handoff only; migration and ONLINE owners
    // cannot export their transports through this session-return boundary.
    std::pair<std::unique_ptr<LanHost>,std::unique_ptr<LanClient>> release_system_link();
    // Service publication/receiver status between frames. A false outer gate
    // defers this service without advancing its retained context; retry after
    // the outer owners can retire. Admission may also run after native hello
    // processing within a frame, before its next cut.
    void service();
    void receive();
    void admit_players();
    // Opt-in terminal recovery admission only: true after this exact current
    // generation's genuine paired initial cut was independently validated.
    // Does not tick a world, set a Live-ready flag or elect a successor host.
    bool terminal_bootstrap_ready()const;
    // Only the opt-in tick0-admitted numeric bank. Null before the initial cut
    // starts its authentic execution journal or for ordinary default gameplay.
    SourceAdmittedCameraBank *admitted_cameras();
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
    // session fenced its generation, a prospective live-client replica is
    // staged, or outer retirement is deferred. Nested frames,
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
    enum class RecoveryEligibility { Unavailable,LegacyDrain,LearnedCommonCut };
    // Completed-executor observation for the post-disconnect caller only.
    // Ordinary matchmaking has no peer-migration decision and cannot wait
    // for a learned cut. Both paths require an actual learned decision;
    // common rounds additionally require that already learned exact cut
    // with its actual owning pair; this neither admits nor exports a world.
    // Explicit fixture/manual recover() retains its existing strict contract.
    RecoveryEligibility recovery_eligibility()const;
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
    // Terminal qualification: latest independently replayed owning endpoint.
    // Adoption requires its exact authenticated snapshot receipt, never tick
    // equality. Retaining it does not elect, promote, or stop the live worker.
    std::shared_ptr<const SourceAuthorityEndpoint> terminal_endpoint()const;
    void adopt_terminal_endpoint(const std::shared_ptr<const SourceAuthorityEndpoint> &,
        const MultiplayerSnapshotOrigin &);
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
    // Weak creator-thread identity of one immutable inventory retained by this
    // runtime. The runtime alone owns its actual native/foreground pairs, so
    // Abort/context retirement can drop them before acknowledging retirement.
    // Copies do not extend ownership or freeze any live foreground/device/input
    // owner. current() observes this lease's lifetime only; runtime methods also
    // recheck the independently admitted Session generation at a completed frame.
    class CommonInventoryLease {
    public:
        CommonInventoryLease()=default;
        bool current()const noexcept;
    private:
        struct State;
        std::weak_ptr<State> state_;
        std::thread::id executor_;
        explicit CommonInventoryLease(const std::shared_ptr<State> &);
        friend class MultiplayerRuntimeAuthority;
        friend struct MultiplayerRuntimeAuthority::Impl;
    };
    // Retain only actual associated strong pairs, deduplicated across the
    // committed/pending/imported slots. Rust derives the bounded inventory and
    // current genuine Common provenance; an unpaired native pin is not a row.
    // One active immutable lease is allowed. Identical requests return its weak
    // identity; another lease id cannot replace it. Release/history rotation
    // preserves these owners, while a matching Abort invalidates the whole
    // inventory after outer retirement and before the native Abort ACK.
    std::optional<CommonInventoryLease> retain_common_inventory(std::uint64_t lease);
    std::optional<PeerSurvivorInventory> common_inventory(const CommonInventoryLease &);
    // Explicit completed-frame execution of the Session's real Rust driver.
    // Prepare retains only genuine immutable common pairs and submits their
    // metadata for fresh serialized admission; it never prepares a live role
    // or advertises full Ready. Retire keeps its opaque Session reservation
    // across outer vetoes, fencing frames and new preparations until all
    // unpublished local alternatives have actually retired outside Session
    // locks. Ordinary recovery pairs and the live world/transport remain owned.
    // No outer publication/timing/SDL hold may span this step; the separately
    // retained private native scope is released only after its outer gate.
    // Late authenticated fencing/context changes can finish retirement but
    // never resume or reset that generation. No production caller or default
    // feature gate is enabled by this explicit executor step.
    MigrationPreparationOutcome advance_common_preparation();
    // Publish only the retained metadata through the Session's Rust admission
    // and fixed-survivor barrier. Fencing is not full native/presentation Ready.
    // Rejection preserves an otherwise-current lease; no replacement/restart or
    // Ready publication is performed here.
    bool freeze_common_inventory(const CommonInventoryLease &);
    // Explicit synchronous reconstruction of one exact strong common pair,
    // at a completed frame on this client's executor. At most one private
    // native import is retained, together with its entire native/foreground
    // pair. Repeating the same request does no work; another cut is refused.
    // This neither advertises Ready nor elects/freezes/replaces a live world.
    // Production's guarded prepare_takeover consumes the same imported owner;
    // a pre-election scheduling/freeze driver remains a separate caller duty.
    // Pending Abort/context retirement discards this private import before
    // acknowledging retirement. Released imports survive ordinary rotation.
    bool prepare_common_native(const ValidatedAuthorityCut &,
        std::shared_ptr<PhysicsLiveClockSource> = {});
    // Metadata only: the private imported native target never escapes here.
    std::optional<ValidatedAuthorityCut> prepared_common_native()const;
    // Prepare this PC's possible host role on its private imported target,
    // before target-dependent producers or immutable full Ready. Rust derives
    // the prospective generation; it is not an elected decision. This runs
    // native camera/departure/lens work once and leaves the live role alone.
    bool prepare_common_prospective(const ValidatedAuthorityCut &,
        std::shared_ptr<PhysicsLiveClockSource> = {});
    std::optional<PeerProspectiveHostRole> prepared_common_prospective()const;
    // Retain this endpoint's possible surviving-client alternative before any
    // learned decision. Its authenticated historical receipt and ONE staged
    // replica belong to the live resident display world, separately from the
    // possible self-host's private native target. Rust derives neutral role
    // metadata; no endpoint, Ready or Session generation is published.
    // Ordinary frames remain fenced while the live replica is staged. The
    // caller also freezes actual outer presentation/input/audio producers.
    // Selection of one exact cut precedes these explicit owner preparations.
    bool prepare_common_client(const ValidatedAuthorityCut &);
    std::optional<PeerProspectiveClientRole> prepared_common_client()const;
    // Typed native-reading foreground preparation precedes seal. No mutable
    // prospective world or generic native callback escapes to the caller.
    SourceForegroundWorldHandoff::Prepared prepare_common_foreground(
        const ValidatedAuthorityCut &,const SourceForegroundWorldHandoff &);
    // Retain real native exclusion after all native-reading preparations.
    // Audio receives only a weak, genuine target binding; no SDL/timing hold
    // spans the election. Seal does not claim complete readiness/activation.
    bool seal_common_prospective(const ValidatedAuthorityCut &);
    std::optional<MccSoundVariationProvider::NativeBinding> common_prospective_audio_binding(
        const ValidatedAuthorityCut &)const;
    // Weak identity of this exact retained native scope and derived boundary.
    // Typed foreground/lens preparations bind once to it; it cannot stand in
    // for their resource/input/device readiness or a migration certificate.
    std::optional<SourceNativePublication::Boundary> common_prospective_boundary(
        const ValidatedAuthorityCut &)const;
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
    // Explicit certificate counterpart. A noncurrent or mismatched decision
    // refuses before retiring replay or consuming an exact private import.
    bool prepare_takeover(const PeerMigrationDecision &,std::shared_ptr<PhysicsLiveClockSource> = {});
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
    // Complete the successful prospective publication only after the caller
    // released SDL/timing/outer publication holds, outside Session locks.
    // Ordinary frames remain fenced until this native exclusion is released.
    void release_prospective_publication();
    // Surviving client counterpart. Stage the locally learned decision without
    // changing its native endpoint. The same presentation hook runs before
    // native rebind and transport-epoch publication, under the same contract.
    bool prepare_client_rebind();
    // Bind a retained pre-election alternative only to this exact complete
    // independently learned certificate. Wrong ballots/voters refuse before
    // any losing-role retirement. No receipt copy or replica restaging occurs.
    bool prepare_client_rebind(const PeerMigrationDecision &);
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
    friend class SourceLensPresentationHandoffTu15;
    // Typed renderer-side producer only, before native sealing. No generic
    // mutable-world getter or callback is exposed by this runtime.
    MultiplayerPreparedWorld &common_prospective_target_for_lens(const ValidatedAuthorityCut &);
    bool prepare_client_rebind_from(const MultiplayerSnapshotProof *,const MultiplayerSnapshotOrigin *,
        const Simulation::ReplicaCheckpoint *,const PeerMigrationDecision * = nullptr);
    std::unique_ptr<Impl> impl_;
};
}
