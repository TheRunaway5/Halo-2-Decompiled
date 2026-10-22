#pragma once
#include <memory>
namespace halo2 {
class Cache;
// One match-scoped preparation of the exact authenticated resident/shared
// resource snapshots. The first SourceRuntimeResources for this same Cache
// consumes them; descriptor translation is still bound by the real executor.
// This owns no source memory, player, simulation, random or clock state.
class SourceRuntimeResourcePreparation {
public:
    explicit SourceRuntimeResourcePreparation(std::shared_ptr<const Cache>);
    ~SourceRuntimeResourcePreparation();
    SourceRuntimeResourcePreparation(const SourceRuntimeResourcePreparation&)=delete;
    SourceRuntimeResourcePreparation& operator=(const SourceRuntimeResourcePreparation&)=delete;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
