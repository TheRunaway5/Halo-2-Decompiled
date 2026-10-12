#pragma once
#include "halo2/vista_widget_registration.hpp"
#include <string_view>

namespace halo2 {
using VistaLobbyInitializerBounds = std::array<std::byte,8>;
struct VistaLobbyInitializerStyle {
  std::uint32_t identity{};
  std::span<const std::byte> bytes;
};
struct VistaLobbyInitializerMapOutputs {
  std::optional<std::uint32_t> first,second;
  std::uint32_t third{}; // Only the third output is initialized before61564E.
};
struct VistaLobbyIdentifierServices {
  std::function<std::span<std::byte>(VistaWidgetIdentity)> bank;
  // Genuine child virtual+4, dispatched by its actual retained type.
  std::function<void(VistaWidgetIdentity)> child_method;
  // Optional observer of611E23 entry; it does not supply a source result.
  std::function<void(VistaWidgetIdentity)> observed;
};
// Complete611E23 root-counter/identifier owner. Each child virtual+4 remains
//explicit, and next-child is reread after it. Source32 identities are data.
void vista_lobby_assign_widget_identifiers(VistaWidgetIdentity,
    const VistaLobbyIdentifierServices&);

inline constexpr std::size_t vista_lobby_chat_manager_bank_size=0x1e08;
// Actual6383EF birth and63870C clear. Birth preserves unowned bytes; clear
//does not reset pending ID1DFC, line count4, timeC or retained buffer1E00.
void vista_lobby_chat_manager_construct(std::span<std::byte>,std::uint32_t source_base);
void vista_lobby_chat_manager_clear(std::span<std::byte>,
    const std::function<void(std::uint32_t)>& release_owned_string);

struct VistaLobbyInitializerState {
  VistaWidgetIdentity screen;
  // Actual retained input descriptor, read after focus. At least32 bytes.
  std::span<const std::byte> descriptor;
  std::span<std::byte> chat_manager;
  // Actual D78CB8..D78DFC inclusive. No fresh/default contents are inferred.
  std::span<std::byte> globals;
  // Source32 identity for the temporary registration pointer table. Storage
  //and identity are caller-owned; they remain valid through register_children.
  std::uint32_t registration_table{};
};
struct VistaLobbyInitializerServices {
  std::function<std::span<std::byte>(VistaWidgetIdentity)> bank;
  std::function<std::uint32_t(std::uint32_t)> definition; //60C701(screen+70).
  std::function<std::uint32_t()> context; //609674.
  std::function<std::uint8_t(std::uint32_t)> chat_available; //61AAB4.
  std::function<std::uint8_t()> factory_flag; //60B365, two separate samples.
  std::function<void(std::uint32_t,const VistaWidgetRegistration&)> register_children; //610A44.
  std::function<unsigned()> language; //4381FD.
  // Actual widget virtual+4C. Borrowed style bytes remain live until copied.
  std::function<VistaLobbyInitializerStyle(VistaWidgetIdentity)> style;
  // Actual returned string-owner virtual+4; this must perform real assignment.
  std::function<void(std::uint32_t,std::u16string_view)> assign_string;
  std::function<void(VistaWidgetIdentity)> identifier_child; //Child virtual+4.
  std::function<void(VistaWidgetIdentity)> identifier_observed;
  std::function<std::uint8_t()> host; //6152B0, independently sampled.
  std::function<void(VistaWidgetIdentity)> activate_focus; //Screen virtual+24.
  std::function<void(VistaWidgetIdentity,std::uint32_t,std::uint32_t)> focus; //612660.
  std::function<std::optional<std::span<const std::byte>>()> variant; //615692.
  std::function<std::uint8_t(VistaLobbyInitializerMapOutputs&)> selected_map; //61564E.
  std::function<std::uint8_t()> favorite; //6156B9.
  // Actual499D1F required text/font preparation.61D288 supplies the literal
  //digits and font4. Its lower glyph/cache binding cannot be a silent no-op.
  std::function<std::uint8_t(std::u16string_view,std::uint32_t)> prepare_text;
  std::function<void(std::uint32_t)> release_owned_string; //48F524 heap owner.
  std::function<VistaWidgetIdentity(std::int16_t)> text; //611973.
  std::function<VistaLobbyInitializerBounds(VistaWidgetIdentity)> animated_bounds; //Virtual+20.
  std::function<void(VistaWidgetIdentity)> append; //61208E(screen,child).
  // Complete61BC81 invocation, including the independently reread style4
  //DWORD and child68 WORD after append. Lower61BB0B remains a real service.
  std::function<void(VistaWidgetIdentity,std::uint32_t,std::uint16_t,
      const std::array<std::uint32_t,4>&,std::uint32_t,
      const VistaLobbyInitializerBounds&)> initialize_clone;
};
// Complete643FF9 orchestration plus643F5B/643ADE and63870C/638487. Required
//genuine lower services are consumed lazily; absent services reject explicitly.
//This does not bind live widgets, font cache, platform heap or manager activation.
void vista_lobby_initialize(VistaLobbyInitializerState&,
    const VistaLobbyInitializerServices&);
} //namespace halo2
