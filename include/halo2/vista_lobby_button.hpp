#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string_view>

namespace halo2 {
// Separate source owners: callback arg1 -> event+4, arg2 -> row ordinal,
// screen+1C6A/+1C6C, and the persistent UTF-16 bank D78BC0. Services can
// change these owners; later source loads must observe those changes.
struct VistaLobbyButtonEvent { std::uint32_t controller{}; };
struct VistaLobbyButtonState {
  std::uint8_t chat_enabled{};
  std::array<std::uint8_t,4> team_edit{};
};
using VistaLobbyButtonChat=std::array<char16_t,121>;
using VistaLobbyButtonTask=std::array<std::uint32_t,8>;
struct VistaLobbyButtonServices {
  std::function<std::uintptr_t()> chat_owner; //screen+D5C virtual+4C.
  std::function<std::optional<std::u16string_view>(std::uintptr_t)> chat_characters; //owner virtual+C; null preserved.
  std::function<void(std::uint32_t)> set_errno; //687567 ->68EE22, TLS errno write.
  std::function<void()> invalid_parameter; //68EDC3(0,0,0,0,0); returning handler only.
  std::function<std::int32_t()> language; //4381FD, independently queried twice unless first==6.
  std::function<void(std::int32_t,std::uint32_t,std::span<const char16_t>,std::uint32_t)> keyboard; //63C9F6(controller,15,D78BC0,121).
  std::function<void()> game_details; //615C81; before sign-in/host gates.
  std::function<std::uint8_t(std::int32_t)> signed_in; //606B50.
  std::function<std::uint8_t()> host; //6152B0.
  std::function<void(std::int32_t,std::uint8_t)> finish_team_edit; //64534C(controller,0).
  std::function<std::uint8_t()> setup_ready; //6153C9 ->5B1C39, independent of616490.
  std::function<void(std::uint8_t)> settings_change; //615219(1), return ignored.
  std::function<void(std::int32_t)> cancel_action; //643C25, return ignored.
  std::function<std::int32_t()> readiness; //616490(NULL), literal null input.
  std::function<void(std::int32_t)> start_action; //643A40, return ignored.
  //60B11E invokes descriptor+1C immediately with the descriptor pointer.
  //Genuine646186/64FE89 allocation/construction/registration remains external;
  //calling this service neither assumes success nor opens a native screen.
  std::function<void(std::uint32_t,const VistaLobbyButtonTask&)> task_factory;
};
// Complete64620F plus reached60B0BC/60B0C6/60B11E and bounded687567 copy.
// No scalar result is assigned to the original void-style callback. Invalid
// controller storage is rejected only when the source reaches its team load.
void vista_lobby_button(VistaLobbyButtonEvent&,std::uint16_t& ordinal,
    VistaLobbyButtonState&,VistaLobbyButtonChat&,const VistaLobbyButtonServices&);
} //namespace halo2
