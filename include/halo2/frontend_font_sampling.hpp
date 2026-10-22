#pragma once
#include <array>
#include <cstddef>
#include <filesystem>
#include <string_view>

namespace halo2 {
struct RetailFont;

// Optional raster-source mode. Keep every original layout/ink/kerning field;
// append sampled pixels only for admitted, same-codepoint source glyphs.
std::size_t apply_retail_font_sampling(RetailFont &original,
                                     const RetailFont &sampled);

// Explicit MCC Classic import: retain logical metrics, sample each authored
// same-codepoint glyph, including controls and equal-size source rasters.
std::size_t apply_mcc_classic_font_sampling(RetailFont &, const RetailFont &);
bool is_mcc_classic_hud_pack(const std::filesystem::path &);
bool is_mcc_classic_menu_pack(const std::filesystem::path &);
std::filesystem::path ui_sampled_font_path(const std::filesystem::path &pack,
    unsigned selector, std::string_view original_name);
std::filesystem::path hud_sampled_font_path(const std::filesystem::path &pack,
    unsigned selector, std::string_view original_name);

// Optional, separately authored Vista prompt overlay. Existing source metrics
// and ordinary MCC glyph samples remain intact; only control UVs are replaced.
std::size_t apply_vista_prompt_sampling(RetailFont &, const std::filesystem::path &pack,
                                      unsigned selector);

// Font owners are authenticated by the caller. Imported MCC uses its native
// font glyphs, even when an older cache retains an unused authored overlay.
// Explicit non-MCC reference/custom artwork packs retain their binding.
std::filesystem::path font_prompt_asset_directory(
    const std::filesystem::path &font_directory,
    const std::filesystem::path &explicit_pack = {});

// Admit source-traced vector rasters after checking donor identity, lookup
// records, integer physical density, and unchanged layout metrics.
std::size_t apply_retail_font_sampling(RetailFont &original,
                                     const RetailFont &sampled,
                                     const RetailFont &donor);

// Input is the original logical source rectangle AFTER source clipping and
// integer truncation. Result is {u0,v0,u1,v1} in the combined sampled atlas.
std::array<float, 4> retail_font_sample_uvs(const RetailFont &, unsigned codepoint,
                                         int source_x, int source_y,
                                         int width, int height);
} // namespace halo2
