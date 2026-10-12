#pragma once
#include "halo2/vista_lobby_constructor.hpp"
#include "halo2/vista_widget_registration.hpp"

namespace halo2 {
using VistaLobbyFactoryDescriptor = std::span<std::byte,32>;
struct VistaLobbyFactoryGlobals {
  VistaLobbyConstructorGlobals construction;
  VistaWidgetIdentity current; // D78CB4; publication precedes the +6C store.
  std::uint8_t alternate{}; // D73ABC; published only after60B8C3 returns.
};
struct VistaLobbyManagerGraphics {
  std::uint32_t identity{}; // E3C6B4, explicit renderer-interface identity.
  std::uint8_t failed{}; // E3E4D4; only a negative HRESULT writes one.
};
struct VistaLobbyManagerServices {
  // Caller-owned, stable source32 registry. Manager banks need34h bytes;
  // source identities are never converted to host pointers/vtable calls.
  std::function<std::span<std::byte>(VistaWidgetIdentity)> bank;
  std::function<std::uint32_t(VistaWidgetIdentity)> window; // screen virtual84.
  std::function<void(std::uint32_t)> sound; //61DD04(3).
  std::function<void(VistaWidgetIdentity,VistaWidgetIdentity,VistaLobbyFactoryDescriptor)> pending; //manager virtual18.
  std::function<void(VistaWidgetIdentity,std::uint32_t)> destructor; //screen virtual0(0).
  std::function<void(VistaWidgetIdentity)> free; //60D2EA.
  std::function<void(VistaWidgetIdentity)> dispose; //60B9AC, not merely free.
  std::function<std::int32_t(std::uint32_t)> graphics; //interface virtual0C, stdcall(this).
  std::function<void(VistaWidgetIdentity)> prepare; //screen virtual54.
  std::function<void(VistaWidgetIdentity,std::uint32_t,std::uint32_t)> focus; //612660(0,1).
  std::function<void(VistaWidgetIdentity,VistaLobbyFactoryDescriptor)> initialize; //virtual58; PC14=643FF9.
  std::function<void(VistaWidgetIdentity)> child_layout,base_layout; //virtual5C/60;642F12/610B4E.
  std::function<void(VistaWidgetIdentity,std::uint32_t)> animate; //612604.
  // Exact borrowed manager+20 pointer: last16 bytes of retained descriptor.
  std::function<void(VistaWidgetIdentity,std::span<std::byte,16>)> parameters; //virtual94.
  std::function<std::uint8_t(VistaWidgetIdentity,std::uint32_t)> focused; //612152(0).
  std::function<void(VistaWidgetIdentity)> update; //virtual0C; PC14=64564B.
};
struct VistaLobbyFactoryServices {
  std::function<VistaWidgetIdentity(std::uint32_t,std::uint32_t)> allocate; //60D2D8(3594h,0).
  std::function<std::uint32_t()> read_cached_ui_clock; //6096AE, nine independent peeks.
  VistaLobbyManagerServices manager;
};
// Complete637787 pending replacement/copy. Disposal may reenter this owner;
// descriptor words are copied in source order, including overlapping storage.
void vista_lobby_manager_pending(VistaWidgetIdentity manager,VistaWidgetIdentity screen,
    VistaLobbyFactoryDescriptor,const VistaLobbyManagerServices&);
// Complete60B8C3. Manager address selection uses the actual six-entry source
// table and wrapping DWORD arithmetic; registry storage/birth stays external.
void vista_lobby_register_screen(VistaWidgetIdentity,VistaLobbyFactoryDescriptor,
    const VistaLobbyManagerServices&);
// Complete646752/6467E9, using the complete frozen6463AB native constructor.
// Null allocation publishes current=0 then throws at the original null+6C
// fault boundary; no successful registration/alternate publication is invented.
VistaWidgetIdentity vista_lobby_factory(bool alternate,VistaLobbyFactoryDescriptor,
    VistaLobbyFactoryGlobals&,const VistaLobbyFactoryServices&);
// Complete637670 orchestration. Required consumed virtual/platform owners
// remain explicit real services. No manager birth, session/license/resource
// state, descriptor612250 implementation, or visible route is supplied here.
// Reentry which clears a source object before virtual dispatch is rejected at
// that dispatch, retaining the original null-fault prefix rather than success.
void vista_lobby_manager_activate(VistaWidgetIdentity,VistaLobbyManagerGraphics&,
    const VistaLobbyManagerServices&);
} //namespace halo2
