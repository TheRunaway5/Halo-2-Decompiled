#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace halo2 {
struct VistaLicenseOpenResult {
  std::int32_t hresult;
  std::uint32_t handle;
};
struct VistaLicenseStatusResult {
  std::int32_t hresult;
  std::uint32_t count,pointer;
};
struct VistaMainmenuPlatformGateServices {
  // All words are source/platform binding identities, never native pointers.
  //4047C4 ignores allocation/setter HRESULTs, as the original caller does.
  std::function<std::int32_t(std::uint32_t,std::uint32_t&)> allocate_pbuffer;
  std::function<std::int32_t(std::uint32_t,std::uint32_t,std::uint8_t)> set_pbuffer_byte;
  std::function<void()> initialize_licensing; //427642.
  //4285FD's fixed CreateEventA(NULL,FALSE,FALSE,NULL) platform boundary.
  std::function<std::uint32_t()> create_event;
  std::function<void(std::uint32_t)> close_event;
  std::function<VistaLicenseOpenResult()> open_license;
  std::function<std::int32_t(std::uint32_t,std::span<const std::byte,16>)> consume_right;
  std::function<VistaLicenseStatusResult(std::uint32_t)> query_license_status;
  std::function<void(std::uint32_t)> close_license;
  // Actual status array has28h stride and the consumed DWORD is row+10h.
  std::function<std::uint32_t(std::uint32_t,std::uint32_t)> read_status;
  std::function<void(std::uint32_t)> free_status;
};

// Original79BB20 product/right GUID in its exact16-byte memory order.
inline constexpr std::array<std::byte,16> vista_mainmenu_license_right={
  std::byte{0x6e},std::byte{0x8c},std::byte{0xb0},std::byte{0x41},
  std::byte{0x7a},std::byte{0x7e},std::byte{0xaa},std::byte{0x44},
  std::byte{0x8f},std::byte{0x1f},std::byte{0xad},std::byte{0x23},
  std::byte{0x14},std::byte{0xe0},std::byte{0x14},std::byte{0xe0}};

// Literal4047C4..40487F producer. Bank is source879E78's first12 bytes:
// PBuffer word+0, event-enabled byte+4, event handle word+8. Other bytes retain.
// All services are mandatory. An unavailable native licensing provider must
// reject/throw at its actual boundary, never return a successful placeholder.
// Ends immediately before42FD23. Returned bool is whether the producer found
// a licensed status row; original caller's PBuffer byte0 is the inverse.
bool vista_mainmenu_platform_gate_initialize(std::span<std::byte> bank,
    const VistaMainmenuPlatformGateServices&);
} // namespace halo2
