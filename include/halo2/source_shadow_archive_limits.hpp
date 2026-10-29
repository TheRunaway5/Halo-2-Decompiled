#pragma once
#include <cstdint>

namespace halo2 {
// Presentation carries the complete converted MCC tag bank as well as the
// native world. This is a separate admission policy from host recovery: the
// latter retains its original 96 MiB decoded/native ceilings.
inline constexpr std::uint64_t source_shadow_archive_encoded_bytes=32ull*1024*1024;
inline constexpr std::uint64_t source_shadow_archive_decoded_bytes=192ull*1024*1024;
inline constexpr std::uint64_t source_shadow_archive_native_bytes=160ull*1024*1024;
inline constexpr std::uint64_t source_shadow_archive_parser_bytes=1024ull*1024*1024;
inline constexpr std::uint64_t source_shadow_archive_staged_bytes=1536ull*1024*1024;
inline constexpr std::uint64_t source_shadow_archive_delta_bytes=768ull*1024*1024;
inline constexpr unsigned source_shadow_archive_acknowledged_bases=4;
inline constexpr unsigned source_shadow_archive_inflight=2;
inline constexpr unsigned source_shadow_archive_host_images=32;
inline constexpr unsigned source_shadow_archive_host_peers=16;
// Six distinct receiver images (four ACKed bases plus two successors).
// Accepted-body receipts share these immutable owners. Account the parser,
// weighted native staging, two encoded slots, three bounded asset banks,
// four ordinary 8 MiB snapshot bodies and fixed scaffolding separately.
inline constexpr std::uint64_t source_shadow_archive_receiver_working_bytes=4096ull*1024*1024;
static_assert((source_shadow_archive_acknowledged_bases+source_shadow_archive_inflight)*source_shadow_archive_decoded_bytes+
    source_shadow_archive_parser_bytes+source_shadow_archive_staged_bytes+
    2*source_shadow_archive_encoded_bytes+3*32ull*1024*1024+4*8ull*1024*1024+
    64ull*1024*1024<=source_shadow_archive_receiver_working_bytes);
// Sender retention is capped across all peers, rather than granting every
// connection another full cache. Each peer has at most two wire cuts and one
// queued frame; the one worker additionally owns one complete checkpoint.
inline constexpr std::uint64_t source_shadow_archive_host_working_bytes=9216ull*1024*1024;
static_assert(source_shadow_archive_host_images*source_shadow_archive_decoded_bytes+
    source_shadow_archive_host_peers*3*(source_shadow_archive_encoded_bytes+8ull*1024*1024+16384)+
    source_shadow_archive_native_bytes+source_shadow_archive_delta_bytes+64ull*1024*1024<=
    source_shadow_archive_host_working_bytes);
}
