#pragma once
#include "halo2/mcc_original_sound_runtime_delivery_tu15.hpp"

namespace halo2 {
class Cache;
// Immutable asset admission only. This binds an exact converted Cache snapshot
// to authenticated original banks and actual native import names. It creates
// no World, memory epoch, cache handle, voice or device eligibility.
class PreparedOriginalSoundCatalogBindingTu15 final {
    struct Impl;
    std::shared_ptr<const Impl> impl_;
    explicit PreparedOriginalSoundCatalogBindingTu15(std::shared_ptr<const Impl>);
public:
    // The input's immutable string owner also retains its binding storage.
    // Dropping this wrapper cannot invalidate the returned span.
    MccOriginalSoundRuntimeInputTu15 input(const Cache& expected_owner)const;
    std::string_view original_context()const;
    std::size_t size()const;
    friend class PreparedOriginalSoundCatalogTu15;
};
class PreparedOriginalSoundCatalogTu15 final {
    struct Impl;
    std::shared_ptr<const Impl> impl_;
    explicit PreparedOriginalSoundCatalogTu15(std::shared_ptr<const Impl>);
public:
    // The expected catalog digest comes from the admitting asset producer.
    // Catalog paths are relative owned bank directories, with no reference
    // paths into the producer's machine or dependence on its scratch layout.
    static std::shared_ptr<const PreparedOriginalSoundCatalogTu15> load(
        const std::filesystem::path& directory,std::string_view expected_catalog_sha256);
    // Select by complete normalized source-runtime bytes, then require the
    // exact runtime filename. No basename-only or unknown-map fallback exists.
    // The Cache must outlive uses of the bound input; its borrowed address is
    // equality only, not an ownership or native-memory token.
    std::shared_ptr<const PreparedOriginalSoundCatalogBindingTu15> bind(const Cache&)const;
};
}
