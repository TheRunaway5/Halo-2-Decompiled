#include "halo2/vista_audio_framing.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <limits>

namespace halo2 {
namespace {
constexpr std::array<std::byte,16> audio_guid{
  std::byte{0x61},std::byte{0x75},std::byte{0x64},std::byte{0x73},
  std::byte{0},std::byte{0},std::byte{0x10},std::byte{0},
  std::byte{0x80},std::byte{0},std::byte{0},std::byte{0xaa},
  std::byte{0},std::byte{0x38},std::byte{0x9b},std::byte{0x71}};
constexpr std::array<std::byte,16> wave_guid{
  std::byte{0x81},std::byte{0x9f},std::byte{0x58},std::byte{0x05},
  std::byte{0x56},std::byte{0xc3},std::byte{0xce},std::byte{0x11},
  std::byte{0xbf},std::byte{0x01},std::byte{0},std::byte{0xaa},
  std::byte{0},std::byte{0x55},std::byte{0x59},std::byte{0x5a}};
std::uint16_t u16(std::span<const std::byte> s,std::size_t p) {
  return std::uint16_t(std::to_integer<unsigned>(s[p])|
                       (std::to_integer<unsigned>(s[p+1])<<8));
}
std::uint32_t u32(std::span<const std::byte> s,std::size_t p) {
  return std::uint32_t(u16(s,p))|(std::uint32_t(u16(s,p+2))<<16);
}
}
bool is_vista_serialized_audio(std::span<const std::byte> bytes) {
  return bytes.size()>=64 &&
    std::equal(audio_guid.begin(),audio_guid.end(),bytes.begin()) &&
    std::equal(wave_guid.begin(),wave_guid.end(),bytes.begin()+44);
}
VistaSerializedMedia parse_vista_serialized_media(std::span<const std::byte> bytes) {
  if(bytes.size()<64||bytes.size()>std::numeric_limits<std::uint32_t>::max())
    throw FormatError("invalid Vista serialized-media chunk size");
  const auto format_size=u32(bytes,60);
  if(format_size<18||format_size>256||format_size>bytes.size()-64)
    throw FormatError("invalid Vista serialized-media format length");
  const auto format=bytes.subspan(64,format_size);
  if(u16(format,16)!=format_size-18)
    throw FormatError("Vista serialized-media extra-data length mismatch");
  VistaSerializedMedia out;
  std::copy_n(bytes.begin(),16,out.major_type.begin());
  std::copy_n(bytes.begin()+16,16,out.subtype.begin());
  std::copy_n(bytes.begin()+44,16,out.format_type.begin());
  out.fixed_size_samples=u32(bytes,32);
  out.temporal_compression=u32(bytes,36);
  out.sample_size=u32(bytes,40);
  out.format=format;
  out.extra=format.subspan(18);
  out.format_tag=u16(format,0);
  out.channels=u16(format,2);
  out.sample_rate=u32(format,4);
  out.average_bytes_per_second=u32(format,8);
  out.block_align=u16(format,12);
  out.bits_per_sample=u16(format,14);
  const std::size_t payload_offset=64+format_size;
  out.payload=bytes.subspan(payload_offset);
  if(!out.sample_size&&!out.payload.empty())
    throw FormatError("Vista serialized-media zero packet size cannot progress");
  for(std::size_t at=payload_offset;at<bytes.size();) {
    const auto count=std::min<std::size_t>(out.sample_size,bytes.size()-at);
    out.packets.push_back({at,bytes.subspan(at,count)});
    at+=count;
  }
  return out;
}
} // namespace halo2
