#pragma once
#include "halo2/peer_authority_transfer.hpp"
#include "halo2/peer_host_agreement.hpp"

namespace halo2 {
// Checked Rust-derived role metadata only: no decision, votes, readiness,
// authorization or native ownership. A real independently learned certificate
// must later match this exact descriptor before any role/epoch publication.
struct PeerProspectiveHostRole {
    AuthorityTransferContext previous,next;
    AuthorityCutSeal cut;
    bool operator==(const PeerProspectiveHostRole &)const=default;
};
PeerProspectiveHostRole derive_peer_prospective_host_role(
    const AuthorityTransferContext &previous,const AuthorityCutSeal &selected,unsigned self);
// Endpoint-neutral metadata for the one resident client alternative. No host
// is selected, no native replica is staged and no certificate is fabricated.
struct PeerProspectiveClientRole {
    AuthorityTransferContext previous;
    AuthorityCutSeal cut;
    unsigned self;
    std::uint32_t next_epoch;
    std::uint64_t next_membership_revision;
    std::uint16_t survivors;
    bool operator==(const PeerProspectiveClientRole &)const=default;
};
PeerProspectiveClientRole derive_peer_prospective_client_role(
    const AuthorityTransferContext &previous,const AuthorityCutSeal &selected,unsigned self);
// Validates only exact endpoint metadata from the supplied real decision.
// Runtime must separately verify its complete independently learned certificate.
AuthorityTransferContext bind_peer_prospective_client_role(
    const PeerProspectiveClientRole &,const PeerMigrationDecision &);
}
