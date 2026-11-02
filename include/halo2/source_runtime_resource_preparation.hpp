#pragma once
#include <memory>
namespace halo2 {
class Cache;
struct MccOriginalSoundRuntimeInputTu15;
class PreparedOriginalSoundCatalogBindingTu15;
// One match-scoped preparation of the exact authenticated resident/shared
// resource snapshots. The first SourceRuntimeResources for this same Cache
// consumes them; descriptor translation is still bound by the real executor.
// This owns no source memory, player, simulation, random or clock state.
class SourceRuntimeResourcePreparation {
public:
    // A provenance-admitted Cache catalog selects the original-bank path
    // automatically; ordinary caches keep their existing resource policy.
    explicit SourceRuntimeResourcePreparation(std::shared_ptr<const Cache>);
    // Explicitly admitted original banks are staged with their canonical
    // metadata, timing plan and immutable resource identity in one registry
    // transaction. No native World, memory epoch or source voice is created.
    // Both owners are consumed once. While this preparation remains alive,
    // a repeated consumer rejects instead of falling back to ordinary assets.
    SourceRuntimeResourcePreparation(std::shared_ptr<const Cache>,const MccOriginalSoundRuntimeInputTu15&);
    SourceRuntimeResourcePreparation(std::shared_ptr<const Cache>,const PreparedOriginalSoundCatalogBindingTu15&);
    ~SourceRuntimeResourcePreparation();
    SourceRuntimeResourcePreparation(const SourceRuntimeResourcePreparation&)=delete;
    SourceRuntimeResourcePreparation& operator=(const SourceRuntimeResourcePreparation&)=delete;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
