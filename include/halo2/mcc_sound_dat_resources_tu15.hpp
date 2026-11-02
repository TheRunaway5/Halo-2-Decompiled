#pragma once
#include "halo2/source_sound_cache_read_tu15.hpp"
#include <array>
#include <cstddef>
#include <memory>
#include <vector>

namespace halo2 {
class Cache;
struct MccSoundResourceRead;
struct SourceRuntimeMccPreparation;
struct MccSoundRuntimePlan;
// Immutable MCC prefix I/O only. Native allocation, cache handles, completion
// receipts, memory epochs and source voices remain with the original owners.
class MccSoundDatResourcesTu15 final {
    struct Impl;
    std::shared_ptr<const Impl> impl_;
    explicit MccSoundDatResourcesTu15(std::shared_ptr<const Impl>);
public:
    using Digest=std::array<std::byte,32>;
    static std::shared_ptr<const MccSoundDatResourcesTu15> prepare(const Cache&,const SourceRuntimeMccPreparation&);
    void require_routes(const MccSoundRuntimePlan&)const;
    std::vector<std::byte> read(const SourceSoundCacheReadTu15&)const;
    // Shares the exact immutable file snapshots with native sector reads.
    // The parser supplies a full salted sound/gestalt and source descriptor.
    std::vector<std::byte> read_encoded(const Cache&,const MccSoundResourceRead&)const;
    Digest bind_resource_identity(const Digest& resident)const;
    std::size_t routes()const;
};
}
