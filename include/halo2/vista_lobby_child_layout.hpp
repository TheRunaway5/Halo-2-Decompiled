#pragma once
#include <array>
#include <cstdint>
#include <functional>

namespace halo2 {
// Identity of a child in the bound screen, not an index or an owned pointer.
// Zero preserves the original lookup's NULL result; the adapter owns lifetime.
using VistaLobbyChildHandle=std::uintptr_t;
using VistaLobbyChildRect=std::array<std::int16_t,4>; // top,left,bottom,right.
struct VistaLobbyChildLayoutState {
  std::uint8_t chat_enabled{}; // screen+1C6A, every nonzero byte is true.
  std::array<VistaLobbyChildHandle,2> arrows{}; // retained screen+A20/A24.
};
struct VistaLobbyChildLayoutServices {
  // Each service is bound to the same screen as the state. Returned identities
  // remain valid through base_layout; this helper neither allocates nor frees.
  std::function<VistaLobbyChildHandle(std::int16_t)> bitmap; //6119F7.
  std::function<VistaLobbyChildHandle(std::int16_t)> text; //611973.
  // Memory-store primitive for the direct child+6E writes at642F4F/642F59.
  // This is not an invented original function call or a visibility policy.
  std::function<void(VistaLobbyChildHandle,std::uint8_t)> store_visible;
  std::function<VistaLobbyChildRect(VistaLobbyChildHandle)> get_bounds; //6116C0.
  std::function<void(VistaLobbyChildHandle,const VistaLobbyChildRect&)> set_bounds; //6116D2.
  std::function<void()> base_layout; //60F790; implementation stays external.
};
// Complete PC14 child-layout642F12..642FAB from supplied halo2.exe SHA
//33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109.
// Evidence: analysis/frontend/vista-session-flow-audit/pc14-child-layout-original.
// Stores both arrow identities, conditionally hides them, doubles text51's
// signed-16 height by replacing only bottom, then calls base_layout last.
// The original chat-disabled branch dereferences both arrows without a NULL
// guard. A missing required child/service is an invalid binding and throws;
// there is no substitute child, bounds, visibility, or base implementation.
void vista_lobby_child_layout(VistaLobbyChildLayoutState&,
    const VistaLobbyChildLayoutServices&);
} // namespace halo2
