#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>

namespace halo2 {
// Supplied Vista halo2.exe SHA256
//33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109.
// PC14 initialize644303/644309 and update6438FE. These are screen-owned
// update counters, independent of elapsed milliseconds or bitmap fps.
struct VistaLobbySpinnerState {
  std::int32_t counter{}; //screen+357C.
  std::int32_t frame{2}; //screen+3580.
};
struct VistaLobbySpinnerBitmap {
  std::uint8_t visible{}; //widget+6E.
  std::uint16_t frame{}; //widget+90; retains its value for invalid requests.
  std::int32_t pages{}; //actual bound bitm+44.
  std::uint8_t bound{1}; //source bitmap definition datum !=NONE.
};
// This service supplies each actual5AD736 current-session pointer query;
// the adapters retain5B1A37's source state admission. No bank is fabricated.
using VistaLobbySpinnerSessionQuery =
    std::function<std::optional<std::span<const std::byte>>() >;
std::uint8_t vista_lobby_spinner_player_present(std::uint32_t player,
    const VistaLobbySpinnerSessionQuery&);
std::uint8_t vista_lobby_spinner_peer_loading(std::uint32_t player,
    const VistaLobbySpinnerSessionQuery&);

struct VistaLobbySpinnerServices {
  //6119F7 direct kind8 ordinal lookup; null is a genuinely absent child.
  std::function<VistaLobbySpinnerBitmap*(std::int16_t)> bitmap;
  std::function<std::uint8_t(std::uint32_t)> occupied; //6156E1 AL.
  std::function<std::uint8_t(std::uint32_t)> peer_loading; //615B4F AL.
  // Optional observer of actual61C300 requests, before frame validation.
  std::function<void(std::int16_t,std::int32_t)> frame_requested;
};
void vista_lobby_spinner(VistaLobbySpinnerState&,
    std::span<const std::uint32_t> ordered_player_ids,std::int32_t count,
    const VistaLobbySpinnerServices&);
} //namespace halo2
