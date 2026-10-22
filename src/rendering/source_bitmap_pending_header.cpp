#include "halo2/source_bitmap_pending_header.hpp"
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
constexpr std::array<std::uint32_t,24> texture_formats{
    25,0,1,26,0xffffffff,0xffffffff,5,0xffffffff,2,4,7,6,
    0xffffffff,0xffffffff,12,14,15,11,11,0xffffffff,0xffffffff,0xffffffff,40,40};
constexpr std::array<std::uint32_t,24> linear_formats{
    31,19,27,32,0xffffffff,0xffffffff,17,0xffffffff,16,29,30,18,
    0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,
    0xffffffff,0xffffffff,0xffffffff,0xffffffff,23,23};
constexpr std::array<std::int16_t,24> format_bits{
    8,8,8,16,0,0,16,0,16,16,32,32,0,0,4,8,8,8,8,128,96,48,16,16};

std::int16_t narrow(std::int32_t value) {
    return std::bit_cast<std::int16_t>(std::uint16_t(value));
}
std::uint32_t source_log2(std::uint32_t value) {
    return value?std::bit_width(value)-1:0;
}
bool source_power_test(std::int32_t value) {
    return !(std::uint32_t(value)&std::uint32_t(value-1));
}
std::int16_t mipmap_count(std::int16_t width,std::int16_t height,
    std::int16_t depth,std::int16_t format,std::int16_t maximum) {
    std::int32_t result=0;
    if(source_power_test(width) && source_power_test(height) && source_power_test(depth)) {
        const auto largest=format>=14 && format<=16?
            std::max({std::int32_t(width)/4,std::int32_t(height)/4,std::int32_t(depth)}):
            std::max({std::int32_t(width),std::int32_t(height),std::int32_t(depth)});
        result=std::int32_t(source_log2(std::uint32_t(largest)));
    }
    return std::int16_t(std::min(result,std::int32_t(maximum)));
}
std::int32_t linear_pitch(const SourceBitmapPendingHeaderMetadata &metadata) {
    auto width=std::max(std::int32_t(metadata.width),1);
    if(metadata.flags&2)width+=(-width)&3;
    //1365A0 sign-extends DX after rounding;32765..32767 can wrap negative.
    return std::int32_t(format_bits[std::size_t(metadata.format)])*narrow(width)/8;
}
} // namespace

std::array<std::uint32_t,5> source_bitmap_pending_header(
    const SourceBitmapPendingHeaderMetadata &metadata,std::uint32_t partition,
    std::uint32_t source_level_address) {
    if(metadata.format<0 || metadata.format>=24 || partition>=3)
        throw std::invalid_argument("source bitmap pending header needs format0..23 and partition0..2");
    const auto format=std::size_t(metadata.format);
    std::uint32_t header_format{},header_size{};
    if(metadata.flags&0x10) {
        auto hardware_format=linear_formats[format];
        if((metadata.flags&0x20) && (metadata.format==10 || metadata.format==11))
            hardware_format=0x36;
        header_format=(hardware_format<<8)|0x10029;
        const auto pitch=linear_pitch(metadata);
        header_size=((std::uint32_t((pitch-1)/64)<<12)|
            std::uint32_t(std::int32_t(metadata.height)-1))<<12;
        header_size|=std::uint32_t(std::int32_t(metadata.width)-1);
    } else {
        const auto width=std::int32_t(metadata.width)>>partition;
        const auto height=std::int32_t(metadata.height)>>partition;
        const auto depth=std::int32_t(metadata.depth)>>partition;
        const auto levels=mipmap_count(narrow(width),narrow(height),narrow(depth),metadata.format,
            narrow(std::int32_t(metadata.mipmap_count)-std::int32_t(partition)));
        auto hardware_format=texture_formats[format];
        if((metadata.flags&0x20) && (metadata.format==10 || metadata.format==11))
            hardware_format=0x33;
        auto dimensions=(source_log2(std::uint32_t(depth))<<4)|source_log2(std::uint32_t(height));
        dimensions=(dimensions<<4)|source_log2(std::uint32_t(width));
        header_format=(dimensions<<12)|hardware_format;
        header_format=(header_format<<4)|(metadata.type==1?3u:2u);
        header_format=(header_format<<4)|(metadata.type==2?4u:0u);
        header_format|=(std::uint32_t(std::int32_t(levels))+1)<<16;
        header_format|=9;
    }
    return {0x40001,source_level_address&0x0fffffff,0,header_format,header_size};
}
} // namespace halo2
