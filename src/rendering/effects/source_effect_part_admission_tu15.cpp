#include "halo2/source_effect_part_admission_tu15.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
bool source_effect_part_reference_admitted_tu15(std::int16_t location,
    std::uint32_t count,std::uint32_t reference)noexcept {
    return location>=0 && location<std::bit_cast<std::int32_t>(count) && reference!=0xffffffffu;
}
bool source_effect_part_selector_admitted_tu15(std::uint16_t selector,
    std::uint16_t flags)noexcept {
    return selector!=((flags&0x20u)?1u:2u);
}
std::optional<SourceEffectSoundPartTu15> source_effect_sound_part_tu15(
    std::span<const std::byte> part,std::uint32_t count) {
    if(part.size()!=0x38)throw std::invalid_argument("Effect sound requires its complete authored part");
    const auto word=[&](unsigned at,unsigned bytes=4){std::uint32_t v{};for(unsigned i=0;i<bytes;++i)v|=std::to_integer<std::uint32_t>(part[at+i])<<(i*8u);return v;};
    if(word(8)!=0x736e6421u)return {};
    const auto location=std::bit_cast<std::int16_t>(std::uint16_t(word(4,2)));
    const auto sound=word(0x10);
    if(!source_effect_part_reference_admitted_tu15(location,count,sound))return {};
    return SourceEffectSoundPartTu15{sound,word(0x30),word(0x34),location,std::uint16_t(word(2,2))};
}
}
