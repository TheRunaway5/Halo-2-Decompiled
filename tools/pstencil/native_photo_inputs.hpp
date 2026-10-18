#pragma once
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/damage_feedback.hpp"
#include "halo2/source_netgame_weapon_scene.hpp"
#include <bit>

namespace halo2::pstencil {
// Fresh native photo actors have no cinematic interpolated-function overrides.
// Borrow their actual BCC20 vitality/recent-damage inputs and evaluate their
// authored BAB40 functions, exactly as SourceCharacterShaderInputs does in game.
// Authored outputs (including elite_burnt) are evaluated instead of guessed
// from their names. No source texture or object state is changed.
inline ShaderInput native_photo_inputs(const Cache& cache,PhysicsLiveWorldTu15& world,
    std::uint32_t object,std::uint32_t definition) {
    const Bytes body(world.memory().view(world.source_object_body(object),0x10au));
    const auto health=body.f32(0xec),shield=body.f32(0xf0);
    if(!(health>0))throw FormatError("photo shader inputs require their freshly born living actor");
    const DamageFeedbackState feedback{
        {body.f32(0xf4),body.f32(0xfc),std::bit_cast<std::int8_t>(std::uint8_t(body.u16(0x108)))},
        {body.f32(0xf8),body.f32(0x100),std::bit_cast<std::int8_t>(std::uint8_t(body.u16(0x108)>>8))}};
    return [functions=cache.load_object_functions(definition),tables=cache.shader_function_tables(),
            feedback,health,shield](std::uint32_t name) {
        return damage_feedback_shader_input(feedback,health,shield,false,functions,*tables,name,0);
    };
}
// Same SourceWorldWeaponShaderInput used by the native pickup scene. Snapshot
// the actual FF5F0 fields and signed magazine shorts; never borrow unit vitality.
inline ShaderInput native_weapon_photo_inputs(const Cache& cache,PhysicsLiveWorldTu15& world,
    std::uint32_t object,std::uint32_t definition) {
    const auto address=world.source_object_body(object);
    const Bytes body(world.memory().view(address,0x188u));
    const SourceWeaponShaderState state{body.u32(0x12c),body.u32(0x16c),body.f32(0x184),body.f32(0x180)};
    auto authored=cache.load_source_world_weapon_definition(definition);
    std::vector<SourceWeaponMagazineState> magazines;
    for(unsigned n=0;n<authored.magazines.size();++n) {
        const Bytes magazine(world.memory().view(address+0x22au+16u*n,4));
        magazines.push_back({std::bit_cast<std::int16_t>(magazine.u16(2)),
            std::bit_cast<std::int16_t>(magazine.u16(0))});
    }
    return [state,authored=std::move(authored),magazines=std::move(magazines),
            tables=cache.shader_function_tables()](std::uint32_t name) {
        return SourceWorldWeaponShaderInput{state,authored.magazines,magazines,authored.functions,tables.get()}
            .value(name).value_or(0.f);
    };
}
}
