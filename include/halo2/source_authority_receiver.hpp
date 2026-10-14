#pragma once
#include "halo2/multiplayer_world.hpp"
#include "halo2/online_match_session.hpp"

namespace halo2 {
struct SourceAuthorityReceiverState {
    bool decoding{},warmed{};
    std::uint64_t received{},prepared{},decode_microseconds{},warm_microseconds{},prepare_microseconds{};
    bool delta{};
    std::optional<ValidatedAuthorityCut> ready;
    std::string error;
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
    ~SourceAuthorityReceiver();
    SourceAuthorityReceiver(const SourceAuthorityReceiver &)=delete;
    SourceAuthorityReceiver &operator=(const SourceAuthorityReceiver &)=delete;
    // Synchronous cold construction at an externally quiescent boundary.
    // Requires independently accepted source identities; no world receipt.
    bool warm();
    void update();
    SourceAuthorityReceiverState state()const;
    // Read-only, executor-local inspection. Expires on cancellation, context
    // change, replacement or take_world(); never retain borrowers across update.
    const MultiplayerPreparedWorld *world()const;
    // Transfer this exact validated world to an external continuation owner.
    // This does not promote a host or rebind peers. The application must first
    // certify its chosen cut/generation, then perform role/hardware rebinding.
    std::unique_ptr<MultiplayerPreparedWorld> take_world(const ValidatedAuthorityCut &expected);
    // Permanently stop this receiver and discard queued encoder output/cache.
    // Does not revoke an authority generation or stop peer transport/voice.
    void cancel();
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
