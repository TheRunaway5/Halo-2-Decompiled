#pragma once
#include "halo2/multiplayer_world.hpp"
#include "halo2/source_authority_receiver.hpp"
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
// Rendering/input adapters must rebind their world-dependent owners before
// activate_takeover(); staging alone never changes the live world or session.
class MultiplayerRuntimeAuthority {
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
    // One-shot, nonblocking drain. Null means the worker is still completing
    // authenticated phases. The owning result survives worker destruction.
    std::shared_ptr<const SourceAuthorityRetainedWorld> recover();
    void cancel_recovery();
    bool has_receiver()const;
    std::optional<SourceAuthorityReceiverState> shadow_state()const;
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
    bool activate_takeover();
    // Surviving client counterpart. Stage the locally learned decision without
    // changing its native endpoint; retire presentation borrowers before
    // activation. Native rebind commits before the session publishes its epoch.
    bool prepare_client_rebind();
    std::optional<PeerMigrationDecision> pending_client_rebind()const;
    bool activate_client_rebind();
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
    struct Impl;std::unique_ptr<Impl> impl_;
};
}
