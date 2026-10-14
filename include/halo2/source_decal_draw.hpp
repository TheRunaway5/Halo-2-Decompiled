#pragma once
#include "halo2/source_decal_vertex_cache.hpp"
#include "halo2/source_bsp_admission.hpp"
#include "halo2/vertex_program.hpp"
#include <memory>

namespace halo2 {
struct SourceDecalLayerState {
    PixelProgram pixel;
    std::uint32_t source_blend{},destination_blend{},blend_equation=0x8006;
    std::uint32_t color_mask=0x10101,cull=0x901,depth_function=0x203;
    float depth_bias=-200;
};
// TU1.5 43A30 ordinary authored layers0..5. Other layers require their own state.
SourceDecalLayerState source_decal_layer_state_tu15(unsigned layer);
struct SourceDecalDraw {
    std::uint32_t datum{},primary{},cache_datum{};
    unsigned layer{};
    const Texture *texture{};
    ShaderVector color{};
    SourceDecalLayerState state;
    std::vector<std::array<SourceDecalCacheVertex,4>> quads;
    // Current original view/fog constants; no captured camera bank is loaded.
    std::array<ShaderVector,192> constants{};
    ShaderVector viewport{};
    ShaderVector clip_range{};
};
// Resources are prepared once. Each completed source update supplies actual
// clipped cache pages and retained lifetimes; no triangle projector runs here.
class SourceDecalScene {
public:
    explicit SourceDecalScene(const Cache &);
    ~SourceDecalScene();
    std::vector<SourceDecalDraw> draw(SourceRuntimeMemory &,
        const SourceBspAdmission &,const std::array<ShaderVector,192> &,
        ShaderVector viewport,ShaderVector clip_range) const;
    bool active() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
