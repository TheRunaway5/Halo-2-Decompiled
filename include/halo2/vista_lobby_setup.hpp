#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <string_view>

namespace halo2 {
// PC23 owners from the supplied halo2.exe SHA33119398...109. Opaque values
// represent existing widgets/images/context results; they are never fabricated
// from a menu selection. These routines do not replace the Xbox 3D backdrop.
using VistaLobbySetupObject=std::uintptr_t;
struct VistaLobbySetupList {
  std::array<std::int16_t,7> rows{};
  std::array<std::uint32_t,7> handles{}; //4667A0 results; bank index is low16.
  std::uint8_t count{};
  std::uint8_t field464{};
  std::uint32_t callback{}; //source+460;64FA19.
};
struct VistaLobbySetupState { std::uint8_t party_removed{}; }; //screen+EC0.
struct VistaLobbySetupMap {
  std::int32_t field4cb8{},map4dd8{};
  VistaLobbySetupObject identifier4cbc{}; //61564E output3, not an image.
};
struct VistaLobbySetupRequest {
  std::uint32_t controllers{},argument2{},argument3{},factory{};
};
struct VistaLobbySetupRegistration {
  std::array<std::uint32_t,4> fields{0,1,0,0};
  std::uint32_t list_offset=0xa5c;
  std::uint32_t field14{};
  std::array<std::uint8_t,0x50> tail{};
};
struct VistaLobbySetupServices {
  //64FABC constructor. The generic child constructor and allocation remain
  //explicit services; their supplied arguments and order are source-owned.
  std::function<void(std::int32_t)> base_list; //613B1C.
  std::function<void(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> child_array; //68870B(offset,stride,count,ctor,dtor).
  std::function<void()> callback_owner; //6113C6.
  std::function<std::int32_t()> session_enum; //615EA9.
  std::function<void(std::string_view,std::uint32_t,std::uint32_t)> row_bank; //60D1FD.
  std::function<void()> bank_initialize; //466B33.
  std::function<std::uint32_t()> allocate_row; //4667A0.
  std::function<void(std::uint16_t,std::int16_t)> store_row; //bank+44[index*4]+2 direct write.
  std::function<std::uint8_t()> host; //6152B0.
  std::function<std::int32_t()> count; //61525A, signed.
  std::function<void(std::uint32_t)> callback_link; //6113D3,64FA19 registration.

  //64EEEF/6139F8 and64F043. A failed label lookup leaves the widget unchanged.
  std::function<VistaLobbySetupObject(std::uint32_t)> text; //611973.
  std::function<std::optional<std::int16_t>(std::uint32_t)> selected_row; //46639B(handle).
  std::function<void(VistaLobbySetupObject,std::uint32_t)> set_string; //61BF85.
  std::function<std::uint32_t(std::uint32_t)> screen_tag; //60C701(screen ID).
  std::function<void(std::uint32_t,const VistaLobbySetupRegistration&)> register_list; //610A44.
  std::function<void()> activate_list; //screen virtual+24.
  std::function<void()> initial_focus; //611E23.

  //Full64F0EB current-details owner. Resource override and authored frame are
  //different writes (61CFED/61CF73 versus61C300); callers must retain both.
  std::function<std::uint32_t()> selection; //61383B.
  std::function<VistaLobbySetupObject(std::uint32_t,std::uint32_t,std::uint32_t)> child; //611909(kind,index,recursive).
  std::function<std::uint8_t(VistaLobbySetupMap&)> map_query; //61564E, actual output tuple.
  std::function<void(VistaLobbySetupObject,VistaLobbySetupObject)> image; //61CF73.
  std::function<std::optional<std::uint32_t>(std::int32_t)> multiplayer_definition; //459A1A ->definition+8.
  std::function<VistaLobbySetupObject(std::uint32_t,std::uint32_t)> resolve_image; //77D482(reference,frame).
  std::function<std::uint8_t(VistaLobbySetupObject,VistaLobbySetupObject&)> identifier_image; //459369, output first dword only.
  std::function<std::optional<std::int32_t>()> variant_engine; //615692 ->variant+44; null preserved.
  std::function<void(VistaLobbySetupObject,std::uint32_t,std::uint32_t)> resource_frame; //61CFED(resource index,image frame).
  std::function<void(VistaLobbySetupObject,std::uint32_t)> authored_frame; //61C300.
  std::function<VistaLobbySetupObject(std::int16_t)> find_row; //613772.
  std::function<void()> reset_list; //614C32.
  std::function<void(std::uint32_t)> set_selection; //613F50.
  std::function<void(VistaLobbySetupObject)> remove_row; //6144A4.
  std::function<void()> base_update; //612CD8.

  //64FA19 dispatch reads a row-bank short directly after the NONE-handle gate.
  //Callbacks3/5/6/7 remain explicit owners; this helper does not invent their
  //session/XLive leaves or substitute native actions for them.
  std::function<std::int16_t(std::uint16_t)> dispatch_row;
  std::function<void(std::uint32_t,VistaLobbySetupObject)> invoke_callback;
  //64F9A1/64F9DD/64EF79/658927 publication.658927 writes+1140 on the result
  //returned by60B11E, not on an assumed60C226 result. Keep this source seam.
  std::function<void()> request_context_initialize; //60B0BC.
  std::function<VistaLobbySetupObject(const VistaLobbySetupRequest&)> screen_request; //60C226.
  std::function<VistaLobbySetupObject()> request_context_release; //60B11E.
  std::function<void(VistaLobbySetupObject,std::int32_t)> set_quick_key; //result+1140.
};
void vista_lobby_setup_construct(VistaLobbySetupList&,std::int32_t controller,const VistaLobbySetupServices&);
bool vista_lobby_setup_label(std::optional<std::uint32_t> row_widget_handle,const VistaLobbySetupServices&);
void vista_lobby_setup_initialize(const VistaLobbySetupServices&);
void vista_lobby_setup_details(VistaLobbySetupState&,const VistaLobbySetupServices&);
void vista_lobby_setup_dispatch(std::uint32_t handle,VistaLobbySetupObject event,const VistaLobbySetupServices&);
void vista_lobby_setup_open_map(std::int32_t controller,const VistaLobbySetupServices&);
void vista_lobby_setup_open_rules(std::int32_t controller,const VistaLobbySetupServices&);
//Invalid engine IDs publish nothing. A null variant violates64EF79's source
//precondition and is rejected, rather than selecting a guessed default engine.
void vista_lobby_setup_open_quick(std::int32_t controller,const VistaLobbySetupServices&);
void vista_lobby_setup_quick_request(std::int32_t key,std::uint32_t argument2,std::uint32_t argument3,
    std::uint32_t controllers,const VistaLobbySetupServices&);
} //namespace halo2
