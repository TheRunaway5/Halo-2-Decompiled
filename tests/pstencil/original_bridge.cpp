// Narrow C ABI for comparing the native port with unchanged original i386.
#include "halo2/pstencil/shadow.hpp"
#include "halo2/pstencil/caster.hpp"
#include <algorithm>

extern "C" std::uint32_t pstencil_shadow_casting_part(std::uint16_t type) {
    return halo2::pstencil::shadow_casting_part(type);
}
extern "C" std::uint32_t pstencil_shadow_stipple_level(std::uint8_t opacity) {
    return halo2::pstencil::shadow_stipple_level(opacity);
}

extern "C" std::uint32_t pstencil_stitch_quad(std::uint16_t a,std::uint16_t b,
    std::uint32_t facing_a,std::uint32_t facing_b,std::uint16_t* indices) {
    const auto quad=halo2::pstencil::stitch_quad(a,b,facing_a!=0,facing_b!=0);
    std::copy(quad.indices.begin(),quad.indices.end(),indices);
    return quad.count;
}
extern "C" std::uint32_t pstencil_shadow_states(std::uint32_t* words) {
    const auto states=halo2::pstencil::shadow_stencil_states();
    for(std::size_t i=0;i<states.size();++i) {
        words[i*2]=states[i].index;words[i*2+1]=states[i].value;
    }
    return std::uint32_t(states.size());
}
extern "C" std::uint32_t pstencil_shadow_clear() {
    return halo2::pstencil::shadow_stencil_clear;
}
extern "C" std::uint32_t pstencil_lightmap_shadow_rgb() {
    return halo2::pstencil::lightmap_shadow_rgb;
}
extern "C" std::uint32_t pstencil_lightmap_shadow_states(std::uint32_t* words) {
    const auto states=halo2::pstencil::lightmap_shadow_states();
    for(std::size_t i=0;i<states.size();++i) {
        words[i*2]=states[i].index;words[i*2+1]=states[i].value;
    }
    return std::uint32_t(states.size());
}
extern "C" float pstencil_extrusion_distance() {
    return halo2::pstencil::shadow_extrusion_distance;
}
extern "C" std::uint32_t pstencil_light_passes(std::uint32_t shadows,std::uint32_t* words) {
    const auto passes=halo2::pstencil::light_passes(shadows!=0);
    for(std::size_t i=0;i<passes.size();++i) {
        words[i*2]=passes[i].scene;words[i*2+1]=std::uint32_t(passes[i].phase);
    }
    return std::uint32_t(passes.size());
}
