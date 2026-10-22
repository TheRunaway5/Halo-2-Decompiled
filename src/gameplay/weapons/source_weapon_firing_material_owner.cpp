#include "halo2/source_weapon_firing_material_owner.hpp"
#include "halo2/cache.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/source_netgame_weapon.hpp"

namespace halo2 {
std::optional<SourceWeaponFiringMaterialState> source_weapon_firing_material_owner(
    MultiplayerObjectRuntime &world,std::uint32_t object,const SourceWorldWeaponDefinition &definition) {
    if(object==0xffffffffu || !world.owns_physics())return {};
    if(world.object_definition(object)!=definition.definition)
        throw FormatError("Firing material lost its full source weapon definition");
    const auto &memory=world.memory();
    const auto clock=Bytes(memory.view(0x51df54u,4)).u32(0);
    if(!clock)return {};
    const auto body=world.object_body(object);
    return SourceWeaponFiringMaterialState{definition.barrel_count,
        Bytes(memory.view(clock+8u,4)).u32(0),Bytes(memory.view(body+0x248u,4)).u32(0),
        {Bytes(memory.view(body+0x1b4u,4)).f32(0),Bytes(memory.view(body+0x1e8u,4)).f32(0)}};
}
}
