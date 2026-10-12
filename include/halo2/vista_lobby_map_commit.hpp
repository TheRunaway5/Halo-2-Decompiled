#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <string_view>

namespace halo2 {
using VistaLobbyMapCommitObject = std::uintptr_t;
struct VistaLobbyMapCommitState {
  std::uint8_t locked{}; // list+7E8.
  std::uint8_t edit{}; // list+804; independent of the locked flag.
  // Real allocation slots: standard stride8, map ID+4; custom stride60h.
  // The callback masks a non-NONE handle to its low16 bits.
  std::span<const std::byte> rows;
};
struct VistaLobbyMapCommitServices {
  // Virtual getters consume no argument. The B4h pushed before v38 is
  // retained for the subsequent609850 dialog's third argument.
  std::function<std::uint32_t()> widget_v34, widget_v38;
  std::function<void(std::uint32_t, std::uint32_t, std::uint32_t)> dialog;
  std::function<std::span<std::byte>()> edit_owner; //609B9D, at least298h bytes.
  std::function<VistaLobbyMapCommitObject(std::uint32_t)> definition; //459A1A.
  std::function<std::u16string_view(VistaLobbyMapCommitObject)> definition_name; //781CA5.
  //687567 bounded32-unit UTF16 copy, including its genuine CRT error owner.
  // No guessed truncation or invalid-parameter handling is supplied here.
  std::function<void(std::span<std::byte>, std::u16string_view)> copy_name;
  std::function<void()> publish_edit; //609C3E.
  std::function<std::uint8_t(std::uint32_t, std::uint32_t)> select_map; //61557F.
  std::function<std::uint8_t()> class_query; //6156B9.
  std::function<void()> selection_context; //643D20, side effect before643DD1.
  std::function<std::uint8_t(std::uint32_t, std::span<const std::byte>)> select_custom_context; //643DD1.
  std::function<std::uint8_t(std::span<const std::byte>)> select_custom_session; //61555C.
  std::function<VistaLobbyMapCommitObject()> root_screen; //611BA1.
  std::function<std::uint32_t(VistaLobbyMapCommitObject)> screen_v80, screen_v84;
  std::function<void(std::uint32_t, std::uint32_t)> close; //6096DA.
  std::function<VistaLobbyMapCommitObject(std::uint32_t)> text; //611973 on row.
  std::function<void(VistaLobbyMapCommitObject, std::u16string_view)> string; //61BAA9.
};
// Complete65A29A/65ACD8 callback bodies. External map/session and platform
// owners remain explicit. Failed publication still reaches the source close.
void vista_lobby_standard_map_commit(const VistaLobbyMapCommitState&,
    std::uint32_t handle, const VistaLobbyMapCommitServices&);
void vista_lobby_custom_map_commit(const VistaLobbyMapCommitState&,
    std::uint32_t handle, const VistaLobbyMapCommitServices&);
// Complete65A24B row-label callback; missing text preserves its prior state.
void vista_lobby_standard_map_label(const VistaLobbyMapCommitState&,
    std::uint32_t handle, const VistaLobbyMapCommitServices&);
} // namespace halo2
