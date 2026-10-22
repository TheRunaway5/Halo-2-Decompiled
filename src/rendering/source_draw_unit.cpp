#include "halo2/source_draw_unit.hpp"
#include "halo2/source_shader_lod.hpp"
#include <algorithm>
#include <bit>
#include <cstring>
#include <limits>

namespace halo2 {
namespace {
constexpr auto none=0xffffffffu;
float real(std::uint32_t word){return std::bit_cast<float>(word);}
std::int32_t signed_word(std::uint32_t word){return std::bit_cast<std::int32_t>(word);}
std::uint32_t selector(std::uint32_t flags){return (flags>>12)&0xffffff01u;}
std::span<std::byte> context(const SourceDrawUnitServices& services,std::uint32_t token) {
    if(!services.context)throw FormatError("source draw unit consumes absent render context service");
    auto bytes=services.context(token);
    if(bytes.size()<0x78)throw FormatError("source draw unit render context is truncated");
    return bytes;
}
std::uint8_t fade_byte(float value) {
    const float scaled=value*256.f; // Literal45DD5C, distinct from coverage.
    // CVTTSS2SI's indefinite result for NaN/outside the signed range clamps0.
    const auto truncated=scaled>=-2147483648.f && scaled<2147483648.f?
        std::int32_t(scaled):std::numeric_limits<std::int32_t>::min();
    return std::uint8_t(std::clamp(truncated,0,255));
}
}
void source_draw_units_begin(SourceDrawUnitState& state,std::uint32_t mode) {
    if(mode){state.draw_count=state.main_prefix;return;}
    state.main_prefix=state.draw_count=state.override_count=state.section_highwater=0;
    state.sections={};
}
std::int16_t source_draw_unit_append(SourceDrawUnitState& state,
    const SourceDrawUnitInput& in,const SourceDrawUnitContext& c,
    const SourceDrawUnitServices& services) {
    const auto ordinal=state.draw_count++;
    if(signed_word(ordinal)>=850 || in.geometry==none){--state.draw_count;return -1;}
    if(signed_word(ordinal)<0)throw FormatError("source draw unit ordinal precedes allocated bank");
    auto& u=state.units[ordinal].words;const auto& a=in.stack;
    u[1]=a[1];u[2]=a[2];
    u[0]=(u[0]&0xe0000000u)|(a[0]&0x1fffffu)|((in.distance_byte&255u)<<21);
    u[6]=(u[6]&0xff00ffffu)|((a[6]&255u)<<16);
    u[3]=(u[3]&0xe0000000u)|((((in.instance&0x7ffu)<<9)|(in.geometry&0x1ffu))<<9)|((a[10]&31u)<<4)|(a[3]&15u);
    u[4]=a[4];u[7]=a[7];
    u[6]=(u[6]&0xf0ffffffu)|((a[18]&15u)<<24);
    const auto section=(u[3]>>4)&31u;
    if(section!=31) {
        if(state.section_highwater<=section+1)state.section_highwater=section+1;
        auto& s=state.sections[section];s.kind=a[11];
        if(a[8]!=none) {
            if(!services.center)throw FormatError("source draw unit consumes absent section center");
            s.depth=real(a[17]);s.center=services.center(a[19]);
            constexpr float maximum=std::numeric_limits<float>::max();
            if((a[12]&255u) || c.force_outer)s.rectangle={-maximum,maximum,-maximum,maximum};
            else if((a[13]&255u) || c.force_inner)s.rectangle={maximum,-maximum,maximum,-maximum};
            else {
                if(!services.rectangle)throw FormatError("source draw unit consumes absent30290 rectangle");
                s.rectangle=services.rectangle(a[19],real(a[20]));
            }
        }
    }
    u[6]=(u[6]&0xffff0000u)|(a[5]&65535u);
    state.override_slots[ordinal]=255;
    if(a[8]!=none && signed_word(state.override_count)<192) {
        if(signed_word(state.override_count)<0)throw FormatError("source draw unit override cursor precedes bank");
        state.override_slots[ordinal]=std::uint8_t(state.override_count);
        state.overrides[state.override_count++]=a[8];
    }
    state.coverage[ordinal]=source_shader_coverage_byte(real(a[9]));
    if(u[4] && a[8]!=none) {
        auto bytes=context(services,u[4]);
        bytes[(a[14]&255u)?0x76:0x77]=std::byte(fade_byte(real(a[(a[14]&255u)?15:16])));
    }
    if(!services.geometry)throw FormatError("source draw unit consumes absent44940 metadata");
    const auto geometry=services.geometry(std::uint16_t(ordinal),selector(a[0]),state.units[ordinal]);
    u[3]=(u[3]&0x1fffffffu)|(std::uint32_t(geometry.classification)<<29);
    std::uint32_t owner=none;
    if(u[4]){const auto bytes=context(services,u[4]);std::memcpy(&owner,bytes.data(),4);}
    if(!services.lighting)throw FormatError("source draw unit consumes absent2D000 lighting classification");
    const auto lighting=services.lighting(owner,u[1],(u[3]>>18)&0x7ffu);
    u[0]=(u[0]&0x1fffffffu)|(lighting<<29);
    if(!services.resident)throw FormatError("source draw unit consumes absent449E0 resource admission");
    if(!services.resident(std::uint16_t(ordinal),selector(u[0]),u[1]==none,state.units[ordinal])) {
        --state.draw_count;return -1;
    }
    if(!c.mode)++state.main_prefix;
    if(u[0]&0x40u) {
        const auto current=services.geometry(std::uint16_t(ordinal),selector(u[0]),state.units[ordinal]);
        u[5]=0;
        if(current.subpart_count) {
            if(!c.mask_arena_address)throw FormatError("source draw unit consumes absent mask arena identity");
            const auto offset=state.arena.allocate(current.subpart_count);
            if(offset)u[5]=c.mask_arena_address+4 * *offset;
        }
        if(!u[5])u[0]=(u[0]&~0x40u)|1u;
    }
    return std::int16_t(ordinal);
}
}
