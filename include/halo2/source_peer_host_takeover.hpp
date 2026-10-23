#pragma once
#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/online_match_session.hpp"
#include "halo2/source_match_authority.hpp"
#include "halo2/source_native_publication.hpp"
#include "halo2/mcc_sound_variation_provider.hpp"
#include "halo2/peer_prospective_role.hpp"
#include <thread>

namespace halo2 {
struct SourceAuthorityRetainedWorld;
// One replacement native host, constructed from a receiver's exact validated
// paired cut on that receiver's executor. It never moves a native world across
// threads. The caller must quiesce old world borrowers before adopting it.
// This owns simulation/input/camera state, not renderer, controllers or audio
// devices. Those owners must be rebound before activate() opens session media.
// Every operation and destruction belongs to the constructing thread.
// Destruction may close the retained session/voice transport; perform it
// outside timing, SDL and device holds. A native-only publication can retain
// the exact target after this wrapper closes, but not arbitrary outer owners.
class SourcePeerHostTakeover {
public:
    ~SourcePeerHostTakeover();
    SourcePeerHostTakeover(const SourcePeerHostTakeover &)=delete;
    SourcePeerHostTakeover &operator=(const SourcePeerHostTakeover &)=delete;
    // Import an owning value exported by a now-quiescent replay executor.
    // Independent current session admissions, views and the local learned
    // decision are checked before importing into one private source world.
    // Import first uses the producer's admitted views, then rebinds the actual
    // recipient views before the private owner can be staged or published.
    // No executor, borrowed native pointer or hardware crosses this boundary.
    static std::unique_ptr<SourcePeerHostTakeover> prepare(const MultiplayerOptions &,
        const SourceAuthorityRetainedWorld &,const PeerMigrationDecision &,
        std::shared_ptr<PhysicsLiveClockSource> candidate_clock={});
    const PeerMigrationDecision &decision()const;
    MultiplayerPreparedWorld &world();
    // Sending/receiving the native host is available only after activation.
    LanHost &host();
    LanHost::LocalContinuation &local();
    // Refresh only the local pending input from the still-admitted old client.
    // The sealed host bank authenticates its cookies, profiles and counters.
    // Native world/camera state and already-delivered serials stay unchanged.
    void refresh_local_input(const LanClient &);
    NativeVehicleCamera &camera();
    // Hold the already-imported exact native target after its complete sealed
    // boundary checks. Separately quiesce Match/Simulation/renderer and raw
    // ownership-slot writers. This never admits or activates migration.
    SourceNativePublication hold_native_publication();
    // Rechecks the still-current local decision and independent admissions.
    // False leaves the native owner private and session traffic fenced. A
    // successful activation is one-shot; it never re-admits or respawns seats.
    // Retained voice may still be retiring old codec/device queues. Retry with
    // the same unmodified owner while its local decision remains current;
    // retirement performs no device I/O on this executor.
    // Optional allocation-free native adoption runs before transport epoch
    // publication, on this executor. It must not call the session; false must
    // leave ownership unchanged. Old native owners retire after this returns.
    bool activate(OnlineMatchSession::NativeCommit native_adopt=nullptr,void *owner=nullptr);
private:
    friend class SourceAuthorityReceiver;
    friend class MultiplayerRuntimeAuthority;
    friend class SourceForegroundWorldHandoff;
    friend class SourceLensPresentationHandoffTu15;
    // Typed presentation friends validate the world's original Cache through
    // this existing native owner; the World accessor stays private.
    static std::shared_ptr<const Cache> original_presentation_cache(const MultiplayerPreparedWorld &);
    // Decision-free native reconstruction stays private and has no public
    // mutable target, host transport, publication token or readiness claim.
    // Its creator/exact cut/admissions/boundary are rechecked before the same
    // world moves into the certificate-gated owning takeover below.
    struct NativeImport;
    static std::shared_ptr<NativeImport> import_native(const MultiplayerOptions &,
        const SourceAuthorityRetainedWorld &,std::shared_ptr<PhysicsLiveClockSource>,
        const MultiplayerPreparedWorld *);
    static const ValidatedAuthorityCut &imported_cut(const NativeImport &);
    // A prospective role is a private hypothesis derived by Rust, never a
    // Decision or Ready. All camera/departure/lens mutations run once here,
    // before a typed foreground producer can prepare against this target.
    // Failure after consumption discards the partially finalized owner.
    static bool prepare_prospective(std::shared_ptr<NativeImport> &,const PeerProspectiveHostRole &);
    static bool has_prospective(const NativeImport &)noexcept;
    static const PeerProspectiveHostRole &prospective_role(const NativeImport &);
    // Only trusted typed producer friends may borrow before sealing. There
    // is no public mutable target callback/getter and no borrow after seal.
    static MultiplayerPreparedWorld &prospective_target(NativeImport &);
    static void seal_prospective(NativeImport &);
    // Sealed native identity only. No Session check or complete foreground /
    // device/input readiness is implied by this direct held witness.
    static bool prospective_ready(const NativeImport &)noexcept;
    static MccSoundVariationProvider::NativeBinding prospective_audio_binding(const NativeImport &);
    static SourceNativePublication::Boundary prospective_boundary_binding(NativeImport &);
    // Refusal before consumption preserves the handle. Once role finalization
    // consumes its world, success or an exception clears the handle; a partly
    // finalized target is never offered again as a valid private import.
    static std::unique_ptr<SourcePeerHostTakeover> prepare_imported(
        std::shared_ptr<NativeImport> &,const PeerMigrationDecision &);
    // Runtime-only same-creator reconstruction after its replay executor has
    // retired. The resident supplies its private original Cache owner; no
    // shared Cache is carried by worker options or a public caller override.
    static std::unique_ptr<SourcePeerHostTakeover> prepare_with_resident_cache(
        const MultiplayerOptions &,const SourceAuthorityRetainedWorld &,
        const PeerMigrationDecision &,const MultiplayerPreparedWorld &,
        std::shared_ptr<PhysicsLiveClockSource>);
    static std::unique_ptr<SourcePeerHostTakeover> prepare_impl(
        const MultiplayerOptions &,const SourceAuthorityRetainedWorld &,
        const PeerMigrationDecision &,std::shared_ptr<PhysicsLiveClockSource>,
        const MultiplayerPreparedWorld *);
    SourcePeerHostTakeover(std::shared_ptr<OnlineMatchSession>,PeerMigrationDecision,
        AcceptedSourcePlayers,std::unique_ptr<MultiplayerPreparedWorld>,const LanHost::Continuation &,
        const ValidatedAuthorityCut &);
    struct ProspectiveNative;
    SourcePeerHostTakeover(std::shared_ptr<OnlineMatchSession>,PeerMigrationDecision,
        AcceptedSourcePlayers,std::unique_ptr<ProspectiveNative>)noexcept;
    static void prepare_role_mutations(ProspectiveNative &,const std::shared_ptr<OnlineMatchSession> &,
        const AcceptedSourcePlayers &,SourceMatchAuthority &);
    static void close_prospective(NativeImport &)noexcept;
    bool has_prospective()const noexcept;
    bool publication_held()const noexcept;
    const PeerMigrationDecision &prospective_decision()const;
    std::unique_ptr<SourceMatchAuthority> take_prospective_authority();
    void refresh_prospective_local_input(const LanClient &);
    bool activate_prospective(OnlineMatchSession::NativeCommit,void *);
    // Successful aggregate adoption releases its actual native-only scope
    // after Session/timing/SDL holds. Public world/host access stays excluded
    // until then. Retirement of an unpublished owner releases it on teardown.
    void release_prospective_publication();
    bool held_prospective()const noexcept;
    void own()const;
    const std::thread::id executor_=std::this_thread::get_id();
    const std::shared_ptr<OnlineMatchSession> session_;
    const PeerMigrationDecision decision_;
    const AcceptedSourcePlayers admissions_;
    std::shared_ptr<MultiplayerPreparedWorld> world_;
    std::shared_ptr<SourceNativePublication::Owner> publication_owner_;
    std::unique_ptr<LanHost> host_;
    LanHost::LocalContinuation local_;
    LanHost::Continuation input_cut_;
    std::unique_ptr<NativeVehicleCamera> camera_;
    SourceMatchAuthority::ExecutionBoundary prepared_;
    bool active_{};
    std::shared_ptr<const Cache> prospective_cache_;
    const PhysicsLiveWorldTu15 *prospective_native_{};
    std::optional<std::uint64_t> prospective_fingerprint_;
    std::unique_ptr<SourceMatchAuthority> prospective_authority_;
    // Last: release native exclusion before any referenced host/world owner.
    std::unique_ptr<SourceNativePublication> prospective_publication_;
};
}
