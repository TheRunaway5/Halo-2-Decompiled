#include "halo2/source_script_effect.hpp"

namespace halo2 {
std::array<std::uint32_t,24> SourceScriptEffectRequest::parameters(std::uint32_t seeds_at)const {
    std::array<std::uint32_t,24> p{};
    p[0]=5; // attached + force; retained-repeat bit1 is clear.
    p[1]=definition;p[2]=object;
    p[3]=p[4]=0xffffffffu; // No damage owner.
    p[5]=0x0000ffffu; // Owner's signed-short final field, zeroed padding.
    p[6]=0x0000ffffu; // Signed-short region NONE in zeroed backing.
    p[7]=seeds_at;p[8]=2;
    p[12]=marker;
    p[16]=p[17]=0x3f800000u;
    p[21]=p[22]=0xff808080u;
    return p;
}
std::optional<SourceScriptEffectRequest> source_script_effect_new_on_object_marker(
    std::uint32_t definition,std::uint32_t object,std::uint32_t marker,
    const SourceScriptEffectMarkerLookup &lookup) {
    if(definition==0xffffffffu || object==0xffffffffu)return {};
    if(!lookup)throw FormatError("Script marker effect requires its current original marker owner");
    if(!lookup(object,marker).count)return {};
    //2AA380 checks the first lookup.1797C0 independently obtains its output
    //record and1791C0 uses that record's WORLD position, without testing count.
    const auto completed=lookup(object,marker);
    return SourceScriptEffectRequest{definition,object,marker,{{
        {completed.world.position,{0,0,-1},0x070000c0u},
        {completed.world.position,{0,0,1},0x020000cau}}}};
}
}
