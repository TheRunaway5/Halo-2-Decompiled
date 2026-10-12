#pragma once
#include "halo2/vista_loading_assets.hpp"
#include "halo2/vista_loading_geometry.hpp"
#include <span>

namespace halo2 {
// Renderer-resident textures and ordered triangle expansion for the original
// normal-mode loading draw stream. This adapter owns no loading lifetime,
// clock, progress publisher, audio owner or frontend/world transition.
class NativeLoadingPresentation {
public:
    explicit NativeLoadingPresentation(const VistaLoadingAssets&);
    const Mesh& resident_mesh() const { return mesh_; }
    std::span<const Material> materials() const { return mesh_.materials; }
    std::span<const Vertex> vertices() const { return vertices_; }
    // Default raster translation is the measured Wine/DXVK-to-Vulkan mapping.
    // A replay may supply another framebuffer-pixel offset explicitly.
    void update(std::span<const VistaLoadingDraw>,std::uint32_t width,
                std::uint32_t height,Vec2 raster_offset_pixels={.5f,.5f});
private:
    Mesh mesh_;
    std::vector<Vertex> vertices_;
};
}
