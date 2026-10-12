#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>

namespace halo2 {
struct VistaWidgetBoundsServices {
  // Ordinary resident source32 memory, never narrowed host addresses. The
  // caller owns all borrowed widget/output/viewport banks and source globals.
  // Writes are immediately visible to later reads, including overlapping banks.
  std::function<std::uint32_t(std::uint32_t,std::size_t)> read;
  std::function<void(std::uint32_t,std::uint32_t,std::size_t)> write;
};

// Complete47708E: add dx to WORDs+2/+6, then dy to WORDs+0/+4, preserving
// individual live reads/writes and low-WORD overflow. No floating-point input.
std::uint32_t vista_widget_translate_short_rectangle(std::uint32_t rectangle,
    std::uint32_t dx,std::uint32_t dy,const VistaWidgetBoundsServices&);

// Original 611703/47708E: copy raw short rectangle, truncate animated XY using
// CVTTSS2SI, then add each low WORD with original overflow/write ordering.
// Each API returns the borrowed output identity, including after aliases.
std::uint32_t vista_widget_short_bounds(std::uint32_t widget,
    std::uint32_t output,const VistaWidgetBoundsServices&);

// Original 61174B: signed WORD -> float conversion and individual SSE additions.
std::uint32_t vista_widget_float_bounds(std::uint32_t widget,
    std::uint32_t output,const VistaWidgetBoundsServices&);

// Original 6117E0 including actual 61D162/61D05F/6096A0 projection. The viewport
// is the caller's borrowed four-WORD rectangle; scale E3E424 and cached float
// D718F8 remain actual source memory inputs. No guessed screen/aspect defaults.
std::uint32_t vista_widget_projected_bounds(std::uint32_t widget,
    std::uint32_t viewport,std::uint32_t output,const VistaWidgetBoundsServices&);

// Complete 61D162, also useful for callers with an existing float rectangle.
std::uint32_t vista_widget_project_float_rectangle(std::uint32_t rectangle,
    std::uint32_t z_bits,std::uint32_t viewport,const VistaWidgetBoundsServices&);

// These owners require x86 SSE/x87 and masked exceptions. The caller's MXCSR
// rounding/DAZ/FTZ and x87 control word are retained. Registry faults preserve
// completed writes. There are no font, renderer or animation substitutes here.
} // namespace halo2
