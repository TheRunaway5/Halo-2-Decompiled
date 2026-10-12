#pragma once
#include "halo2/vista_local_user.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace halo2 {
// Supplied Vista PE SHA256
// 33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109.
// Complete profile assignment 6087BF, input-control helper 6080DC, display
// publication 608312 and player-payload publication 606A97/60696F. These
// operations consume retained owner banks; they do not invent identities,
// profiles, input devices, account state or session membership.
using VistaPcProfile=std::array<std::byte,0x1208>;
inline constexpr std::size_t vista_input_control_stride=0x1680;
inline constexpr std::size_t vista_input_control_bank_size=
    vista_local_user_count*vista_input_control_stride;
struct VistaControllerProfileBanks {
  std::span<std::byte> controllers;
  std::span<std::byte> local_users;
  std::span<std::byte> input_controls;
};
struct VistaPlatformXuidResult {
  std::uint32_t hresult_word{};
  VistaLocalUserIdentity output{};
};
struct VistaControllerPresentationContext {
  std::uint32_t type_word{}; // actual 609674() result +8
  std::span<const std::byte> network_text; // actual result +4E, for type 2
};
struct VistaInputBindingArguments {
  // The original MOVZX AX preserves argument_word's upper sixteen bits.
  std::uint32_t selector_5_word{};
  std::uint32_t selector_4_word{};
  std::uint32_t selector_8_word{};
  std::uint32_t argument_word{};
};

// Synchronous owner boundaries. Implementations must supply the real platform
// result, real binding-layout mutation, dependency refresh, reference-manager
// events, and presentation context/formatter. Callbacks must not reenter these
// banks. Their exceptions and external effects are not rolled back.
class VistaControllerProfileServices {
 public:
  virtual ~VistaControllerProfileServices()=default;
  virtual VistaPlatformXuidResult xuser_get_xuid(std::uint32_t controller)=0;
  virtual void release_profile(std::uint32_t storage_index)=0; // 43F0FC
  virtual void acquire_profile(std::uint32_t storage_index)=0; // 43F0DE
  // Actual 45EE72: receives the complete mutable runtime control record.
  // Returned raw AL is eventually returned unchanged by 6087BF.
  virtual std::uint8_t apply_input_bindings(std::span<std::byte> control_record,
      VistaInputBindingArguments)=0;
  // Actual 45E717, after 461C17 has published the complete control record.
  virtual void refresh_input_dependencies(std::span<const std::byte> control_record,
      std::uint32_t controller)=0;
  virtual VistaControllerPresentationContext presentation_context()=0;
  // Actual 44BD59(destination,32,L"%hs",context+4E), with the unchanged
  // 5AC4C0 stub's AL=0 branch. The formatter must preserve untouched tail bytes
  // exactly; no generated display string substitutes for its owner output.
  virtual void format_network_display_name(std::span<std::byte> destination64,
      std::span<const std::byte> actual_network_text)=0;
};

// Explicit bounded admission: all three banks must be complete and disjoint;
// controller is 0..3. A first assignment requires a free local-user slot;
// existing mappings must be FFFFFFFF or 0..3. These checks precede mutations
// and callbacks. The original unchecked full-bank underflow is rejected.
std::uint8_t vista_controller_profile_assign(VistaControllerProfileBanks,
    std::uint32_t controller,VistaPcProfile actual_complete_profile,
    std::uint32_t actual_profile_storage_index,std::uint32_t input_control_argument,
    VistaControllerProfileServices&);
// Display/payload publication also accepts a controller with mapping FFFFFFFF:
// it updates display only. Active and inactive mapped user banks retain the
// original read/default/write distinction; publication does not activate users.
void vista_controller_profile_publish(VistaControllerProfileBanks,
    std::uint32_t controller,VistaControllerProfileServices&);
// Actual 608257 removal clears the mapped complete local-user record and
// releases a selected profile. It preserves controller bytes the source does.
void vista_controller_profile_remove(VistaControllerProfileBanks,
    std::uint32_t controller,VistaControllerProfileServices&);
// Actual 608986: display/payload publication occurs before fresh allocation
// and before identity replacement. A new allocation uses the OLD controller
// XUID. It consequently does not publish a fresh user's payload in this call.
void vista_controller_identity_update(VistaControllerProfileBanks,
    std::uint32_t controller,VistaLocalUserIdentity actual_supplied_identity,
    VistaControllerProfileServices&);
// Exact 6068F2 leaf: store the raw team DWORD at controller+1220. No user
// publication or service call occurs here, and no team clamping occurs until
// the separate 60696F payload builder. Requires only a complete controller bank
// and controller0..3; current profile/user mappings do not gate this store.
void vista_controller_team_write(std::span<std::byte> controllers,
    std::uint32_t controller,std::uint32_t raw_team_word);
} // namespace halo2
