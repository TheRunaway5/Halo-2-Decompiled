#pragma once
#include "halo2/bsp_atmosphere.hpp"

namespace halo2 {
// The mainmenu's one-sky title path retains its authored textures and motion
// parameters. Original12E5E0 supplies these camera/portal-weighted values to
// the fog constants and27AA0 haze producer on every frame.
inline void update_menu_fog_inputs(MenuAtmosphere &menu, const BspFogSample &sample) {
    if(sample.cluster<0 || !sample.patchy || !menu.patchy)
        throw FormatError("title fog state requires its authored camera cluster and haze definition");
    menu.atmospheric=sample.primary;
    menu.patchy->color=sample.patchy->color;
    menu.patchy->density=sample.patchy->density;
    menu.patchy->distance=sample.patchy->distance;
}
} // namespace halo2
