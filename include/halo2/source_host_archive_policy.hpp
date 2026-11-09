#pragma once
#include "halo2/source_authority_codec.hpp"

namespace halo2 {
// The real native World requires its bank, arena files and Match/Simulation
// values together. Use the existing bounded native archive policy only after
// the authenticated peer advertises this capability. Unpaired archives and
// peers without it retain their original network limits.
inline SourceAuthorityCodecLimits source_host_archive_network_codec_limits(bool complete){
    return complete?source_shadow_archive_codec_limits():source_authority_network_codec_limits();
}
inline std::uint64_t source_host_archive_retained_bank_limit(bool complete){
    return complete?source_shadow_archive_native_bytes:authority_network_native_bytes;
}
// One global full-cut materialization reservation is shared by both policies.
// Paired cuts retain no canonical delta base. Account conservatively for the
// decoder, native staging, two wire slots, immutable assets, fixed scaffolding
// and the three optional retained endpoints within the existing native bound.
inline constexpr auto source_complete_host_archive_working_bytes=source_shadow_archive_receiver_working_bytes;
static_assert(source_shadow_archive_parser_bytes+source_shadow_archive_staged_bytes+
    2*source_shadow_archive_encoded_bytes+3*authority_network_asset_bytes+
    authority_network_fixed_bytes+3*source_shadow_archive_native_bytes<=source_complete_host_archive_working_bytes);
}
