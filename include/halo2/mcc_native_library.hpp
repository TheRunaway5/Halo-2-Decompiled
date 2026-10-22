#pragma once
#include "halo2/mcc_native_shaders.hpp"
#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <nlohmann/json_fwd.hpp>
#include <string>
#include <string_view>

namespace halo2 {
// A prepared-cache caller supplies its already admitted metadata snapshots.
// The callback must reject any requested path outside that admitted set.
using MccNativeLibraryRead = std::function<std::string(const std::filesystem::path&)>;
// Admit only the audited, already retained native template closure. Existing
// records are never replaced. Returned paths own only newly appended payloads;
// ordinary payload validation and typed relocation remain with Cache.
std::map<std::string,std::filesystem::path> supplement_mcc_native_library(
    nlohmann::json& library,const std::filesystem::path& native_library_root,
    std::span<const Tag> runtime_directory={});
std::map<std::string,std::filesystem::path> supplement_mcc_native_library(
    nlohmann::json& library,const std::filesystem::path& native_library_root,
    std::span<const Tag> runtime_directory,const MccNativeLibraryRead&);
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
// These are the encoded source's already admitted fingerprint and extent,
// captured before Cache decodes or relocates it. All existing compiled policy,
// directory, region, string and native-program checks still apply.
std::map<std::string,std::filesystem::path> bootstrap_mcc_native_hud_library(
    nlohmann::json& library,const std::filesystem::path& source_map,
    std::span<const Tag> directory,const MccShaderRead&,
    const std::function<std::string(std::uint32_t)>& string_id,
    const std::filesystem::path& native_library_root,std::uint64_t admitted_bytes,
    std::string_view admitted_sha256,const MccNativeLibraryRead&);
}
