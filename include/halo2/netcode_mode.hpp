#pragma once
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <string_view>

namespace halo2 {
// Host-selected netcode mode (docs/NETCODE_ENHANCED_GOAL.md). The host picks
// it before the match; snapshots (MultiplayerState::netcode_mode) and the
// System Link lobby state carry it to non-hosts so both sides agree.
// Original is exactly the 2004 result flow: no rewind owner is installed.
enum class NetcodeMode : std::uint8_t { original=0, balanced=1, full=2 };
inline constexpr NetcodeMode default_netcode_mode=NetcodeMode::balanced;

inline const char *netcode_mode_name(NetcodeMode mode) {
    switch(mode) {
    case NetcodeMode::original:return "original";
    case NetcodeMode::balanced:return "balanced";
    case NetcodeMode::full:return "full";
    }
    return "original";
}
inline std::optional<NetcodeMode> parse_netcode_mode(std::string_view text) {
    if(text=="original")return NetcodeMode::original;
    if(text=="balanced")return NetcodeMode::balanced;
    if(text=="full")return NetcodeMode::full;
    return std::nullopt;
}
inline std::optional<NetcodeMode> netcode_mode_from_wire(unsigned value) {
    if(value>2)return std::nullopt;
    return NetcodeMode(value);
}
// H2_NET_MODE=original|balanced|full overrides the host selection (tests and
// diagnostics). An unset or unknown value keeps the selected mode.
inline NetcodeMode resolve_netcode_mode(NetcodeMode selected) {
    const char *value=std::getenv("H2_NET_MODE");
    if(!value || !*value)return selected;
    return parse_netcode_mode(value).value_or(selected);
}

// Lag-compensated hitscan (item 2). Rewind caps in milliseconds; a shooter
// whose displayed time is older than the cap is rewound only to the cap and
// leads the remainder. Full is meant for low-latency groups.
inline constexpr std::uint32_t lag_compensation_balanced_cap_ms=100;
inline constexpr std::uint32_t lag_compensation_full_cap_ms=250;
// Balanced cover veto: a rewound hit stands only if the target was exposed to
// the shot origin at the present time or at a recorded tick no older than this.
inline constexpr std::uint32_t lag_compensation_cover_veto_ms=70;
inline constexpr std::uint32_t lag_compensation_cap_ms(NetcodeMode mode) {
    return mode==NetcodeMode::full?lag_compensation_full_cap_ms:
        mode==NetcodeMode::balanced?lag_compensation_balanced_cap_ms:0u;
}

// Displayed snapshot time carried by a non-host input (protocol 113): the low
// 16 bits of the older interpolation endpoint's host tick, and the blend toward
// the next tick in 1/255 steps (0..254). The host resolves the full tick as the
// latest tick at or before its present tick with those low bits.
struct NetcodeViewTime {
    std::uint16_t tick_low{};
    std::uint8_t fraction{};
    bool operator==(const NetcodeViewTime &)const=default;
};
inline constexpr std::uint8_t netcode_view_absent_fraction=255;
// Quantize a presentation time (older tick, newer tick, blend) for the wire.
inline NetcodeViewTime netcode_view_time(std::uint32_t older,std::uint32_t newer,float blend) {
    if(!(blend>=0.f))blend=0.f;
    if(blend>1.f)blend=1.f;
    const double span=newer>older?double(newer-older):0.0;
    const double viewed=double(older)+double(blend)*span;
    auto tick=std::uint32_t(viewed);
    auto steps=unsigned((viewed-double(tick))*255.0+0.5);
    if(steps>=255){++tick;steps=0;}
    return {std::uint16_t(tick&65535u),std::uint8_t(steps)};
}
// Full host tick of a view at the present tick, or nullopt if it cannot have
// been displayed yet (the low bits do not occur at or before present).
inline std::optional<std::uint32_t> netcode_view_tick(NetcodeViewTime view,std::uint32_t present) {
    const std::uint32_t candidate=(present&~std::uint32_t(65535u))|view.tick_low;
    if(candidate<=present)return candidate;
    if(present<65536u)return std::nullopt;
    return candidate-65536u;
}
} // namespace halo2
