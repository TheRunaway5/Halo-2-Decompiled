#include "halo2/source_effect_tag_layout_tu15.hpp"
#include "halo2/cache.hpp"
#include <algorithm>

namespace halo2 {namespace {
struct Fields {
    std::span<const std::byte> bytes;
    std::uint32_t word(unsigned at,unsigned count)const {
        if(at>bytes.size() || count>bytes.size()-at)
            throw FormatError("Source effect field escapes its complete payload");
        std::uint32_t result{};
        for(unsigned n=0;n<count;++n)result|=std::to_integer<std::uint32_t>(bytes[at+n])<<(n*8u);
        return result;
    }
    std::uint16_t u16(unsigned at)const{return std::uint16_t(word(at,2));}
    std::uint32_t u32(unsigned at)const{return word(at,4);}
};
void owner(const Tag& tag,std::span<const std::byte> payload,
    std::string_view group,std::size_t minimum) {
    if(tag.group!=group || tag.handle().is_null() || !tag.address || tag.name.empty() ||
       tag.size!=payload.size() || payload.size()<minimum ||
       std::uint64_t(tag.address)+payload.size()>0x100000000ull)
        throw FormatError("Source effect tag has no complete typed payload owner");
}
std::uint32_t bitmap_images(const SourceEffectTagInputTu15& input,unsigned reference) {
    owner(input.bitmap,input.bitmap_payload,"bitm",0x4c);
    const Fields root{input.definition_payload},bitmap{input.bitmap_payload};
    if(root.u32(reference)!=tag_groups::bitmap.value ||
       root.u32(reference+4)!=input.bitmap.datum)
        throw FormatError("Source effect bitmap reference lost its full salt or type");
    const auto count=bitmap.u32(0x44);
    if(!count)
        throw FormatError("Source effect bitmap has no authored images");
    return count;
}
}
SourceContrailTagLayoutTu15 qualify_source_contrail_tag_layout_tu15(
    const SourceEffectTagInputTu15& input,std::span<const std::byte> states) {
    owner(input.definition,input.definition_payload,"cont",0xf0);
    const Fields root{input.definition_payload};
    // SourceContrailScene consumes this original runtime bitmap-type field.
    // It does not interpret a PC shader/template or a secondary map as it.
    if(root.u16(0x60)!=1 || root.u32(0x8c)!=0xffffffffu)
        throw FormatError("Source contrail requires its ordinary primary bitmap path");
    if(root.u16(0x18)>5 || root.u16(0x66)>10 || root.u16(0x68)>2)
        throw FormatError("Source contrail render/blend/fade selector is unsupported");
    const auto count=root.u32(0xe8),address=root.u32(0xec);
    if(count>0x7fffffffu || std::uint64_t(count)*64u!=states.size() ||
       (count && (!address || std::uint64_t(address)+states.size()>0x100000000ull)))
        throw FormatError("Source contrail point-state block has a different count, stride or address extent");
    const auto images=bitmap_images(input,0x30);
    return {input.definition.handle(),input.bitmap.handle(),root.u16(0x18),
        root.u16(0x64),root.u16(0x66),root.u16(0x68),root.u16(0x6a),images,
        {std::int32_t(count),address}};
}
SourceDecalTagLayoutTu15 qualify_source_decal_tag_layout_tu15(
    const SourceEffectTagInputTu15& input) {
    owner(input.definition,input.definition_payload,"deca",0xac);
    const Fields root{input.definition_payload};
    if(root.u16(4)>5)
        throw FormatError("Source decal layer has no ordinary TU1.5 draw state");
    const auto next=root.u32(0xc);
    if(next!=0xffffffffu && root.u32(8)!=tag_groups::decal.value)
        throw FormatError("Source decal chain reference has a different authored type");
    const auto images=bitmap_images(input,0x88);
    if(images>128)
        throw FormatError("Source decal image count exceeds its signed retained-index range");
    return {input.definition.handle(),input.bitmap.handle(),{next},root.u16(4),images};
}
void qualify_source_effect_vertex_program_tu15(TagGroup group,
    unsigned classification,std::span<const std::byte> code,
    std::span<const std::uint16_t> semantics) {
    constexpr std::uint16_t decal[]{0,3},contrail[]{0,3,9,14};
    const bool trail=group==tag_groups::contrail;
    if((!trail && group!=tag_groups::decal) || classification!=0)
        throw FormatError("Source effect vertex program has a different group or classification");
    const std::span<const std::uint16_t> expected=trail?
        std::span<const std::uint16_t>(contrail):std::span<const std::uint16_t>(decal);
    std::uint64_t hash=14695981039346656037ull;
    for(auto byte:code)hash=(hash^std::to_integer<unsigned char>(byte))*1099511628211ull;
    // Literal programs already authenticated by the original generators:
    // effect436B/e51226f3..., decal212B/c655c93b... . No shader is executed here.
    if(code.size()!=(trail?436u:212u) ||
       hash!=(trail?0x9eb11be43eaff30aull:0x8809e493f32bc92eull) ||
       !std::ranges::equal(semantics,expected))
        throw FormatError("Source effect vertex code or declaration differs from the TU1.5 translation");
}
}
