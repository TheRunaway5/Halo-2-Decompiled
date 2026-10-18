#include "halo2/mcc_sound_tag_layout.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <bit>

namespace halo2 {
namespace {
// Assembly fd14ef3c46135e9422966c6a64cd926fb0bd23fb Halo2MCC/Halo2
// snd!, ugh!, sncl. Actual TU1.5 sound_create/admission consume ugh +4,
// +c,+1c,+24,+2c,+3c,+44,+4c and sncl row stride 0x5c. 21ABD0/21A960
// consume 12-byte chunk records; MCC Cache::load_mcc_sound owns 16-byte
// language chunks and different DAT-file/reset-bit semantics.
template<std::size_t N> std::array<std::byte,N> copy(std::span<const std::byte> source) {
    Bytes(source).require(0,N);std::array<std::byte,N> out;
    std::copy_n(source.begin(),N,out.begin());return out;
}
template<class Out> void word(Out& out,unsigned at,std::uint32_t value) {
    Bytes(out).require(at,4);for(unsigned n=0;n<4;++n)out[at+n]=std::byte(value>>(8*n));
}
void descriptor(std::array<std::byte,0x58>& out,unsigned at,const TagBlock& block,
                std::uint32_t expected,std::uint32_t stride) {
    if(block.count<0 || std::uint32_t(block.count)!=expected ||
       (expected && (!block.address || block.address==0xffffffffu)) ||
       std::uint64_t(block.address)+std::uint64_t(expected)*stride>0x100000000ull)
        throw FormatError("Native sound resource descriptor differs from the selected owned table");
    word(out,at,std::uint32_t(block.count));word(out,at+4,block.address);
}
}
MccSoundLayoutRequirement::MccSoundLayoutRequirement(MccSoundLayoutNeed need,const char* message)
    :std::runtime_error(message),need_(need){}
std::array<std::byte,0x24> project_mcc_sound_globals(std::span<const std::byte> row) {
    Bytes(row).require(0,0x2c);return copy<0x24>(row.subspan(8));
}
bool MccSoundDefinitionLayout::original_adpcm_admission() const {
    return codec.compression==1 && codec.encoding!=2;
}
void MccSoundDefinitionLayout::require_original_duration_domain() const {
    if(codec.sample_rate>2 || codec.encoding>2 || codec.compression>4)
        throw MccSoundLayoutRequirement(MccSoundLayoutNeed::NativeCodecDuration,
            "MCC extended codec requires actual native PCM format/duration metadata");
}
MccSoundDefinitionLayout project_mcc_sound_definition(
    std::span<const std::byte> sound,std::span<const std::byte> codecs) {
    const Bytes source(sound);source.require(0,0x1c);
    if(codecs.size()%3 || codecs.size()/3>65536)throw FormatError("Invalid owning MCC sound codec bank");
    MccSoundDefinitionLayout out;
    out.definition=copy<0x14>(sound);out.mcc_extension=copy<8>(sound.subspan(0x14));
    // MCC inserts Legacy/Remastered route bits10/11 before the four
    // compiled gameplay predicates. Actual MCC769876/769826 select the
    // same class/playback minimum/maximum fields as TU1.5 127AF2/21F752;
    // MCC6E83CB and770BE2 select the original cone/pitch branches with
    // bits14/15 instead of original12/13. Route bits never enter that
    // original domain; the complete raw flags remain owned by Cache and
    // the runtime delivery sidecar.
    const auto flags=source.u16(0);
    std::uint16_t native_flags=flags&0x03ffu;
    constexpr std::array<std::pair<std::uint16_t,std::uint16_t>,4> compiled{{
        {0x1000u,0x0400u},{0x2000u,0x0800u},
        {0x4000u,0x1000u},{0x8000u,0x2000u}}};
    for(const auto [mcc,native]:compiled)if(flags&mcc)native_flags|=native;
    out.definition[0]=std::byte(native_flags);out.definition[1]=std::byte(native_flags>>8);
    out.codec_index=std::to_integer<std::uint8_t>(sound[3]);out.remastered_sound_index=source.u16(4);
    const auto at=std::size_t(out.codec_index)*3;
    Bytes(codecs).require(at,3);
    out.codec={std::to_integer<std::uint8_t>(codecs[at]),std::to_integer<std::uint8_t>(codecs[at+1]),
        std::to_integer<std::uint8_t>(codecs[at+2])};
    if(out.codec.sample_rate>3 || out.codec.encoding>3 || out.codec.compression>5)
        throw FormatError("Unknown MCC sound codec semantics");
    out.definition[3]=std::byte(out.codec.sample_rate);out.definition[4]=std::byte(out.codec.encoding);
    out.definition[5]=std::byte(out.codec.compression);
    return out;
}
std::vector<std::byte> project_mcc_sound_class_records(std::span<const std::byte> records) {
    if(records.size()%0x78 || records.size()/0x78>0x100000)
        throw FormatError("Invalid MCC sound-class record bank");
    const auto count=records.size()/0x78;std::vector<std::byte> out(count*0x5c);
    for(std::size_t n=0;n<count;++n)
        std::copy_n(records.begin()+std::ptrdiff_t(n*0x78),0x5c,out.begin()+std::ptrdiff_t(n*0x5c));
    return out;
}
std::array<std::byte,8> project_mcc_sound_class_root(std::span<const std::byte> root,std::uint32_t address) {
    auto out=copy<8>(root);const auto count=Bytes(root).u32(0);
    if(count>0x100000 || (count && (!address || address==0xffffffffu)) ||
       std::uint64_t(address)+std::uint64_t(count)*0x5c>0x100000000ull)
        throw FormatError("Native sound-class owner does not contain the projected records");
    word(out,4,address);return out;
}
std::array<std::byte,0x58> project_mcc_sound_gestalt_root(
    std::span<const std::byte> gestalt,std::span<const std::byte> language,
    const MccSoundResourceBlocks& resources) {
    const Bytes source(gestalt);source.require(0,0x78);const Bytes chunks(language);chunks.require(0,8);
    if(!resources.permutations)throw MccSoundLayoutRequirement(MccSoundLayoutNeed::NativePermutationOwner,
        "MCC localized permutations require a declared original-layout native owner");
    if(!resources.chunks)throw MccSoundLayoutRequirement(MccSoundLayoutNeed::NativeChunkOwner,
        "MCC DAT/language chunks require a declared native resource owner");
    if(!resources.extra_info)throw MccSoundLayoutRequirement(MccSoundLayoutNeed::NativeExtraInfoOwner,
        "MCC localized extra-info resources require a declared native owner");
    std::array<std::byte,0x58> out;
    constexpr std::array<std::pair<unsigned,unsigned>,8> common{{
        {0x08,0x00},{0x10,0x08},{0x18,0x10},{0x20,0x18},
        {0x28,0x20},{0x38,0x30},{0x58,0x38},{0x68,0x48}}};
    for(const auto [from,to]:common)
        std::copy_n(gestalt.begin()+from,8,out.begin()+to);
    descriptor(out,0x28,*resources.permutations,source.u32(0x30),16);
    descriptor(out,0x40,*resources.chunks,chunks.u32(0),12);
    descriptor(out,0x50,*resources.extra_info,source.u32(0x70),0x2c);
    return out;
}
std::array<std::byte,16> project_mcc_sound_permutation(
    std::span<const std::byte> permutation,std::span<const std::byte> localized,
    std::optional<std::uint32_t> encoded_extent) {
    Bytes(permutation).require(0,16);const Bytes info(localized);info.require(0,8);
    if(!encoded_extent)throw MccSoundLayoutRequirement(MccSoundLayoutNeed::NativeEncodedSampleExtent,
        "MCC localized permutation requires the checked encoded extent of its selected chunks");
    std::array<std::byte,16> out;
    std::copy_n(permutation.begin(),8,out.begin());word(out,8,*encoded_extent);
    std::copy_n(localized.begin()+4,4,out.begin()+12);return out;
}
MccSoundChunkLayout project_mcc_sound_chunk(std::span<const std::byte> chunk) {
    const Bytes source(chunk);source.require(0,16);
    if(source.u32(12)!=0xffffffffu)throw MccSoundLayoutRequirement(MccSoundLayoutNeed::ForeignRuntimeCacheHandle,
        "A bound MCC runtime cache index cannot enter a different native cache owner");
    MccSoundChunkLayout out;
    std::copy_n(chunk.begin(),8,out.chunk.begin());
    word(out.chunk,4,source.u32(4)&~0x20000000u);word(out.chunk,8,source.u32(12));
    out.decoded_bytes=source.u32(8);out.reset_after=(source.u32(4)&0x20000000u)!=0;
    return out;
}
} // namespace halo2
