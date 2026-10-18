#pragma once
#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/online_match_session.hpp"
#include "halo2/source_match_authority.hpp"
#include <thread>

namespace halo2 {
struct SourceAuthorityRetainedWorld;
// One replacement native host, constructed from a receiver's exact validated
// paired cut on that receiver's executor. It never moves a native world across
// threads. The caller must quiesce old world borrowers before adopting it.
// This owns simulation/input/camera state, not renderer, controllers or audio
// devices. Those owners must be rebound before activate() opens session media.
// Every operation and destruction belongs to the constructing thread.
class SourcePeerHostTakeover {
public:
    ~SourcePeerHostTakeover();
    SourcePeerHostTakeover(const SourcePeerHostTakeover &)=delete;
    SourcePeerHostTakeover &operator=(const SourcePeerHostTakeover &)=delete;
    // Import an owning value exported by a now-quiescent replay executor.
    // Independent current session admissions, views and the local learned
    // decision are checked before importing into this thread's private pair.
    // No executor, borrowed native pointer or hardware crosses this boundary.
    static std::unique_ptr<SourcePeerHostTakeover> prepare(const MultiplayerOptions &,
        const SourceAuthorityRetainedWorld &,const PeerMigrationDecision &,
        std::shared_ptr<PhysicsLiveClockSource> candidate_clock={},
        std::shared_ptr<PhysicsLiveClockSource> workspace_clock={});
    const PeerMigrationDecision &decision()const;
    MultiplayerPreparedWorld &world();
    // Sending/receiving the native host is available only after activation.
    LanHost &host();
    LanHost::LocalContinuation &local();
    NativeVehicleCamera &camera();
    // Rechecks the still-current local decision and independent admissions.
    // False leaves the native owner private and session traffic fenced. A
    // successful activation is one-shot; it never re-admits or respawns seats.
    // Retained voice may still be retiring old codec/device queues. Retry with
    // the same unmodified owner while its local decision remains current;
    // retirement performs no device I/O on this executor.
    bool activate();
private:
    friend class SourceAuthorityReceiver;
    SourcePeerHostTakeover(std::shared_ptr<OnlineMatchSession>,PeerMigrationDecision,
        AcceptedSourcePlayers,std::unique_ptr<MultiplayerPreparedWorld>,const LanHost::Continuation &);
    void own()const;
    const std::thread::id executor_=std::this_thread::get_id();
    const std::shared_ptr<OnlineMatchSession> session_;
    const PeerMigrationDecision decision_;
    const AcceptedSourcePlayers admissions_;
    std::unique_ptr<MultiplayerPreparedWorld> world_;
    std::unique_ptr<LanHost> host_;
    LanHost::LocalContinuation local_;
    std::unique_ptr<NativeVehicleCamera> camera_;
    SourceMatchAuthority::ExecutionBoundary prepared_;
    bool active_{};
};
}
