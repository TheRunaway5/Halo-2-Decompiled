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
    // original shared-resource snapshots. For MCC, resident also authenticates
    // owned English facial-resource sectors. Filesystem paths stay local. The
    // Cache must outlive this owner and remain immutable after construction.
    explicit SourceRuntimeResources(const Cache &);
    // Canonical runtime descriptor addresses can differ from the Cache's
    // donor-normalized metadata addresses. Translate only that typed field
    // domain; encoded file/resource offsets retain their serialized meaning.
    using DescriptorAddress=std::function<std::uint32_t(std::uint32_t,std::size_t)>;
    SourceRuntimeResources(const Cache &,DescriptorAddress);
    ~SourceRuntimeResources();
    const Identity &identity() const noexcept;
    std::vector<std::byte> load(std::uint32_t encoded_offset,std::uint32_t bytes) const;
    SourceRuntimeCacheAnimationImage animation(std::uint32_t descriptor) const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
