#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <string_view>

namespace halo2 {
// Supplied halo2.exe SHA33119398...109. These are PC14 owners; Xbox14's
// authored text indices, geometry and constructor are a different namespace.
enum class VistaLobbyLayoutWidget : unsigned { start,setup,find_games,players,chat,screen };
using VistaLobbyLayoutRect=std::array<std::int16_t,4>; // top,left,bottom,right.
struct VistaLobbyLayoutWidgetState {
  std::uint8_t visible{},enabled{}; // widget+6E/+6F.
  VistaLobbyLayoutRect bounds{}; // widget+20.
};
struct VistaLobbyLayoutText {
  std::uint8_t visible{};
  VistaLobbyLayoutRect bounds{};
};
struct VistaLobbyLayoutBitmap { std::uint8_t visible{}; };
struct VistaLobbyLayoutState {
  std::uint8_t flags{}; // screen+42, bit1 bypasses643632.
  std::int16_t pane{}; // screen+3484.
  std::array<std::uint8_t,4> team_edit{}; // screen+1C6C, stride4.
  std::uint8_t chat_enabled{}; // screen+1C6A; external61AAB4 result.
  std::uint8_t buttons_allowed=1; // screen+13A4, tracks editing/countdown.
  std::optional<VistaLobbyLayoutWidget> remembered_focus=VistaLobbyLayoutWidget::start; //+13A8.
  std::array<VistaLobbyLayoutWidgetState,5> widgets{};
  // Captured by644194..6441AB from original Players/Start bounds when the
  // factory flag is set. They are source rectangles, never inferred offsets.
  VistaLobbyLayoutRect normal_bounds{},host_bounds{}; //+3584/+358C.
};
struct VistaLobbyLayoutRegistration {
  std::array<VistaLobbyLayoutWidget,5> buttons{};
  unsigned count{};
};
// Original64404E..644176, after the genuine chat-provider query. Both inputs
// stay explicit; no offline/account assumption assigns the source global flag.
VistaLobbyLayoutRegistration vista_lobby_layout_registration(std::uint8_t factory_flag,
    std::uint8_t chat_flag);
// Original644177..64418F/86550C. A true factory flag overwrites the authored
// GAME DETAILS caption with this language-table text. Index0 is PLAYERS.
// This API does not implement the button's external XLive UI callback.
std::optional<std::u16string_view> vista_lobby_layout_players_caption(
    std::uint8_t factory_flag,unsigned source_language);

struct VistaLobbyLayoutServices {
  std::function<std::uint8_t()> host; //6152B0; each consumed call is separate.
  std::function<std::uint8_t()> countdown_active; //616BA2.
  std::function<std::uint8_t()> factory_flag; //60B365 ->actual D73ABC byte.
  std::function<std::int32_t()> player_count; //61525A; signed comparison >1.
  //611973 indices include header0/button-key1 before the pane's text array.
  std::function<VistaLobbyLayoutText*(std::int16_t)> text;
  std::function<std::uint8_t(VistaLobbyLayoutWidget)> focused; //612152(widget,0).
  std::function<void(VistaLobbyLayoutWidget,std::int32_t,std::uint8_t)> focus_change; //612660.
  std::function<void(VistaLobbyLayoutWidget)> activate_focus; //screen vtable+24.
  std::function<std::int32_t()> session_enum; //615EA9, supplied actual bank owner.
  std::function<void(std::int16_t)> select_pane; //610227, only when changed.
  std::function<VistaLobbyLayoutBitmap*(std::int16_t)> bitmap; //6119F7.
  std::function<std::uint8_t()> favorite; //6156B9; consumed only for bitmap41.
};
// Full643632..6438FD, including source focus order/remembered-focus writes.
// This owner emits no readiness text, roster entries or native mouse policy.
void vista_lobby_layout_visibility(VistaLobbyLayoutState&,const VistaLobbyLayoutServices&);
// Full64300D..643083: enums0/2/4 ->pane1,1/3/5 ->pane0,6 ->pane2;
// every other value retains the existing pane, then updates bitmap41.
void vista_lobby_layout_pane(VistaLobbyLayoutState&,const VistaLobbyLayoutServices&);
} // namespace halo2
