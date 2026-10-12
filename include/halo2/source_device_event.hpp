#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace halo2 {
enum class SourceDeviceEventKind { Ignore, Sound, Effect };
// Original107980's tag-group selection, shared by the native adapter.
SourceDeviceEventKind source_device_event_kind(std::uint32_t tag,std::uint32_t group);
struct SourceDeviceSoundRequest {
    std::uint32_t tag{},object{},marker=~0u,gain_bits=0x3f800000u;
    // Literal vectors referenced by original468788/4687A8. Original189060
    // attaches this request to object; position is not an invented world point.
    std::array<std::uint32_t,3> position{},direction{0x3f800000u,0,0};
};
struct SourceDeviceEffectRequest {
    std::uint32_t tag{},object{},power_bits{},position_bits{};
    std::array<std::byte,10> location{}; // object+C4, +C8, then word+C2.
    std::uint32_t argument_b{},argument_c{};
};
struct SourceDeviceEventServices {
    std::function<std::span<const std::byte>(std::uint32_t)> object;
    std::function<std::uint32_t(std::uint32_t)> tag_group;
    // Mandatory actual189060 /176780 owners at their respective boundaries.
    std::function<void(const SourceDeviceSoundRequest &)> sound;
    std::function<void(const SourceDeviceEffectRequest &)> effect;
};
// Complete107980 through its two external creation services. NONE bypasses
// all services. This function does not supply sound/effect creation success.
void dispatch_source_device_event(std::uint32_t tag,std::uint32_t object,
    const SourceDeviceEventServices &);
}
