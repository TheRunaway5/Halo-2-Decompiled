#pragma once

#include "halo2/vista_loading.hpp"
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace halo2 {

// Literal D73AA0 owner layout, original Vista 638CE9..639051. The final
// timestamp is deliberately retained by initialize, as in 638CE9.
struct VistaLoadingMusicState {
  std::uint32_t applied;
  std::uint32_t requested;
  std::uint32_t desired;
  std::uint32_t previous_ui_clock;
  std::uint32_t sound_handle;
  std::uint32_t stopped_network_clock;
  bool operator==(const VistaLoadingMusicState&) const = default;
};

struct VistaLoadingMusicSharedUi {
  std::uint32_t music_datum; // Actual wigl+1BC, not a native song ordinal.
  std::uint32_t silence_ms;  // Actual wigl+1C0; not a map loading estimate.
};

// Each callback is a service boundary in the unchanged original routines.
// Request/conversion/state/estimate are used only by actual screen209.
struct VistaLoadingMusicServices {
  std::function<std::uint32_t()> current_screen;                 // 6096B4
  std::function<bool()> audio_allowed;                           // 621B68 AL
  std::function<std::uint32_t()> ui_clock;                       // 6096AE
  std::function<std::uint32_t()> network_clock;                  // 437E51
  std::function<std::optional<VistaLoadingMusicSharedUi>()> shared_ui; // 60BB89
  std::function<std::uint32_t(std::uint32_t,std::uint32_t,std::uint32_t)> play; // 485DA1
  std::function<void(std::uint32_t)> stop;                        // 485DEB
  std::function<std::optional<std::u16string>()> loading_request; // 609E82+8
  std::function<std::string(std::u16string_view)> path_conversion; // WideCharToMultiByte
  std::function<std::uint32_t(std::string_view)> map_state;       // 44B633
  std::function<std::int32_t(std::u16string_view)> estimate;       // 4317A9(0,path)
};

void vista_loading_music_initialize(VistaLoadingMusicState&);

// Scenario owner absent or kind!=2 sets desired=0. Only original mapped screen
// IDs change desired; every other screen retains it. No session transition or
// platform owner is inferred by these helpers.
void vista_loading_music_desired(VistaLoadingMusicState&,
    std::optional<std::uint16_t> scenario_kind,const VistaLoadingMusicServices&);

// Entire 63902E→638EE0/638C60 control flow, including signed 6000ms admission,
// source sound datum/handle and distinct UI/network clock domains.
void vista_loading_music_update(VistaLoadingMusicState&,
    std::optional<std::uint16_t> scenario_kind,const VistaLoadingMusicServices&);

// Actual 638D16 strict unsigned elapsed>duration predicate, and 638D4C's
// binary32 secondary progress. Neither function owns map progress or lifetime.
bool vista_loading_music_ready(const VistaLoadingMusicState&,
    const VistaLoadingMusicServices&);
std::uint32_t vista_loading_music_secondary_bits(const VistaLoadingMusicState&,
    const VistaLoadingMusicServices&,const VistaLoadingFloatingPoint&);

} // namespace halo2
