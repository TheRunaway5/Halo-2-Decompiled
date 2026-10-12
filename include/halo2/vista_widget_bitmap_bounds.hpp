#pragma once
#include "halo2/vista_widget_bounds.hpp"

namespace halo2 {
// Complete original bitmap virtual+20 (61C364). The genuine611703/47708E
// offset prefix is shared with the other bounds owners. After that, live
// scales at+84/+88 are consumed in source order using individual SSE scalar
// operations. CVTTSS2SI and low-WORD stores retain overflow/NaN behavior.
// Output and widget storage may overlap; no authored/resource bounds are
// substituted. Requires the masked x86 floating-point context admitted by
// vista_widget_short_bounds; rounding/DAZ/FTZ are caller-owned.
std::uint32_t vista_widget_bitmap_bounds(std::uint32_t widget,
    std::uint32_t output,const VistaWidgetBoundsServices&);
} //namespace halo2
