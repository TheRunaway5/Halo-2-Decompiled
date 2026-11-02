#include "halo2/source_sound_wet_voice_layout_tu15.hpp"
#include <stdexcept>
namespace halo2 {
SourceSoundWetVoiceLayoutTu15 source_sound_wet_voice_layout_tu15(std::uint32_t voice,
    const SoundSdkVoiceAttachmentBindings& bindings,const SoundSdkDeviceMemory& memory){
    const auto read=[&](std::uint32_t at,unsigned bytes=4){
        if(!memory)throw std::runtime_error("Original wet voice layout memory unavailable");
        const auto raw=memory(at,bytes);
        if(raw.size()!=bytes)throw std::runtime_error("Original wet voice layout extent unavailable");
        std::uint32_t value{};for(unsigned i=0;i<bytes;++i)value|=std::to_integer<std::uint32_t>(raw[i])<<(8*i);return value;
    };
    const auto format=read(voice+0x70u),source=read(voice+0x80u);
    if(read(format+0xcu,2)!=1u || read(format+0xeu,1)!=1u || read(format+0xfu,1)!=24u ||
       read(format+0x10u)!=48000u || read(format+0x14u)!=4u)
        throw std::invalid_argument("Wet voice is outside original mono48000/24-in-four-byte format");
    const auto flags=read(source+8u),bin=read(source+0xd4u),bytes=read(source+0xc0u);
    if(flags!=0x80100000u || bin<15u || bin>22u || bytes!=128u || !(read(voice+0x12u,2)&0x100u))
        throw std::invalid_argument("Original wet voice has no matching prepared source owner");
    const auto base=read(bindings.stream_samples_pointer_address),buffer=read(source+0xbcu);
    const auto offset=buffer-base,expected=(bin-11u)*128u;
    const auto cursor=read(voice+0x154u),hardware=read(voice+0x14u);
    if(!buffer || offset!=expected || cursor!=(0x7ff000u|(buffer&0xfffu)) ||
       (hardware&0xf81f0000u)!=0xe0000000u)
        throw std::invalid_argument("Original wet source/cursor/format binding differs");
    SourceSoundWetVoiceLayoutTu15 result;result.input_bin_=bin;result.byte_offset_=offset;
    result.byte_extent_=bytes;result.cursor_word_=cursor;result.hardware_format_=hardware;return result;
}
}
