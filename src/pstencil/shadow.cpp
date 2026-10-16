#include "halo2/pstencil/shadow.hpp"

namespace halo2::pstencil {
StitchQuad stitch_quad(std::uint16_t first,std::uint16_t second,
                      bool first_face,bool second_face) {
    const auto a=std::uint16_t(std::uint32_t(first)*2u);
    const auto b=std::uint16_t(std::uint32_t(second)*2u);
    const auto af=std::uint16_t(std::uint32_t(a)+1u);
    const auto bf=std::uint16_t(std::uint32_t(b)+1u);
    const auto face_a=std::uint16_t(0u-std::uint32_t(first_face));
    const auto face_b=std::uint16_t(0u-std::uint32_t(second_face));
    return {{{std::uint16_t((af&face_b)|(b&face_a)),a,
              std::uint16_t((af&face_a)|(b&face_b)),bf}},
            std::uint32_t(first_face!=second_face)*4u};
}

std::span<const RenderState> shadow_stencil_states() {
    static constexpr std::array states{
        RenderState{0x43,0},RenderState{0x3b,0},RenderState{0x3c,0},
        RenderState{0x8f,2},RenderState{0x40,0},RenderState{0x39,0x201},
        RenderState{0x95,0},RenderState{0x52,0x10},RenderState{0x90,1},
        RenderState{0x91,0x1e00},RenderState{0x44,0x1e00},RenderState{0x45,0x1e00},
        RenderState{0x46,0x207},RenderState{0x47,1},
        RenderState{0x48,0xff},RenderState{0x49,0xff}};
    return states;
}

std::span<const LightPass> light_passes(bool shadows) {
    static constexpr std::array passes{
        LightPass{2,LightPhase::Shadows},LightPass{1,LightPhase::Utility},
        LightPass{1,LightPhase::Lighting}};
    return std::span(passes).subspan(shadows?0:1);
}
std::span<const RenderState> lightmap_shadow_states() {
    static constexpr std::array states{
        RenderState{0x43,0x1010101},RenderState{0x3b,1},RenderState{0x3e,0},
        RenderState{0x3f,0x300},RenderState{0x4a,0x8006},RenderState{0x3c,0},
        RenderState{0x39,0x207},RenderState{0x93,0},RenderState{0x90,1},
        RenderState{0x91,0x1e00},RenderState{0x44,0x1e00},RenderState{0x45,0x1e00},
        RenderState{0x46,0x205},RenderState{0x47,0x80}};
    return states;
}
} // namespace halo2::pstencil
