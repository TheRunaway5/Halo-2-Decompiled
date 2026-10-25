#pragma once
#include "halo2/source_execution_codec.hpp"
#include "halo2/peer_authority_transfer.hpp"

namespace halo2 {
enum class SourceAuthorityStreamMode { Snapshots,Replay };
struct SourceReplayBinding {
    AuthorityTransferContext context;
    AuthorityCutSeal base,previous;
    std::uint64_t first_sequence=1;
    bool operator==(const SourceReplayBinding &)const=default;
};
struct SourceReplayStep {
    std::uint64_t before_tick{},after_tick{};
    SourceExecutionPhase phase;
};
struct SourceReplayBatch {
    SourceReplayBinding binding;
    std::vector<SourceReplayStep> steps;
};
struct SourceReplayLimits {
    std::uint64_t encoded_bytes=32ull*1024*1024;
    std::uint64_t decoded_bytes=32ull*1024*1024;
    std::uint64_t allocated_bytes=64ull*1024*1024;
    unsigned phases=32;
};
// This recognizes the envelope, never its authority. The independent physical
// sender/context and previously validated complete world select expected.
bool source_replay_batch_payload(std::span<const std::byte>)noexcept;
void validate_source_network_replay_calls(std::span<const SourceExecutionCall>);
std::uint64_t source_replay_phase_allocation(const SourceExecutionPhase &);
std::vector<std::byte> encode_source_replay_batch(const SourceReplayBatch &,SourceReplayLimits = {});
// Check the accepted context/base/previous receipt/sequence before allocating
// or decompressing any phase. Each phase retains the native codec's bounds,
// with aggregate decoded/allocation budgets across the entire batch.
SourceReplayBatch decode_source_replay_batch(std::span<const std::byte>,const SourceReplayBinding &expected,
    SourceReplayLimits = {});
}
