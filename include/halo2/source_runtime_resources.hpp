#pragma once
#include "halo2/source_runtime_cache_tags.hpp"
#include <memory>
#include <optional>
namespace halo2 {
class Cache;
class SourceRuntimeResources {
public:
    using Digest=std::array<std::byte,32>;
    struct Identity {
        Digest resident;
        std::array<std::optional<Digest>,3> shared;
        bool operator==(const Identity &) const = default;
    };
    // Bind actual private resident bytes (including balance writes) and pinned
    // original shared-resource snapshots. Filesystem paths stay local. The
    // Cache must outlive this owner and remain immutable after construction.
    explicit SourceRuntimeResources(const Cache &);
    ~SourceRuntimeResources();
    const Identity &identity() const noexcept;
    std::vector<std::byte> load(std::uint32_t encoded_offset,std::uint32_t bytes) const;
    SourceRuntimeCacheAnimationImage animation(std::uint32_t descriptor) const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
