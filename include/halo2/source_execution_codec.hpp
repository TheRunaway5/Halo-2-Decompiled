#pragma once
#include "halo2/source_execution_inputs.hpp"

namespace halo2 {
struct SourceExecutionCodecLimits {
    std::uint64_t encoded_bytes=16ull*1024*1024;
    std::uint64_t decoded_bytes=16ull*1024*1024;
    // Dynamic destination values. Raw compression buffers and the bounded
    // existing call/input validators have separate bounds; this is not RSS.
    std::uint64_t allocated_bytes=32ull*1024*1024;
};
// Owning little-endian values, independent of C++ layout. The checksum only
// detects corruption. The receiver must authenticate the physical host and
// bind the phase to its accepted generation, membership, base cut and sequence
// before replay. Decoding alone never advances or certifies a shadow world.
std::vector<std::byte> encode_source_execution_phase(const SourceExecutionPhase &,
    SourceExecutionCodecLimits = {});
SourceExecutionPhase decode_source_execution_phase(std::span<const std::byte>,
    SourceExecutionCodecLimits = {});
}
