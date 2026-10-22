#include "halo2/source_weapon_magazine.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <array>
#include <bit>

namespace halo2 {
SourceWeaponMagazineState source_weapon_magazine_initial_state(const SourceWeaponMagazineDefinition &d){
    const auto loaded=std::min(d.initial_rounds,d.maximum_loaded);
    const auto remaining=std::int32_t(d.initial_rounds)-std::int32_t(loaded);
    const auto reserve=std::min(remaining,std::int32_t(d.maximum_reserve));
    // FDBB3 stores the low16bits even when the signed difference exceeds a
    // short. Avoid implementation-defined signed narrowing.
    return {loaded,std::bit_cast<std::int16_t>(std::uint16_t(reserve))};
}
FunctionValue source_weapon_primary_ammunition(std::span<const SourceWeaponMagazineDefinition> d,
    std::span<const SourceWeaponMagazineState> s){
    if(d.size()!=s.size())throw FormatError("source weapon magazine state does not match its authored bank");
    float value=0;
    if(!d.empty() && d[0].maximum_loaded!=0){
        value=float(s[0].loaded)/float(d[0].maximum_loaded);
        if(value<0)value=0;
        else if(value>1)value=1;
    }
    return {value,value>0};
}
std::optional<FunctionValue> source_weapon_magazine_input(std::uint32_t name,
    std::span<const SourceWeaponMagazineDefinition> d,std::span<const SourceWeaponMagazineState> s) {
    if(name==0x1200056au)return source_weapon_primary_ammunition(d,s);
    if(name!=0x1700056bu && name!=0x1700056cu)return {};
    if(d.size()!=s.size())throw FormatError("source weapon magazine state does not match its authored bank");
    float value=0.f;
    if(!d.empty()) {
        const int loaded=s[0].loaded;
        // 104C38/104CB5 use signed division;104C9E..104CAD computes the
        // same truncating division by10 for the tens input. Preserve the
        // separate CVTSI2SS and MULSS against original46A178 (0x3DCCCCCD).
        const int digit=name==0x1700056bu?loaded%10:(loaded/10)%10;
        value=float(digit)*std::bit_cast<float>(0x3dcccccdu);
        if(value<0.f)value=0.f;
        else if(value>1.f)value=1.f;
    }
    return FunctionValue{value,value>0.f};
}
std::optional<FunctionValue> source_weapon_magazine_shader_input(std::uint32_t name,
    std::span<const SourceWeaponMagazineDefinition>d,std::span<const SourceWeaponMagazineState>s,
    std::span<const ObjectFunction>functions,const ShaderFunctionTables&tables){
    if(const auto direct=source_weapon_magazine_input(name,d,s))return direct;
    std::optional<FunctionValue> result;
    constexpr std::array<std::byte,20> identity{std::byte{0},std::byte{0},std::byte{0},std::byte{0},
        std::byte{0},std::byte{0},std::byte{0},std::byte{0},std::byte{0},std::byte{0},
        std::byte{0x80},std::byte{0x3f},std::byte{0},std::byte{0},std::byte{0},std::byte{0},
        std::byte{0},std::byte{0},std::byte{0},std::byte{0}};
    // BAB40 scans the entire bank; the last matching authored output wins.
    for(const auto&f:functions)if(f.output==name){
        if(f.flags || f.input!=0x1200056au || f.enable || f.threshold!=0 || f.scale ||
           !std::equal(f.curve.begin(),f.curve.end(),identity.begin(),identity.end()))
            throw FormatError("source world weapon function requires its unresolved original input/curve producer");
        const auto builtin=source_weapon_primary_ammunition(d,s);
        const float value=shader_function_value(f.curve,builtin.value,1.f,tables);
        result=FunctionValue{value,value>0};
    }
    return result;
}
} // namespace halo2
