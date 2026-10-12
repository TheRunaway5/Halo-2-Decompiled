#pragma once
#include "halo2/cache.hpp"
#include <filesystem>
#include <map>

namespace halo2 {
struct FrontendContent;
struct UiImage;
// Optional texture pages only. Original content owns all geometry, banks,
// sprite rectangles and navigation. MCC menu libraries also supply verified
// framing pages; native horizontal extensions sample that same selected page.
using FrontendUiAssetPack = std::map<std::string,std::vector<Texture>>;
FrontendUiAssetPack load_frontend_ui_assets(const FrontendContent &,
                                           const std::filesystem::path &);
// MCC imports bind the separately selected original pause/menu metadata source.
std::filesystem::path hud_menu_asset_directory(const std::filesystem::path &);
// Setup-generated V2 libraries may contain banks for other screens/maps.
// Only existing source-local images are admitted; their logical data survives.
FrontendUiAssetPack load_ui_asset_library(const std::map<std::uint32_t,UiImage> &,
    const std::filesystem::path &, const char *manifest);
// Setup records artwork-local UV corrections from the donor's relative HUD
// layer placement. The existing widget geometry and anchors remain unchanged.
using UiAssetSampleOffsets = std::map<std::pair<std::string,unsigned>,std::array<float,2>>;
UiAssetSampleOffsets load_hud_asset_sample_offsets(const FrontendUiAssetPack &,
    const std::filesystem::path &);
} // namespace halo2
