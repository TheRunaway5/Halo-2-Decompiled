#pragma once
#include <cstdint>

namespace halo2 {
// Network admission is independent of larger opt-in offline archives.
inline constexpr std::uint64_t authority_network_encoded_bytes=32ull*1024*1024;
inline constexpr std::uint64_t authority_network_decoded_bytes=96ull*1024*1024;
inline constexpr std::uint64_t authority_network_native_bytes=96ull*1024*1024;
inline constexpr std::uint64_t authority_network_parser_bytes=448ull*1024*1024;
inline constexpr std::uint64_t authority_network_staged_bytes=768ull*1024*1024;
inline constexpr std::uint64_t authority_network_asset_bytes=32ull*1024*1024;
inline constexpr std::uint64_t authority_network_working_bytes=1184ull*1024*1024;
inline constexpr std::uint64_t authority_network_fixed_bytes=64ull*1024*1024;
// Two wire slots, retained base and successor, weighted native staging,
// bounded immutable grenade assets and fixed schema/host scaffolding. Existing
// local map/cache worlds and allocator bookkeeping remain separate.
static_assert(2*authority_network_encoded_bytes+2*authority_network_decoded_bytes+
    authority_network_staged_bytes+3*authority_network_asset_bytes+authority_network_fixed_bytes<=authority_network_working_bytes);
}
