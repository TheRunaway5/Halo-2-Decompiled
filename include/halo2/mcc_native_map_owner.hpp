#pragma once
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>

namespace halo2 {

enum class MccCacheRole { Scenario, Resources };
enum class MccNativeBankMode { Owned, AuthenticatedHudBootstrap, None };

class MccNativeMapOwnerError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// expected_index_sha256 must be authenticated independently by the caller's
// provenance/producer authority, not calculated from an untrusted index here.
// There is deliberately no inferred no-index/legacy stock fallback.
struct MccNativeMapOwnerContext {
    std::filesystem::path cache_root;
    std::string_view encoded_index;
    std::string_view expected_index_sha256;
};

// Bytes already admitted by the caller, including exact library.json metadata
// or scalar-table snapshots. Paths establish a private filesystem owner only;
// this resolver never reopens them to replace the supplied snapshot bytes.
struct MccNativeMetadataSnapshot {
    std::string_view relative_path;
    std::span<const std::byte> bytes;
};

// All strings/paths own their storage. This adds no Cache field, serialized
// datum or shader-program contract. Whole-program/typed closure validation and
// TU1.5 semantic/readiness admission remain with their existing owners.
struct MccNativeMapBinding {
    std::string map_relative;
    std::string source_sha256;  // Original copied MCC input, not Xbox library provenance.
    std::string input_sha256;   // Final encoded snapshot, after ordered producer changes.
    std::uint64_t input_bytes{};
    MccCacheRole role{MccCacheRole::Scenario};
    MccNativeBankMode mode{MccNativeBankMode::None};
    bool custom_authoring{}; // Authenticated explicit-custom-owner receipt.
    std::filesystem::path shared_directory, bank_directory;
    std::string library_namespace, library_sha256;
    std::string bootstrap_policy_sha256, bootstrap_source_canonical_sha256;
    std::filesystem::path bootstrap_dependency;
};

// Authenticate an already-derived owner index, exact private paths and the
// ALREADY admitted encoded map bytes. The caller supplies final input size/SHA
// and metadata authority through that index; source/conversion/graphics-chain
// derivation belongs to the producer. No actual map/library content is read.
// Missing exact owner returns nullopt, while a declared malformed/stale binding
// throws MccNativeMapOwnerError. No basename/case-fold bank selection exists.
std::optional<MccNativeMapBinding> resolve_mcc_native_map_binding(
    const MccNativeMapOwnerContext&,
    const std::filesystem::path& map,
    const std::filesystem::path& explicit_shared_directory,
    std::span<const std::byte> admitted_encoded_source,
    std::span<const MccNativeMetadataSnapshot> admitted_metadata);

}
