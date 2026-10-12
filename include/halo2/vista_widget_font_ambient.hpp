#pragma once
#include "halo2/vista_widget_font.hpp"

namespace halo2 {
// Compose the separately frozen 498F61 body in the caller's actual masked
// x87/MXCSR environment. Source32 banks and genuine lower providers are the
// same as VistaWidgetFontServices; its declared FP fields are not used here.
// The terminal 431858 provider is the last source FP boundary. Its observed
// environment must survive the frozen body's host-context restoration.
std::uint8_t vista_font_iterator_birth_ambient(std::uint32_t state_identity,
    std::span<std::byte,0x44>,std::uint32_t vector_identity,
    std::array<std::uint32_t,6>,const VistaWidgetFontServices&);
// Complete frozen 498B74 is integer-only except genuine callback effects;
// this adapter never resets/restores the caller's floating-point environment.
std::uint32_t vista_font_iterator_next_ambient(std::uint32_t state_identity,
    std::span<std::byte,0x44>,const VistaWidgetFontServices&);
} //namespace halo2
