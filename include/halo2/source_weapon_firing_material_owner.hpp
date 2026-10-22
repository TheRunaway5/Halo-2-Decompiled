#pragma once
#include "halo2/source_weapon_shader_input.hpp"

namespace halo2 {
class MultiplayerObjectRuntime;
struct SourceWorldWeaponDefinition;
// Read only the full weapon owner's actual fields and original source clock.
// Remote/unowned objects and an unpublished clock do not supply a producer.
std::optional<SourceWeaponFiringMaterialState> source_weapon_firing_material_owner(
    MultiplayerObjectRuntime &,std::uint32_t object,const SourceWorldWeaponDefinition &);
}
