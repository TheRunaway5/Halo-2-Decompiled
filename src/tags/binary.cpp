#include "halo2/tags/binary.hpp"
#include "halo2/cache.hpp"
#include <limits>
#include <algorithm>
#include <bit>

namespace halo2 {
TagBlock read_tag_block_descriptor(const Bytes& bytes, std::size_t descriptor) {
    return {std::bit_cast<std::int32_t>(bytes.u32(descriptor)),bytes.u32(descriptor+4)};
}
std::size_t TagBlock::extent(std::size_t stride, std::size_t limit) const {
    if (count < 0 || !stride || std::size_t(count) > limit ||
        std::size_t(count) > std::numeric_limits<std::size_t>::max()/stride)
        throw FormatError("Tag block count exceeds its allocation limit");
    return std::size_t(count)*stride;
}
TagAddressSpace::TagAddressSpace(const Bytes& bytes, std::int64_t magic)
    : bytes_(bytes.slice(0,bytes.size())), magic_(magic) {}
std::size_t TagAddressSpace::offset(std::uint32_t address, std::size_t size) const {
    if (magic_ < -std::int64_t(0x100000000ull) || magic_ > std::int64_t(0x100000000ull))
        throw FormatError("Tag address translation exceeds source address space");
    const auto translated = std::int64_t(address) - magic_;
    if (translated < 0 || std::uint64_t(translated) > std::numeric_limits<std::size_t>::max())
        throw FormatError("Tag pointer precedes its allocation");
    const auto at = std::size_t(translated);
    Bytes(bytes_).require(at,size);
    return at;
}
TagBlockRange TagAddressSpace::block(std::size_t descriptor, std::size_t stride, std::size_t limit) const {
    const Bytes bytes(bytes_);
    const auto block = read_tag_block_descriptor(bytes,descriptor);
    const auto size = block.extent(stride,limit);
    if (!block.count) return {};
    return {std::size_t(block.count),offset(block.address,size)};
}
std::size_t Cache::metadata_offset(std::uint32_t address, std::size_t size) const {
    if(pc_geometry()) {
        auto region=std::upper_bound(vista_metadata_regions_.begin(),vista_metadata_regions_.end(),address,
            [](std::uint32_t p,const auto& r){return p<r.first;});
        if(region==vista_metadata_regions_.begin())throw FormatError("Vista pointer precedes resident metadata");
        --region;
        const auto offset=std::uint64_t(address)-region->first;
        if(offset>region->second || size>region->second-offset)
            throw FormatError("Vista pointer escapes resident tag metadata");
    }
    return TagAddressSpace(Bytes(cache_bytes()), metadata_magic_).offset(address, size);
}
TagBlockRange Cache::metadata_block(std::size_t descriptor, std::size_t stride, std::size_t limit) const {
    const Bytes bytes(cache_bytes());
    auto result=TagAddressSpace(bytes,metadata_magic_).block(descriptor,stride,limit);
    if (result.count) result.offset=metadata_offset(bytes.u32(descriptor+4),result.count*stride);
    return result;
}
std::vector<std::size_t> Cache::metadata_block_offsets(std::size_t descriptor, std::size_t stride, std::size_t limit) const {
    const auto range=metadata_block(descriptor,stride,limit);
    std::vector<std::size_t> result;
    result.reserve(range.count);
    for (std::size_t i=0;i<range.count;++i) result.push_back(range.offset+i*stride);
    return result;
}
} // namespace halo2
