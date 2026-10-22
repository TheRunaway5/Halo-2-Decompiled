#pragma once
#include "halo2/mcc_native_map_owner.hpp"
#include <map>

namespace halo2 {

// Local admission result, retained across MCC decoding by the caller. No Cache
// field or serialized identity changes. Metadata values own their exact bytes,
// including binary scalar-table snapshots when named by the owner index.
struct MccPreparedMapOwner {
    std::filesystem::path cache_root;
    std::optional<MccNativeMapBinding> binding;
    std::map<std::string, std::string> metadata;
    bool indexed{};
};

// The caller supplies its ALREADY read final encoded map, before MCC decoding.
// Inference is restricted to an immediate halo2/h2_maps_win64_dx11 map with
// that exact directory as its explicit shared owner. Arbitrary/nested paths
// need an explicit prepared root. Without an inferred preparation, returns
// nullopt; an explicit root or declared owner with bad metadata throws.
//
// A no-index legacy preparation only projects exact immediate stock owners.
// Nested/mod maps never inherit a stock bank by basename. The returned library
// JSON snapshots, rather than reopened filenames, must reach the existing
// shader/bootstrap/closure validators. This authenticates metadata ownership,
// not program/payload closure, conversion completeness or gameplay readiness.
std::optional<MccPreparedMapOwner> admit_mcc_prepared_map_owner(
    const std::filesystem::path& map,
    const std::filesystem::path& explicit_shared_directory,
    std::span<const std::byte> admitted_encoded_source,
    std::optional<std::filesystem::path> explicit_prepared_root = std::nullopt);

}
