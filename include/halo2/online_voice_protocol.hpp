#pragma once
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <variant>
#include <vector>

namespace halo2 {
struct ParsedOnlineVoicePreferences {
    // Empty means use the authenticated transport peer. A forwarded source
    // remains an untrusted claim requiring current-host authorization.
    std::optional<unsigned> forwarded_source;
    bool enabled{};
    std::uint16_t muted{};
};
struct ParsedOnlineVoiceAudio {
    unsigned source{};
    std::uint32_t sequence{};
    bool radio{};
    float left{},right{};
    std::vector<std::uint8_t> opus;
};
using ParsedOnlineVoice=std::variant<ParsedOnlineVoicePreferences,ParsedOnlineVoiceAudio>;

// Owns all returned values. The input span is borrowed only for this call;
// no callbacks, device work, native owners or packet buffers cross back.
// Success proves syntax only. The caller still checks the authenticated
// peer, seat ownership, current epoch/role, mute/block and delivery generation.
// Malformed input fails without a fallback decoder. Allocation failure may
// throw on the C++ side and follows the existing receive-loop failure policy.
std::optional<ParsedOnlineVoice> decode_online_voice_frame(
    std::span<const std::byte>,std::uint16_t expected_protocol);
}

