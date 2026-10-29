#pragma once
#include "halo2/peer_authority_transfer.hpp"

namespace halo2 {
class LanClient;
struct MultiplayerState;
// Owning receipt for one fully decoded snapshot. Only LanClient can create
// it; the private connection/epoch/sequence/body identity cannot be supplied
// by a caller from a tick or a received host sidecar.
class MultiplayerSnapshotOrigin {
public:
    std::optional<std::uint32_t> tick()const noexcept;
    std::optional<std::uint32_t> sequence()const noexcept;
    // Actual authenticated complete body extent, for bounded local retention.
    // Zero for an absent receipt; this confers no proof or decoded admission.
    std::uint32_t encoded_body_bytes()const noexcept;
    std::optional<AuthorityTransferContext> context()const noexcept;
    bool same(const MultiplayerSnapshotOrigin &)const noexcept;
    // Continuity-bound or native-presentation receipts retain decoded values.
    // Missing/pre-admission receipts throw instead of certifying caller state.
    // References/spans remain valid while a copy of this receipt is retained,
    // including after later receive batches or destruction of its LanClient.
    const MultiplayerState &accepted_state()const;
    std::span<const unsigned> local_slots()const noexcept;
    // Historical connection/generation equality, not body equality, current
    // liveness, a native proof or permission to adopt either state.
    bool same_connection(const MultiplayerSnapshotOrigin &)const noexcept;
private:
    friend class LanClient;
    struct Data;std::shared_ptr<const Data> data_;
};
class MultiplayerSnapshotProof {
public:
    const ValidatedAuthorityCut &cut()const;
    bool matches(const MultiplayerSnapshotOrigin &)const noexcept;
private:
    friend class LanClient;
    struct Data;std::shared_ptr<const Data> data_;
};
}
