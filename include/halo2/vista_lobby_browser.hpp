#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace halo2 {
enum class VistaLobbyBrowserDialog : std::uint32_t {
  none=0,notification106=0x106,question107=0x107,notification108=0x108
};
struct VistaLobbyBrowserServices {
  //644B1C dispatches on pending+8 before acknowledgement. The adapter binds
  // the original3/4/NONE channel/window/controller and source callbacks:
  //106/108 ->643CE4;107 ->643C9F,643CBC. A callback may mutate the bank;
  // the original acknowledgement and percentage read observe those writes.
  // Pending NONE and0 instead jump past acknowledgement at644B67.
  std::function<void(VistaLobbyBrowserDialog)> dialog;
  // Runtime text51 exists independently of its current visibility. This
  // sink receives original5E4AA5's exact DWORD and1..99 visibility gate.
  // Localized name/format and widget text ownership remain separate.
  std::function<void(std::uint32_t percent,bool visible)> progress_text;
};
VistaLobbyBrowserDialog vista_lobby_browser_dialog(std::span<const std::byte> browser);
void vista_lobby_browser_acknowledge(std::span<std::byte> browser);
// received is the actual raw DWORD at86299C, sign extended by the original
// CDQ before unsigned64 multiply/divide. No clamping or float conversion.
std::uint32_t vista_lobby_browser_percent(std::span<const std::byte> browser,
    std::uint32_t received);
void vista_lobby_browser_update(std::span<std::byte> browser,std::uint32_t received,
    bool has_progress_text,const VistaLobbyBrowserServices&);
} //namespace halo2
