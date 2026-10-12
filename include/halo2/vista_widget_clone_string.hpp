#pragma once
#include "halo2/vista_widget_registration.hpp"

namespace halo2 {
// Explicit source32 byte banks; identities are registry data, never cast host
// pointers. UTF16 units use source little-endian bytes, including surrogates.
struct VistaUtf16View {
  std::uint32_t identity{};
  std::span<const std::byte> bytes;
};
struct VistaUtf16Bank {
  std::uint32_t identity{};
  std::span<std::byte> bytes;
};
struct VistaUtf16FailureServices {
  std::function<void(std::uint32_t)> store_errno; //68EE22 result DWORD write.
  std::function<void(std::array<std::uint32_t,5>)> invalid_parameter; //68EDC3.
};
// Complete reached687AB5 countFFFFFFFF path. Zero/invalid pointer cases use
// genuine mandatory error services if reached. No tail fill; forward reads and
// writes retain overlap behavior. Returns exact internal0/16h/50h status.
std::uint32_t vista_utf16_copy_truncate(VistaUtf16Bank, VistaUtf16View,
    std::uint32_t capacity_units, const VistaUtf16FailureServices& = {});
// Complete44BD27 wrapper: discards the CRT status and returns destination.
std::uint32_t vista_utf16_copy(VistaUtf16Bank, VistaUtf16View,
    std::uint32_t capacity_units, const VistaUtf16FailureServices& = {});
// Complete44BC9A; returns the original arithmetic result, not Unicode points.
std::uint32_t vista_utf16_length(VistaUtf16View);
// Complete643EF5 and643E66 pure source32 LEA arithmetic (including wrap).
VistaWidgetIdentity vista_widget_clone_style_identity(VistaWidgetIdentity);
std::uint32_t vista_widget_clone_text_identity(VistaWidgetIdentity);
struct VistaCloneStringServices {
  std::function<std::span<std::byte>(VistaWidgetIdentity)> bank;
  // Actual style virtualC, queried after copying. The required clone binds
  //643E66 above and its retained bank. It can mutate banks before the length
  //read, and must retain the returned text bank through that read.
  std::function<VistaUtf16View(VistaWidgetIdentity)> text;
  VistaUtf16FailureServices failure;
};
// Complete643DDB. Owns the1024-unit buffer at style+44 and countWORD3E;
//signed-negativeWORD3C is preserved, otherwise it is reset after count store.
//Returns the original EAX length. Full consumed style extent is844h bytes.
std::uint32_t vista_widget_clone_string_assign(VistaWidgetIdentity,
    VistaUtf16View input, const VistaCloneStringServices&);
} //namespace halo2
