#pragma once
#include "halo2/cache.hpp"

namespace halo2::pstencil {
// May2003 default_light's resident state43 is RGB-only 00010101; its
// separate utility/texture targets are not overwritten by the light sum.
// This hybrid retains the current base alpha for the authored specular mask.
inline ForwardRasterState light_accumulation_state(ForwardRasterState state) {
    state.color_write&=0x00ffffffu;
    return state;
}
}
