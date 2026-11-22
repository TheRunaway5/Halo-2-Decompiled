#pragma once

#include <array>
#include <cstdint>

namespace halo2 {
inline constexpr std::uint32_t native_tu15_attached_speech_descriptor_address=0x44ec34u;
inline constexpr std::array<std::uint32_t,8> native_tu15_attached_speech_descriptor{
    0x18f040u,0x18f420u,0x18f490u,0x18f600u,0x18f700u,0u,0u,0u};
inline constexpr std::uint32_t native_tu15_attached_speech_origin_address=0x474d04u;
inline constexpr std::uint32_t native_tu15_attached_speech_origin=0x44abecu;
} // namespace halo2
