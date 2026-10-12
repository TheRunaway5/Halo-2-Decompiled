#pragma once
#include "halo2/vista_session_birth.hpp"
#include <string_view>

namespace halo2 {
// Supplied Vista5C8D6F/5C903B, bounded to local states5..8. These are real
// selected-parameter setters, including source host admission and revision
// writes. They do not mark a map ready or start a world. Remote3/4 transport
// is outside this local binding and rejects before mutation.
// The first map argument is the campaign ID. Original617138(2)->6169C5
// passes NONE through61557F for a multiplayer map; it is not the cache type.
bool vista_session_select_map(std::span<std::byte>,std::uint32_t campaign_id,
    std::uint32_t map_id,std::u16string_view source_path);
// Null or kind+44==0 selects the source's fully zeroed304-byte variant.
bool vista_session_select_variant(std::span<std::byte>,
    const std::array<std::byte,0x130>* actual_pc_variant);
// Literal615EA9, including5AD736's enabled/nonzero-state current-session
// predicate and5AD782's distinct signed lifecycle-kind1..5 parameter gate.
// The caller supplies its actual retained source session and lifecycle owner.
// Each consumed field leaf retains state3..8 admission; unsupported or absent
// source values return NONE rather than selecting a desired frontend pane.
std::int32_t vista_session_frontend_protocol(std::span<const std::byte>,
    bool lifecycle_enabled,std::int32_t lifecycle_kind);
} // namespace halo2
