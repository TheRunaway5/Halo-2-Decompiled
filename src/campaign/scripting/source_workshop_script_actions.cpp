#include "halo2/source_workshop_script_actions.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/cache.hpp"
#include <array>
#include <bit>
namespace halo2 { namespace {
std::uint32_t word(const SourcePhysicsContactMemory &memory,std::uint32_t at,unsigned width=4) {
    std::array<std::byte,4> bytes{};memory.read(at,std::span(bytes).first(width));
    std::uint32_t value{};for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8);
    return value;
}
}
CinematicBloomOverride source_cinematic_bloom_state(const SourcePhysicsContactMemory &memory) {
    const auto bank=word(memory,0x515ec4);
    if(!bank)throw FormatError("Cinematic bloom has no retained original state bank");
    CinematicBloomOverride out;out.enabled=word(memory,bank,1)!=0;
    out.blur_amount=std::bit_cast<float>(word(memory,bank+4));
    out.threshold=std::bit_cast<float>(word(memory,bank+8));
    out.brightness=std::bit_cast<float>(word(memory,bank+12));
    out.box_factor=std::bit_cast<float>(word(memory,bank+16));
    out.silver_bullet=word(memory,bank+24,1)!=0;return out;
}
void source_cinematic_bloom_set(PhysicsLiveWorldTu15 &world,std::string_view name,std::uint32_t value) {
    const auto bank=word(world.memory(),0x515ec4);
    if(!bank)throw FormatError("Cinematic bloom has no retained original state bank");
    unsigned offset{},width=4;
    if(name=="rasterizer_bloom_override")width=1;
    else if(name=="rasterizer_bloom_override_blur_amount")offset=4;
    else if(name=="rasterizer_bloom_override_threshold")offset=8;
    else if(name=="rasterizer_bloom_override_brightness")offset=12;
    else if(name=="rasterizer_bloom_override_box_factor")offset=16;
    else if(name=="rasterizer_bloom_override_silver_bullet"){offset=24;width=1;}
    else throw FormatError("Unknown cinematic bloom state field");
    std::array<std::byte,4> bytes{};for(unsigned n=0;n<width;++n)bytes[n]=std::byte(value>>(8*n));
    world.write_source_call_input(bank+offset,std::span(bytes).first(width));
}
void source_object_cinematic_collision(PhysicsLiveWorldTu15 &world,std::uint32_t object,bool enabled) {
    if(object!=0xffffffffu)(void)world.source_object_body(object);
    auto abi=world.prepare_source_call(std::array{std::uint32_t(enabled)});
    abi.gp[0]=object;world.run_actual(0xbe130,abi);
}
void source_ai_force_active(PhysicsLiveWorldTu15 &world,std::uint32_t reference,bool enabled) {
    auto abi=world.prepare_source_call(std::array{std::uint32_t(enabled)});
    abi.gp[0]=reference;world.run_actual(0x27f760,abi);
}
void source_unit_set_enterable_by_player(PhysicsLiveWorldTu15 &world,std::uint32_t unit,bool enabled) {
    if(unit!=0xffffffffu)(void)world.source_object_body(unit);
    auto abi=world.prepare_source_call(std::array{std::uint32_t(enabled)});
    abi.gp[0]=unit;world.run_actual(0x11db00,abi);
}
void source_object_cannot_die(PhysicsLiveWorldTu15 &world,std::uint32_t object,bool enabled) {
    if(object==0xffffffffu)return;
    // Original2ABCA0 inlines this action between argument parsing and
    // completion: preserve every other vitality flag, including invulnerability.
    const auto flag=world.source_object_body(object)+0x10bu;
    const auto before=word(world.memory(),flag,1);
    const std::array value{std::byte(enabled?(before|0x40u):(before&~0x40u))};
    world.write_source_call_input(flag,value);
}
}
