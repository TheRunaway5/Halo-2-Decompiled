#pragma once
#include "halo2/cache.hpp"
#include <array>
#include <filesystem>

namespace halo2 {
struct VistaLoadingImage {
  // Source66B95D retains D3DXIMAGE_INFO dimensions independently of the
  // actual allocated texture dimensions. Geometry must use the former.
  std::uint32_t source_width{}, source_height{};
  Texture texture;
};
using VistaLoadingAssets = std::array<VistaLoadingImage,12>;
// Exported real D3DX9_31 results; this reader does not approximate BMP decoding
// or publish loading progress, resource readiness or loading-screen lifetime.
VistaLoadingAssets load_vista_loading_assets(const std::filesystem::path&);
}
