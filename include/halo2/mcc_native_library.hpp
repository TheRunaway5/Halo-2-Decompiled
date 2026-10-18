#pragma once
#include "halo2/mcc_native_shaders.hpp"
#include <filesystem>
#include <map>
#include <nlohmann/json_fwd.hpp>
#include <string>

namespace halo2 {
// Admit only the audited, already retained native template closure. Existing
// records are never replaced. Returned paths own only newly appended payloads;
// ordinary payload validation and typed relocation remain with Cache.
std::map<std::string,std::filesystem::path> supplement_mcc_native_library(
    nlohmann::json& library,const std::filesystem::path& native_library_root);
// Only the two authenticated PC-exclusive source maps without a retained
// manifest may bootstrap this three-record HUD closure. Nothing is published
// here: source owners, the complete retained bank and all payloads are checked
// before replacing library. Unknown/custom inputs leave it unchanged.
// The result deliberately has no rasterizer/globals record.
std::map<std::string,std::filesystem::path> bootstrap_mcc_native_hud_library(
    nlohmann::json& library,const std::filesystem::path& source_map,
    std::span<const Tag> directory,const MccShaderRead&,
    const std::function<std::string(std::uint32_t)>& string_id,
    const std::filesystem::path& native_library_root);
}
