#include "halo2/source_weapon_shader_input.hpp"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace halo2 {
std::int16_t source_weapon_ready_begin(SourceWeaponShaderState &s,float seconds,std::int16_t rate) {
    s.weapon_flags&=~0x20u; //1040F0 clears only the low-byte bit.
    const float rounded=std::nearbyint(float(rate)*seconds);
    // CVTSS2SI returns the indefinite integer for unordered/overflow inputs.
    const auto ticks=rounded>=-2147483648.f && rounded<2147483648.f
        ?std::int32_t(rounded):std::int32_t(-2147483647-1);
    return std::bit_cast<std::int16_t>(std::uint16_t(ticks));
}
void source_weapon_ready_tick(SourceWeaponShaderState &s,std::int16_t &countdown) {
    if(!(s.weapon_flags&0x20u)) {
        countdown=std::bit_cast<std::int16_t>(std::uint16_t(std::uint16_t(countdown)-1));
        if(countdown<1)s.weapon_flags|=0x20u;
    }
}
FunctionValue source_weapon_turned_on(const SourceWeaponShaderState &s) {
    const bool item_active=(s.item_flags&1u)!=0 && (s.item_flags&2u)==0;
    // The original ordered comparisons reject NaN and +infinity, retain
    // negative values/-infinity, and reject exactly one. Do not clamp the
    // actual runtime field before this source predicate.
    const bool on=(!item_active || (s.weapon_flags&0x20u)!=0) &&
                  s.power_fraction<=1.f && s.power_fraction!=1.f;
    return {on?1.f:0.f,on};
}
std::optional<float> source_weapon_shader_input(std::uint32_t name,
                                               const SourceWeaponShaderState &s) {
    if(name==0x09000568u)return source_weapon_turned_on(s).value;
    if(name==0x04000567u || name==0x03000577u) {
        float value=name==0x04000567u?s.heat:s.power_fraction;
        if(value<0.f)value=0.f;
        else if(value>1.f)value=1.f;
        return value;
    }
    return {};
}
FunctionValue source_weapon_primary_charge(std::uint8_t mode,std::int16_t ticks,
    float charging_seconds,float tick_seconds) {
    float value=0.f;
    if(mode==1) {
        const float divided=float(ticks)/charging_seconds;
        const float elapsed=divided*tick_seconds;
        value=1.f-elapsed;
    } else if(mode==2)value=1.f;
    if(value<0.f)value=0.f;
    else if(value>1.f)value=1.f;
    return {value,value>0.f};
}
FunctionValue source_weapon_firing_material(const SourceWeaponFiringMaterialState &s,unsigned barrel) {
    if(barrel>=s.values.size())throw std::out_of_range("Invalid original firing material barrel");
    float value=0.f;
    if(std::int32_t(barrel)<s.barrel_count &&
        std::bit_cast<std::int32_t>(s.current_tick-s.last_fired_tick)<=1)
        value=s.values[barrel];
    // Ordered COMISS branches retain NaN payloads and negative zero. Neither
    // an unsigned recency comparison nor std::clamp matches this callback.
    if(value<0.f)value=0.f;
    else if(value>1.f)value=1.f;
    return {value,value>0.f};
}
} // namespace halo2
