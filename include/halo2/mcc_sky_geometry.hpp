#pragma once
#include "halo2/model.hpp"
#include <optional>
#include <string>
#include <string_view>

namespace halo2 {
// Finite, authenticated original-asset projection. This contains decoded data,
// never Xbox instructions or a run-time decoder for another graphics API.
std::string mcc_sky_geometry_input_sha256(const Cache &,std::uint32_t model);
std::string mcc_sky_geometry_sha256(std::span<const std::byte>);
// Exact finite derived-input receipt. This replays only source-declared pointer
// relocation words and requires the original complete material payload hash.
bool mcc_sky_material_relocation_matches(std::string_view model_input,
    std::uint32_t shader,std::string_view name,std::string_view original_payload_sha256,
    std::span<const std::byte> payload);
std::vector<std::byte> encode_mcc_sky_geometry(const RenderModel &);
RenderModel decode_mcc_sky_geometry(std::span<const std::byte>);
// Shared admission seam for the compiled catalog and offline negative controls.
// A profile is immutable build input; this accepts no path or asset translator.
std::string_view mcc_sky_geometry_profiles();
bool restore_mcc_sky_geometry_profile(const Cache &,std::uint32_t model,RenderModel &,
    bool textures,std::string_view profile_json,std::optional<unsigned> geometry_lod={},
    std::optional<std::span<const unsigned>> resident_sections={});

// The complete input model/resources select a compiled profile. Unrelated
// models retain their ordinary native decoder. A selected profile is atomic:
// stale geometry, mappings or source output are errors, never partial fixes.
bool restore_mcc_sky_geometry(const Cache &,std::uint32_t model,RenderModel &,
    bool textures,std::optional<unsigned> geometry_lod={},
    std::optional<std::span<const unsigned>> resident_sections={});
} // namespace halo2
