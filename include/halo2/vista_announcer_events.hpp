#pragma once
#include "halo2/vista_announcer_queue.hpp"
#include "halo2/vista_loading.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace halo2 {
class Cache;
struct OriginalData;
struct SourceKingNotice;
// Actual45B05D defined34-byte prefix. Caller stack padding is not a field.
struct VistaAnnouncerEvent {
  std::array<std::uint32_t,8> words{};
  std::uint16_t data{};
};
VistaAnnouncerEvent vista_announcer_event(std::span<const std::byte,34>);
VistaAnnouncerEvent vista_announcer_event_construct(std::uint32_t type,std::uint32_t subtype);
// King slot18 broadcasts cause player/team; recipient and effect fields stay
// NONE. The separate player-activation event has a targeted recipient.
VistaAnnouncerEvent vista_announcer_king_notice(const SourceKingNotice&);
// Consumed end-publication projection of47156A: selected engine, actual mode
// and game-over bytes, retained C48 marker and45B05D(0,19). Rank/phase/network
// side effects between the marker store and event construction stay external.
std::optional<VistaAnnouncerEvent> vista_announcer_game_over_event(
    std::uint8_t& published,bool engine_present,std::uint8_t world_mode,
    std::uint8_t game_over);
using VistaAnnouncerResponse=std::array<std::byte,0xa8>;
struct VistaAnnouncerResponses {
  std::vector<VistaAnnouncerResponse> general,slayer,king;
};
// Actual selected Vista matg+16C→mulg+0C runtime. No exported fixture paths.
VistaAnnouncerResponses load_vista_announcer_responses(const Cache&);
struct VistaAnnouncerAssets {
  std::filesystem::path path;
  std::shared_ptr<const Cache> cache;
  VistaAnnouncerResponses responses;
};
// Resolve the exact selected scenario in genuine PC cache headers. No Xbox
// datum or guessed filename mapping is used. Explicit native audio constants
// are passed through to the owned Cache for the separate host audio adapter.
VistaAnnouncerAssets open_vista_announcer_assets(const std::filesystem::path& pc_maps,
    const std::string& actual_scenario,std::shared_ptr<const OriginalData> native_audio_constants={});
struct VistaAnnouncerRecent {
  std::uint32_t hash{};
  std::array<std::uint32_t,4> sounds{};
  std::uint32_t tick{};
  bool operator==(const VistaAnnouncerRecent&)const=default;
};
struct VistaAnnouncerEventState {
  VistaAnnouncerQueueState queue;
  std::array<VistaAnnouncerRecent,4> recent;
};
void vista_announcer_events_initialize(VistaAnnouncerEventState&,std::int16_t tick_rate);
struct VistaAnnouncerLocalPlayer {
  std::uint32_t datum{};
  std::uint16_t flags{};
  std::int8_t membership{};
  std::int16_t output_user=-1; // Actual PC player+24; native local-view mapping.
};
struct VistaAnnouncerEventContext {
  std::uint8_t world_mode{};
  bool game_over{};
  std::uint32_t language{},tick{};
  std::int16_t tick_rate{};
};
struct VistaAnnouncerAudioSelection {
  std::uint32_t sound{},caption_key{},display_key{};
  std::int32_t delay_ticks{};
  unsigned audience{},response_ordinal{};
  bool announcer{},requested{};
};
// Audio-consumed projection of45DBF0/45DB5E/45BFCC/45D893/45C1E8/4D0F72.
// Call once per actual local recipient, in original iterator order. Only the
// actual general/Slayer/King banks are supported. Weighted responses reject.
// Caption/network side effects remain caller-owned; their keys are retained.
// Non-announcer playback is explicit; the admitted source start/end cues queue.
std::optional<VistaAnnouncerAudioSelection> vista_announcer_event_audio(
    VistaAnnouncerEventState&,const VistaAnnouncerResponses&,const VistaAnnouncerEvent&,
    const VistaAnnouncerLocalPlayer&,const VistaAnnouncerEventContext&,
    const VistaLoadingFloatingPoint&,const VistaAnnouncerPlayback& ordinary_playback={});
} // namespace halo2
