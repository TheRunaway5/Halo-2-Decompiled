#pragma once
#include "halo2/vista_widget_registration.hpp"

namespace halo2 {
struct VistaLobbyWidgetQueryServices {
  // Retained source32 identities, never host pointer casts. Bank lookup is a
  // storage primitive; nodes stay alive across every original callback.
  std::function<std::span<std::byte>(VistaWidgetIdentity)> bank;
  std::function<std::uint32_t(VistaWidgetIdentity)> root80, root84;
  std::function<std::uint32_t(VistaWidgetIdentity)> virtual34, virtual38;
  std::function<VistaWidgetIdentity(std::uint32_t, std::uint32_t, std::uint32_t)>
      focused; //609EFD(argument,virtual34,virtual38).
  std::function<void(std::uint32_t, std::uint32_t, std::uint32_t,
      VistaWidgetIdentity)> set_focused; //609FE4, same tuple, new owner.
  std::function<void(VistaWidgetIdentity, std::uint32_t)> gained, lost;
  std::function<void(VistaWidgetIdentity)> base_layout; //60F790.
};

// PC14's actual virtual leaves: +1C=6116BD, +54=60E884, +3C/+40=
//611903/611906. The duration is zero because the supplied EXE returns zero;
//60EAEC's authored first-pane duration is a different virtual implementation.
std::uint32_t vista_lobby_widget_duration() noexcept;
void vista_lobby_widget_prepare() noexcept;
void vista_lobby_widget_focus_notification(std::uint32_t) noexcept;
std::uint32_t vista_lobby_widget_root80(VistaWidgetIdentity,
    const VistaLobbyWidgetQueryServices&); //60EB8A, root+74.
std::uint32_t vista_lobby_widget_root84(VistaWidgetIdentity,
    const VistaLobbyWidgetQueryServices&); //60EB8E, root+78.

// Complete6120F8/612120 root walks. An unparented non-screen has no root:
//virtual34 faults in the original and rejects here; virtual38 returns literal4.
std::uint32_t vista_lobby_widget_virtual34(VistaWidgetIdentity,
    const VistaLobbyWidgetQueryServices&);
std::uint32_t vista_lobby_widget_virtual38(VistaWidgetIdentity,
    const VistaLobbyWidgetQueryServices&);
bool vista_lobby_widget_focused(VistaWidgetIdentity, std::uint32_t argument,
    const VistaLobbyWidgetQueryServices&); //612152.
void vista_lobby_widget_focus(VistaWidgetIdentity, std::uint32_t argument,
    std::uint8_t acquire, const VistaLobbyWidgetQueryServices&); //612660.

// Complete611973(kind6) or6119F7(kind8): direct children only, ordinal is
//signed low16. Unsupported kinds reject rather than invent another owner.
VistaWidgetIdentity vista_lobby_widget_child(VistaWidgetIdentity,
    std::uint32_t ordinal, std::uint32_t kind,
    const VistaLobbyWidgetQueryServices&);
//6116C0/6116D2 copy two DWORDs in source order, including aliased storage.
void vista_lobby_widget_get_bounds(VistaWidgetIdentity,
    std::span<std::byte,8>, const VistaLobbyWidgetQueryServices&);
void vista_lobby_widget_set_bounds(VistaWidgetIdentity,
    std::span<const std::byte,8>, const VistaLobbyWidgetQueryServices&);
// Complete642F12 with the genuine lookup/copy owners above. The screen retains
//actual source32 arrow identities at+A20/A24; no shadow state or guessed child.
//60F790 remains a required resource/platform service, called last.
void vista_lobby_widget_child_layout(VistaWidgetIdentity,
    const VistaLobbyWidgetQueryServices&);
} //namespace halo2
