#include "halo2/online_voice_protocol.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"

namespace halo2 {
std::optional<ParsedOnlineVoice> decode_online_voice_frame(
    std::span<const std::byte> packet,std::uint16_t protocol) {
    // std::byte and uint8_t are byte-sized; the real transport retains the
    // contiguous input for this synchronous call. CXX validates the shared
    // ABI and owns the Rust Vec's allocation/deallocation on its Rust side.
    static_assert(sizeof(std::byte)==sizeof(std::uint8_t));
    const auto *bytes=reinterpret_cast<const std::uint8_t *>(packet.data());
    const auto frame=online_protocol_bridge::decode_voice_frame(
        rust::Slice<const std::uint8_t>(bytes,packet.size()),protocol);
    if(frame.status!=online_protocol_bridge::DecodeStatus::Ok)return {};
    if(frame.subtype==1 || frame.subtype==2) {
        ParsedOnlineVoicePreferences value;
        if(frame.subtype==2)value.forwarded_source=frame.source;
        value.enabled=frame.enabled;value.muted=frame.muted;return value;
    }
    if(frame.subtype!=0)return {};
    ParsedOnlineVoiceAudio value;
    value.source=frame.source;value.sequence=frame.sequence;value.radio=frame.radio;
    value.left=frame.left;value.right=frame.right;
    value.opus.assign(frame.opus.begin(),frame.opus.end());return value;
}
}
