#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace halo2 {
// D3D viewport words, including the exact binary32 min/max depth bits.
using SourceClearViewport = std::array<std::uint32_t,6>;
struct SourceViewClearState {
    std::array<std::byte,0x298> header{};
    std::uint8_t transition_byte{};
    std::uint16_t x87_control_word{};
};
struct SourceClearEvent {
    enum class Kind : std::uint32_t { bind_target, set_viewport, clear };
    Kind kind{};
    // Bind: target, depth-enable AL, second stack argument. Set viewport:
    // six raw viewport words. Clear: hardware mask, ARGB, depth bits, stencil
    // byte, followed by zeroes. A clear uses the currently bound resource.
    std::array<std::uint32_t,6> words{};
    bool operator==(const SourceClearEvent &) const = default;
};
// Original14B60/14AC0. The supplied viewport is the actual device viewport at
// entry, after the resource bind. No source target alias or extent is inferred.
std::vector<SourceClearEvent> source_clear_plan(std::uint32_t semantic_flags,
    std::uint32_t argb,std::uint32_t depth_bits,std::uint8_t stencil,
    std::uint8_t temporary_clear_byte,std::int32_t main_target,
    const SourceClearViewport &bound_viewport);
// Original132D0 through1339F only: target selection, bind and conditional
// clear. Supply the actual0x298 caller header and transition byte sampled at
// entry, plus the viewport produced by12FA0/14BC0. This does not implement
// those resource/projection services, the later view setup or their lifecycle.
std::vector<SourceClearEvent> source_view_initialization_plan(
    std::span<const std::byte> header,std::uint8_t transition_byte,
    std::uint16_t x87_control_word,const SourceClearViewport &bound_viewport);
}
