#pragma once
#include "halo2/content.hpp"
#include "halo2/inventory.hpp"
#include "halo2/transform.hpp"
#include "halo2/source_weapon_shader_input.hpp"
#include "halo2/source_weapon_magazine.hpp"
#include <functional>
#include <optional>

namespace halo2 {
// Literal weap item/object metadata; this does not require the native combat
// implementation to support that weapon's firing or ammunition behavior.
struct SourceWorldWeaponDefinition {
    std::string name;
    std::uint32_t definition=0xffffffffu,model=0xffffffffu,render_model=0xffffffffu;
    std::uint32_t animation_graph=0xffffffffu,default_variant{},item_flags{};
    std::uint32_t collision_model=0xffffffffu;
    std::uint16_t object_flags{};
    float multiplayer_scale{},campaign_scale{};
    std::vector<std::uint32_t> widget_groups;
    std::uint32_t owner_function_count{}; // raw weap+104
    std::int32_t barrel_count{}; // raw signed weap+2D0, used by1046C0
    std::vector<SourceWeaponMagazineDefinition> magazines;
    std::vector<ObjectFunction> functions;
};
// B7930 with parentNONE/locationNULL, then15AAD0's literal row assignments.
// The entire source196-byte creation packet remains available for allocation.
struct SourceNetgameWeaponBirth {
    std::array<std::uint32_t,49> descriptor{};
    std::uint32_t definition=0xffffffffu,variant{};
    Vec3 position,forward,up;
    float object_scale=1;
    bool levitated{};
};
float source_world_weapon_scale(const SourceWorldWeaponDefinition &,int game_type,
                                std::uint32_t item_flags);
// Complete singleton class0/default stock option0 branch. Other collection
// choices/options are rejected instead of inventing a RNG or variant producer.
SourceNetgameWeaponBirth source_netgame_weapon_birth(const NetgameEquipment &,
    const SourceWorldWeaponDefinition &,int game_type,int weapon_option=0);

// Actual bound item's fields consumed by15AE70. position is its animated
// object+30 sphere center, not its authored row position or physics root.
struct SourceNetgameWeaponBound {
    std::uint32_t handle=0xffffffffu;
    Vec3 position;
    std::uint32_t item_flags{};
    std::uint8_t control10a{};
};
struct SourceNetgameWeaponStepInput {
    std::int32_t tick{};
    std::int16_t tick_rate=30;
    bool playback_mode{},engine_available=true;
    int original_engine=2,timer_option{};
};
//15A090: first admitted literal selector, orNONE. The engine callback's actual
// result is explicit and stays separate from the gameoptions engine number.
int source_netgame_equipment_selector(const NetgameEquipment &,bool engine_available,
                                     int original_engine);
//15AE70 complete weapon/type2 timer-start0/1 control; no vehicle services are
// used. True requests the original15AAD0 allocation stage. It may replace a
// bound weapon, which must be unbound before the allocation attempt.
bool source_netgame_weapon_step(NetgameWeaponState &,const NetgameEquipment &,
    unsigned row,const SourceNetgameWeaponStepInput &,
    const std::optional<SourceNetgameWeaponBound> &);
//15AD30 controls. The caller supplies each actual destruction event/service.
void source_netgame_weapon_reset(std::span<NetgameWeaponState>,bool playback_mode,
    std::uint32_t variant_flags,std::int16_t round,
    const std::function<void(std::uint32_t)> &destroy);
//15AAD0's unbind-before-allocation order and failed-allocation row state.
void source_netgame_weapon_allocate(NetgameWeaponState &,unsigned row,
    const SourceNetgameWeaponBirth &,
    const std::function<void(std::uint32_t)> &unbind,
    const std::function<std::uint32_t(const SourceNetgameWeaponBirth &,unsigned)> &allocate);

// Explicit10B4D0/10B850 inputs for the completely recovered non-spinning,
// unattached levitated branch. Unsupported contact/spin/expiry/alignment
// services are rejected. In this branch original source leaves the pose and
// velocity words untouched and returnsfalse (no item update work performed).
struct SourceLevitatedWeaponUpdate {
    std::uint32_t object_flags{},parent=0xffffffffu,item_flags{};
    std::uint32_t ignored_object=0xffffffffu,item_owner=0xffffffffu;
    std::uint8_t control_c1{};
    std::int16_t remaining_ticks{},collision_model=-1;
    std::uint32_t object_owner_c8=0xffffffffu;
};
bool source_levitated_weapon_update(const SourceWorldWeaponDefinition &,
                                   SourceLevitatedWeaponUpdate &);
//336F0 selector37 ->33850->33670: a type2 owner with parentNONE
//terminates before any unit camouflage read. Attached state needs the actual
//parent chain and remains outside this bounded world-item producer.
float source_unattached_weapon_camouflage(const SourceLevitatedWeaponUpdate &);
struct SourceNetgameWeaponInitialState {
    SourceLevitatedWeaponUpdate item;
    SourceWeaponShaderState shader;
    std::uint8_t control10a{};
    std::vector<SourceWeaponMagazineState> magazines;
};
// Zeroed BC280 storage, FDAD0/10B2C0 and the levitated15AAD0/B9B90
// suffix. Graph controllers and nonempty widget services remain separate.
SourceNetgameWeaponInitialState source_netgame_weapon_initial_state(
    const SourceWorldWeaponDefinition &,const SourceNetgameWeaponBirth &);
struct SourceNetgameWeaponRegistration {
    unsigned row{};
    SourceWorldWeaponDefinition definition;
    SourceNetgameWeaponBirth birth;
    Vec3 sphere_center;
    float sphere_radius{};
};
struct SourceNativeWorldWeapon {
    unsigned registration{},row{};
    std::uint32_t handle=0xffffffffu,created_tick{};
    bool row_bound{};
    SourceNetgameWeaponInitialState state;
};
} // namespace halo2
