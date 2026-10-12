#pragma once
#include "halo2/cache.hpp"
#include "halo2/transform.hpp"
namespace halo2 {
// Retail 001305D0/00130DE0 camera-dependent planar classification and
// 00025A10 c12..c17. The menu has one sky and one visible planar-fog layer.
// Values are original vertex-program inputs, not a replacement screen tint.
std::array<ShaderVector,6> menu_fog_constants(
    const MenuAtmosphere &, Vec3 eye, Vec3 forward);
struct MenuFogComposite {
    PixelProgram program;
    std::array<ShaderVector,2> lookup_coordinates{};
};
// Retail 00025E50 and its 0003A5C0 mode-1 fast path, including packed
// constants and quantized depth ranges. The camera far clip is 00467018.
MenuFogComposite menu_fog_composite(const MenuAtmosphere &, Vec3 eye,
                                   float far_clip = 1024.f);
// 25D50 dispatch for the bounded retail menu atmosphere: classification 5
// uses 25E50; classification 3 uses the far-depth-only 26520 eye-fog pass.
// A zero eye opacity skips that pass entirely.
std::optional<MenuFogComposite> menu_fog_draw(const MenuAtmosphere &, Vec3 eye,
                                            float far_clip = 1024.f);
struct MenuFogExterns {
    std::array<float,43> scalars{};
    std::array<ShaderVector,42> colors{};
};
// 336F0/33980 fog inputs after 130DE0, including the distinct transition
// and planar-eye fractions. Pixel packing remains the original 19CA0 port.
MenuFogExterns menu_fog_externs(const MenuAtmosphere &, Vec3 eye);
PixelProgram menu_planar_fog_program(const MaterialPass &, const MenuAtmosphere &, Vec3 eye);
// Every cluster in this BSP resolves 16E210 to the same fog palette datum;
// 1332F0/4CBF0 consequently choose implementation 0 for its layer 16.
// Also admitted for the carrier body, whose original draws 1880/1885 select
// that same variant. Other scene objects require their own selection evidence.
void prepare_menu_bsp_fog(const Cache &, Mesh &);
struct PatchyFogState {
    std::uint32_t initialized{};
    Transform previous;
    std::uint32_t layer{};
    float rotation{}, fraction{}, time{};
    std::array<std::array<float,2>,4> offsets{};
    std::array<float,8> wind{};
};
struct PatchyFogFrame {
    std::array<ShaderVector,4> weights{};
    std::array<ShaderVector,8> rows{};
    float vertical{};
    PixelProgram noise, cached, direct;
};
// Completed 26CA0 attributes in 34420's corner order. Keep the float4 UVs
// through interpolation; evaluating the affine rows per fragment rounds at
// different points from the original CPU callback and vertex shader.
struct PatchyFogQuad {
    std::array<ShaderVector,20> attributes{}; // position, then four UV streams.
    ShaderVector clip_range{},surface{}; // width/height, inline D0/D1 alpha.
};
static_assert(sizeof(PatchyFogQuad)==352);
// The title's captured Vulkan constants are expanded on the CPU before
// interpolation/combiner arithmetic. GPU reciprocal folding changes some
// byte/255 results by one ULP and can change the final blended framebuffer.
std::array<ShaderVector,18> patchy_fog_combiner_constants(const PixelProgram &);
struct PatchyFogUniform {
    std::array<std::uint32_t,60> program{};
    std::array<ShaderVector,8> rows{};
    ShaderVector controls{},lookup{};
    std::array<ShaderVector,18> combiner_constants{};
};
static_assert(sizeof(PatchyFogUniform)==688);
PatchyFogQuad patchy_fog_noise_quad(const PatchyFogFrame &,float width,float height,
    float near_clip=.015625f,float far_clip=1024.f);
// 27520's title direct-composition branch: screen/depth alias coordinates,
// two separately interpolated depth-gradient rows and target-19 coordinates.
PatchyFogQuad patchy_fog_direct_quad(const PatchyFog &,float width,float height,
    float noise_width,float noise_height,float near_clip=.015625f,float far_clip=1024.f);
// The Vulkan reference expands BC1 endpoints to integer RGBA8 before filtering.
// Native hardware BC1 sampling differs by up to three bytes in the captured
// title noise pass. This bounded decode preserves the reference's texels/mips;
// it does not change compressed material textures in other rendering paths.
Texture prepare_menu_noise_texture(const Texture &);
// 27AA0, 26880 and 26E50; random state is the original unsynchronized LCG.
// 27AA0 divides the viewport's bottom-top by right-left: height / width.
PatchyFogFrame patchy_fog_frame(const PatchyFog &, PatchyFogState &,
    std::uint32_t &random_state, const Transform &world_to_view,
    float forward_z,float vertical_fov,float height_over_width,float time);
}
