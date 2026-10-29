#pragma once
#include <bit>
#include <cstdint>
namespace halo2 {
// Salted object-header lookup with the original signed extent/salt checks
// and x86 type-mask shift semantics. It returns actual retained body backing.
template<class State> std::uint32_t native_campaign_sound_object(State& state,std::uint32_t datum,std::uint32_t mask) {
    if(datum==0xffffffffu)return 0u;
    auto pool=state.objects();const auto index=datum&0xffffu;
    if(std::int32_t(index)>=pool.extent())return 0u;
    auto header=pool.indexed(index);const auto salt=header.salt();if(!salt)return 0u;
    if(std::int32_t(salt)!=(std::bit_cast<std::int32_t>(datum)>>16))return 0u;
    if(!header.present())return 0u;
    if(!(mask&(1u<<(header.type()&31u))))return 0u;
    return header.body();
}
// The original association owner performs a fresh fixed-stride object-header
// lookup; it does not reuse a preceding validation query's body pointer.
template<class State> void native_campaign_clear_object_sound(State& state,std::uint32_t object,std::uint32_t sound) {
    auto pool=state.objects();auto header=pool.direct(object&0xffffu);auto body=state.object(header.body());
    const auto offset=body.sound_offset();auto association=body.sound_association(offset);
    const auto current=association.sound();if(current==sound)association.clear();
}
// Complete ordinary/weapon descriptor stop notification. No provider,
// transform, PVS or device result is synthesized: this callback only clears
// an actual matching object sound association after its original gates.
template<class State> void native_campaign_sound_stop_notice(State& state,std::uint32_t object,std::uint32_t source) {
    if(!state.game_active())return;
    if(!native_campaign_sound_object(state,object,3u))return;
    auto sources=state.sources();auto row=sources.row(source);const auto sound=row.tag();
    native_campaign_clear_object_sound(state,object,sound);
}
} // namespace halo2
