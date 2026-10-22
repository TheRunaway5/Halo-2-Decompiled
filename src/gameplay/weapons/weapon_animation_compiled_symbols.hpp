#pragma once
#include "halo2/cache.hpp"
#include <array>
#include <string_view>

namespace halo2::detail {
struct WeaponAnimationCompiledSymbol {std::uint32_t original,mcc;std::string_view name;};
// Exact unique-name join retained by mcc_frontend_string_ids.inc. Only compiled
// first-person/body animation activity and dual-class selectors use this boundary.
// Authored graph/class/type IDs and the canonical source clock stay unchanged.
inline constexpr std::array weapon_animation_compiled_symbols{
    WeaponAnimationCompiledSymbol{0x0400000cu,0x0400000cu,"idle"},
    WeaponAnimationCompiledSymbol{0x06000006u,0x06000006u,"fire_1"},
    WeaponAnimationCompiledSymbol{0x05000024u,0x05000024u,"ready"},
    WeaponAnimationCompiledSymbol{0x0c000064u,0x0c000064u,"reload_empty"},
    WeaponAnimationCompiledSymbol{0x0b000065u,0x0b000065u,"reload_full"},
    WeaponAnimationCompiledSymbol{0x0d000021u,0x0d000021u,"throw_grenade"},
    WeaponAnimationCompiledSymbol{0x0e000607u,0x0e000613u,"melee_strike_1"},
    WeaponAnimationCompiledSymbol{0x0e000608u,0x0e000614u,"melee_strike_2"},
    WeaponAnimationCompiledSymbol{0x0e000609u,0x0e000615u,"melee_strike_3"},
    WeaponAnimationCompiledSymbol{0x0e00060au,0x0e000616u,"melee_strike_4"},
    WeaponAnimationCompiledSymbol{0x0c000073u,0x0c000073u,"melee_1sthit"},
    WeaponAnimationCompiledSymbol{0x0c000075u,0x0c000075u,"melee_2ndhit"},
    WeaponAnimationCompiledSymbol{0x0c000077u,0x0c000077u,"melee_3rdhit"},
    WeaponAnimationCompiledSymbol{0x11000074u,0x11000074u,"melee_1sthit2idle"},
    WeaponAnimationCompiledSymbol{0x11000076u,0x11000076u,"melee_2ndhit2idle"},
    WeaponAnimationCompiledSymbol{0x0a0005bbu,0x0a0005c7u,"melee_dash"},
    WeaponAnimationCompiledSymbol{0x0b0005b2u,0x0b0005beu,"melee_lunge"},
    WeaponAnimationCompiledSymbol{0x130005bcu,0x130005c8u,"melee_dash_airborne"},
    WeaponAnimationCompiledSymbol{0x140005b3u,0x140005bfu,"melee_lunge_airborne"},
    WeaponAnimationCompiledSymbol{0x0500000au,0x0500000au,"melee"},
    WeaponAnimationCompiledSymbol{0x0b00006du,0x0b00006du,"overheating"},
    WeaponAnimationCompiledSymbol{0x0a000066u,0x0a000066u,"overheated"},
    WeaponAnimationCompiledSymbol{0x08000071u,0x08000071u,"o_h_exit"},
    WeaponAnimationCompiledSymbol{0x0c0005aeu,0x0c0005bau,"reload_enter"},
    WeaponAnimationCompiledSymbol{0x150005afu,0x150005bbu,"reload_continue_empty"},
    WeaponAnimationCompiledSymbol{0x140005b0u,0x140005bcu,"reload_continue_full"},
    WeaponAnimationCompiledSymbol{0x0b0005b1u,0x0b0005bdu,"reload_exit"},
    WeaponAnimationCompiledSymbol{0x08000025u,0x08000025u,"put_away"},
    WeaponAnimationCompiledSymbol{0x06000007u,0x06000007u,"fire_2"},
    WeaponAnimationCompiledSymbol{0x0900006au,0x0900006au,"misfire_1"},
    WeaponAnimationCompiledSymbol{0x0900006bu,0x0900006bu,"misfire_2"},
    WeaponAnimationCompiledSymbol{0x0b000067u,0x0b000067u,"overcharged"},
    WeaponAnimationCompiledSymbol{0x08000063u,0x08000063u,"light_on"},
    WeaponAnimationCompiledSymbol{0x09000062u,0x09000062u,"light_off"},
    WeaponAnimationCompiledSymbol{0x1000006cu,0x1000006cu,"throw_overheated"},
    WeaponAnimationCompiledSymbol{0x1000006eu,0x1000006eu,"ready_overheated"},
    WeaponAnimationCompiledSymbol{0x030000d9u,0x030000d9u,"any"},
    WeaponAnimationCompiledSymbol{0x0400054bu,0x04000557u,"dual"},
    WeaponAnimationCompiledSymbol{0x0a000069u,0x0a000069u,"ammunition"},
    WeaponAnimationCompiledSymbol{0x0b00054au,0x0b000556u,"arm_offsets"},
    WeaponAnimationCompiledSymbol{0x0400060bu,0x04000617u,"grip"},
    WeaponAnimationCompiledSymbol{0x0b000027u,0x0b000027u,"aim_move_up"},
    WeaponAnimationCompiledSymbol{0x0c000026u,0x0c000026u,"aim_still_up"},
    WeaponAnimationCompiledSymbol{0x0d000029u,0x0d000029u,"aim_move_down"},
    WeaponAnimationCompiledSymbol{0x0e000028u,0x0e000028u,"aim_still_down"},
    // Compiled body requests stay outside the canonical first-person prefix.
    WeaponAnimationCompiledSymbol{0x08000002u,0x08000002u,"reload_1"},
    WeaponAnimationCompiledSymbol{0x0b00053eu,0x0b00054au,"fire_1_left"},
    WeaponAnimationCompiledSymbol{0x0d00053au,0x0d000546u,"reload_1_left"},
    WeaponAnimationCompiledSymbol{0x0a000014u,0x0a000014u,"move_front"},
    // Published native clocks may request posing on the body bank. Resolve
    // its exact authored name; an absent body action remains an omission.
    WeaponAnimationCompiledSymbol{0x0600005fu,0x0600005fu,"posing"},
};
inline constexpr std::size_t first_person_compiled_activity_count=36;
static_assert([] {
    for(const auto &symbol:weapon_animation_compiled_symbols)
        if((symbol.original>>24)!=symbol.name.size() || (symbol.mcc>>24)!=symbol.name.size())return false;
    return true;
}(),"Compiled animation selectors must retain their complete named StringID words");
class WeaponAnimationCompiledSymbols {
public:
    explicit WeaponAnimationCompiledSymbols(const Cache &cache):mcc_(cache.is_mcc()) {
        if(mcc_)for(const auto &symbol:weapon_animation_compiled_symbols)
            if(cache.string_id(symbol.mcc)!=symbol.name)
                throw FormatError("MCC weapon animation selector differs from its authenticated name join");
    }
    std::uint32_t authored(std::uint32_t original) const {
        for(const auto &symbol:weapon_animation_compiled_symbols)if(symbol.original==original)
            return mcc_?symbol.mcc:original;
        throw FormatError("Unsupported compiled weapon animation selector");
    }
private:
    bool mcc_;
};
}
