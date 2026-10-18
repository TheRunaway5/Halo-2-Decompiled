#pragma once
#include "halo2/multiplayer_network.hpp"
#include "halo2/source_authority_codec.hpp"
#include "halo2/source_replay_batch.hpp"

namespace halo2 {
// One owning native world and its matching host/input/camera state. The serial
// is the producer's cut ordinal; the outer peer-transfer sequence is separate.
// Neither value certifies an election, membership change or host promotion.
struct SourceHostContinuation {
  AuthorityTransferContext context;
  std::uint64_t serial{};
  SourceMatchAuthority::Checkpoint world;
  LanHost::Continuation host;
};
// Capture on this host's native executor at a completed frame. The paired
// world/host must remain quiescent across capture. All returned values own
// their storage and can be encoded later without borrowing that executor.
SourceHostContinuation capture_source_host_continuation(LanHost &,
    const AuthorityTransferContext &,std::uint64_t serial);
bool source_host_continuation_owner(LanHost &,const SourceMatchAuthority &);
bool source_host_continuation_payload(std::span<const std::byte>)noexcept;
// A bounded, checksummed container binds the host bytes to the exact encoded
// complete-world bytes. Authenticate the enclosing peer/context before decode,
// then perform native preparation with independent identities before use.
std::vector<std::byte> encode_source_host_continuation(const SourceHostContinuation &,
    SourceAuthorityCodecLimits = {});
SourceHostContinuation decode_source_host_continuation(std::span<const std::byte>,
    const AuthorityTransferContext &,std::uint64_t fingerprint,
    std::span<const OnlineAdmission>,std::span<const SourcePlayerAdmission>,SourceAuthorityCodecLimits = {});
// A replay cut proves the resulting world through its independently accepted
// base/previous/sequence and actual native execution. The host sidecar is bound
// to those exact replay bytes and the final tick; it is not a standalone world.
struct SourceHostReplay {SourceReplayBatch replay;LanHost::Continuation host;};
bool source_host_replay_payload(std::span<const std::byte>)noexcept;
std::vector<std::byte> encode_source_host_replay(const SourceHostReplay &,SourceReplayLimits = {});
SourceHostReplay decode_source_host_replay(std::span<const std::byte>,const SourceReplayBinding &,
    std::uint64_t fingerprint,std::span<const OnlineAdmission>,std::span<const SourcePlayerAdmission>,SourceReplayLimits = {});
}
