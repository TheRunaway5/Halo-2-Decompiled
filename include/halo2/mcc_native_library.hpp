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

// Extend only the already authenticated three-record PC HUD bootstrap with
// the complete nine paired sky materials and their retained Xbox programs.
// Whole input, typed owners, every borrowed bank/function body and complete
// source library/payload are authenticated before any record is published.
std::map<std::string,std::filesystem::path> supplement_mcc_native_pc_sky_library(
    nlohmann::json& library,const std::filesystem::path& source_map,
    std::span<const Tag> directory,const MccShaderRead&,
    const std::filesystem::path& native_library_root,std::uint64_t admitted_bytes,
    std::string_view admitted_sha256,const MccNativeLibraryRead&);

// Add only the finite original six opaque template/program closures to the
// fully authenticated PC HUD/sky bootstrap. Material instances still require
// the existing complete cooked-tuple projectors; no PC program is interpreted.
std::map<std::string,std::filesystem::path> supplement_mcc_native_pc_world_library(
    nlohmann::json& library,const std::filesystem::path& source_map,
    const std::filesystem::path& native_library_root,std::uint64_t admitted_bytes,
    std::string_view admitted_sha256,const MccNativeLibraryRead&);

// Additional source-proved world profiles extend only the complete preceding
// HUD/sky/six-profile bank. Complete original source/program ownership is
// authenticated atomically; cooked material admission remains independent.
std::map<std::string,std::filesystem::path> supplement_mcc_native_pc_world_programs(
    nlohmann::json& library,const std::filesystem::path& source_map,
    const std::filesystem::path& native_library_root,std::uint64_t admitted_bytes,
    std::string_view admitted_sha256,const MccNativeLibraryRead&);
// Finite complete-TU vehicle/material restoration for the two authenticated
// PC map inputs. Program storage names are donor scoped; original labels are
// returned separately and become trusted only after Cache installs the batch.
struct MccNativeVehicleLibrary {
    std::map<std::string,std::filesystem::path> payloads;
    std::map<std::pair<std::string,std::string>,std::string> semantic_names;
};
MccNativeVehicleLibrary supplement_mcc_native_vehicle_library(
    nlohmann::json&,const std::filesystem::path& source_map,std::span<const Tag>,
    const MccShaderRead&,const MccShaderBitmapFrame&,unsigned source_string_count,
    const std::function<std::string(std::uint32_t)>& string_id,
    const std::filesystem::path& native_library_root,std::uint64_t admitted_bytes,
    std::string_view admitted_sha256,const MccNativeLibraryRead&);

}
