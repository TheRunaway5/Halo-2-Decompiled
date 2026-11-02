#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>
namespace halo2 {
class MultiplayerAudio;
struct MultiplayerState;
// Actual source request, or the committed CABC0 reset projected through its
// authored sound selector before applying the recipient's local-player gate.
struct NativeWeaponZoomAudioEventTu15 {
    std::uint64_t serial{};
    std::uint32_t tick{},unit=0xffffffffu,player=0xffffffffu,sound=0xffffffffu,control_bits{};
    bool forced_reset{};
    std::array<std::byte,160> request{};
    bool operator==(const NativeWeaponZoomAudioEventTu15 &)const=default;
};
struct NativeWeaponZoomAudioCursorTu15 {
    std::uint64_t serial{},missing_prefix{};
    bool baselined{};
};
struct NativeWeaponZoomAudioPendingTu15 {
    NativeWeaponZoomAudioEventTu15 event;
    unsigned view{};
};
void validate_native_weapon_zoom_audio_tu15(std::span<const NativeWeaponZoomAudioEventTu15>,std::uint32_t tick);
void collect_native_weapon_zoom_audio_tu15(const MultiplayerState &,std::span<const unsigned> local_slots,
    NativeWeaponZoomAudioCursorTu15 &,std::vector<NativeWeaponZoomAudioPendingTu15> &,bool enabled=true);
std::size_t deliver_native_weapon_zoom_audio_tu15(MultiplayerAudio &,std::vector<NativeWeaponZoomAudioPendingTu15> &);
}
