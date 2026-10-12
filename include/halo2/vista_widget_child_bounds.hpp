#pragma once
#include "halo2/vista_widget_registration.hpp"

namespace halo2 {
using VistaWidgetChildBounds=std::array<std::byte,8>;
struct VistaWidgetChildBoundsServices {
  // Source32 storage stays alive across every provider. The raw read primitive
  // consumes real flag bytes at the retained child+70 record pointer.
  std::function<std::span<std::byte>(VistaWidgetIdentity)> bank;
  std::function<std::uint32_t(std::uint32_t,std::size_t)> read;
  // Required real child virtual+20, returning its complete eight output bytes.
  // No authored-bounds, animation or resource-ready defaults are supplied.
  std::function<VistaWidgetChildBounds(VistaWidgetIdentity)> bounds;
};
// Complete61C344/640F86 skip leaves: null record includes the child; otherwise
// bit0 is read at BYTE width for bitmap(kind8), DWORD width for kind9.
std::uint8_t vista_widget_bitmap_bounds_excluded(VistaWidgetIdentity,
    const VistaWidgetChildBoundsServices&);
std::uint32_t vista_widget_kind9_bounds_excluded(VistaWidgetIdentity,
    const VistaWidgetChildBoundsServices&);
// Complete6425BF. Four WORD sentinels are stored in original order, followed
// by live direct-child traversal. Only kinds8/9 use the skip leaves. Included
// bounds merge in left,right,top,bottom order with signed WORD comparisons.
// Signed WORD width/height subtraction can wrap; an invalid result clears
// only the first DWORD. Empty-child output is the original sentinel rectangle.
// Output may alias retained banks; stores and subsequent link reads keep their
// source order. Missing consumed services or invalid identities reject.
void vista_widget_child_bounds(VistaWidgetIdentity,std::span<std::byte,8> output,
    const VistaWidgetChildBoundsServices&);
} //namespace halo2
