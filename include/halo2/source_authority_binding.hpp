#pragma once
#include "halo2/source_match_authority.hpp"
#include "halo2/peer_authority_transfer.hpp"

namespace halo2 {
// Native room/authority/membership binding, independent of presentation keys
// and Discord IDs. This is a digest, not an authority or quorum certificate.
SourceMatchAuthority::AuthorityDigest source_authority_binding(const AuthorityTransferContext &);
}
