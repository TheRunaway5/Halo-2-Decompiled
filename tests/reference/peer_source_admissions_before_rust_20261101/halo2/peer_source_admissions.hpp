#pragma once
#include "halo2/multiplayer_datagrams.hpp"
#include "halo2/peer_authority_transfer.hpp"
#include "halo2/source_player_admission.hpp"

namespace halo2 {
struct AcceptedSourcePlayers {
    AuthorityTransferContext context;
    AuthorityTransferDigest digest{};
    std::vector<SourcePlayerAdmission> players;
    bool operator==(const AcceptedSourcePlayers &)const=default;
};
// Small, reliable admission exchange on the already authenticated peer path.
// The native host exports its live allocations, never an incoming checkpoint.
// The first accepted roster is immutable within its authority context. A local
// timeout cannot replace membership, identities or the hosting generation.
// Receive must be given the DTLS-verified physical sender. Original identity
// bytes remain private peer state: never publish them in UI, logs or stats.
class PeerSourceAdmissions {
public:
    PeerSourceAdmissions(unsigned self,AuthorityTransferContext,std::span<const OnlineAdmission>);
    ~PeerSourceAdmissions();
    AuthorityTransferContext context()const;
    bool replace_context(const AuthorityTransferContext &expected,AuthorityTransferContext next);
    // After an independently committed host-loss decision, retain all original
    // native/PCR allocations across epochs, including the departed host's rows.
    // previous must be this endpoint's already authenticated accepted roster.
    // This does not elect a host or certify a prepared world. A replacement
    // host's different native identities cannot overwrite the retained values.
    static std::unique_ptr<PeerSourceAdmissions> resume_after_host_loss(unsigned self,
        AuthorityTransferContext next,std::span<const OnlineAdmission>,const AcceptedSourcePlayers &previous);
    bool publish(const AuthorityTransferContext &,std::span<const SourcePlayerAdmission>);
    std::optional<AcceptedSourcePlayers> accepted()const;
    // Metadata delivery only. This never certifies a prepared source world;
    // that requires the separate complete-cut executor acknowledgement.
    bool confirmed(unsigned physical_peer)const;
    std::vector<AuthorityTransferPacket> poll(std::uint64_t now_ms,unsigned packet_budget=2);
    bool receive(unsigned authenticated_peer,std::span<const std::byte>);
    static bool is_packet(std::span<const std::byte>);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
