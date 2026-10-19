#pragma once
#include "halo2/peer_authority_transfer.hpp"
#include <compare>

namespace halo2 {
struct PeerMigrationBallot {
    std::uint64_t round{};
    unsigned proposer=16;
    auto operator<=>(const PeerMigrationBallot &)const=default;
};
struct PeerMigrationValue {
    unsigned host=16;
    AuthorityCutSeal cut;
    bool operator==(const PeerMigrationValue &)const=default;
};
struct PeerMigrationDecision {
    AuthorityTransferContext previous,next;
    PeerMigrationBallot ballot;
    PeerMigrationValue value;
    std::uint16_t voters{};
    bool operator==(const PeerMigrationDecision &)const=default;
};
struct PeerHostAgreementState {
    bool suspected{},fenced{};
    std::uint16_t electorate{},silent{};
    unsigned majority{};
    std::array<std::optional<AuthorityCutSeal>,16> ready;
    std::optional<PeerMigrationDecision> decision;
    std::uint64_t rejected{};
};
// One agreed physical membership/host generation, never a party/seat quorum.
// authenticated_peer and context must come from the established DTLS session.
// The failed host is excluded; local timeouts never shrink the remaining
// electorate. Accepted proposals survive retries/preemption for this object's
// entire lifetime. Do not reconstruct it within the same live generation.
// This is crash/partition consensus, not Byzantine consensus or a cheating ban.
// Every public operation is synchronized. No native world or device is owned.
class PeerHostAgreement {
public:
    PeerHostAgreement(unsigned self,AuthorityTransferContext,std::uint64_t last_host_ms);
    ~PeerHostAgreement();
    PeerHostAgreement(const PeerHostAgreement &)=delete;
    PeerHostAgreement &operator=(const PeerHostAgreement &)=delete;
    // Only authenticated traffic from the accepted simulation host counts.
    // A granted accept/final decision permanently fences that old generation;
    // late host traffic cannot erase votes or silently undo the transition.
    void observe_host(std::uint64_t now_ms);
    // Freeze/retain one complete, natively validated paired world/host cut
    // before publishing. The caller must retain that exact owning state until
    // this generation ends. Byte delivery or a display snapshot is insufficient.
    bool publish_ready(const ValidatedAuthorityCut &);
    // The caller chooses among ready candidates using measured survivor QoS.
    // A retry must carry the highest prior accepted value learned from promises.
    bool propose(unsigned candidate,std::uint64_t now_ms);
    // Foreground-independent session pump. Starts/retries a proposal using the
    // caller's measured, ready candidate; frequent calls do not reset an
    // in-flight ballot. Fixed-electorate ordering staggers competing proposers,
    // while new promises/votes defer retries. Duplicate traffic cannot keep a
    // failed proposer alive. The 500ms suspicion threshold is unchanged.
    bool drive(unsigned candidate,std::uint64_t now_ms);
    std::vector<AuthorityTransferPacket> poll(std::uint64_t now_ms,unsigned budget=4);
    bool receive(unsigned authenticated_peer,std::span<const std::byte>,std::uint64_t now_ms);
    PeerHostAgreementState state(std::uint64_t now_ms)const;
    static bool is_packet(std::span<const std::byte>)noexcept;
private:
    struct Impl;std::unique_ptr<Impl> impl_;
};
}
