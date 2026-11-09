#pragma once
#include "halo2/multiplayer.hpp"
#include <bit>

namespace halo2::detail {
inline bool native_nonplaying_member_candidate(const MultiplayerState::Player& player) {
    return player.source_membership==-1 && player.dead && player.source_unit==null_datum &&
        player.source_weapon==null_datum && player.source_configuration &&
        (*player.source_configuration)[0x7c]==0xffu;
}

inline void validate_native_source_membership(const MultiplayerState& state,
    const MultiplayerState::Player& player) {
    if(!player.connected)return;
    if(player.source_membership==-1) {
        // Original14E7B0/14E9E0 accepts NONE participation, and15B470
        // refuses its native birth. This is an absent actor, not a playing
        // member outside the retained team mask. Require the complete native
        // WORLD contract after the decoder has read its authority bit.
        if(state.source_game_type!=2 || !state.source_body_poses_authoritative ||
            !native_nonplaying_member_candidate(player) ||
            player.secondary_weapon.datum!=null_datum || player.first_person_pose)
            throw FormatError("Invalid nonplaying native LAN source participation");
        return;
    }
    const auto member=player.source_membership;
    if((state.team_mode && (member<0 || member>7 || !(state.retained_team_mask&(1u<<unsigned(member))))) ||
        (!state.team_mode && (member<0 || member>=int(multiplayer_capacity))))
        throw FormatError("Invalid native LAN source team membership");
    if(player.source_configuration &&
        std::bit_cast<std::int8_t>((*player.source_configuration)[0x7c])!=member)
        throw FormatError("Native profile and live membership differ");
}
} // namespace halo2::detail
