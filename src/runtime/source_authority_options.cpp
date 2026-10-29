#include "halo2/source_authority_options.hpp"
#include "halo2/multiplayer.hpp"

namespace halo2 {
MultiplayerOptions source_authority_options(const MultiplayerOptions &input){
    MultiplayerOptions out;out.online_match=input.online_match;out.map=input.map;out.shared_maps=input.shared_maps;out.gameplay_xbe=input.gameplay_xbe;
    out.bsp=input.bsp;out.variant=input.variant;out.starting_weapon=input.starting_weapon;out.session_random_seed=input.session_random_seed;
    out.local_players=input.local_players;out.selected_teams=input.selected_teams;out.selected_profiles=input.selected_profiles;return out;
}
}
