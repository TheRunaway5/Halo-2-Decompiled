#include "halo2/source_runtime_cache_tags.hpp"
#include "halo2/cache.hpp"
#include <algorithm>

namespace halo2 {
SourceRuntimeCacheAnimationImage Cache::load_source_runtime_animation_image(std::uint32_t descriptor) const {
  if(vista_)throw FormatError("Source animation resource delivery requires the Xbox cache");
  const Bytes data(cache_bytes());
  for(const auto &tag:select_tags({.group=tag_groups::model_animation_graph,.require_address=true})) {
    const auto root=metadata_offset(tag.address,tag.size);
    if(tag.size<0xb4)throw FormatError("Source animation graph descriptor block is truncated");
    const auto count=data.u32(root+0xac),first=data.u32(root+0xb0);
    if(descriptor<first || std::uint64_t(descriptor)-first>=std::uint64_t(count)*20 ||
        (descriptor-first)%20)continue;
    const auto at=metadata_offset(descriptor,20);
    SourceRuntimeCacheAnimationImage out;
    const auto literal=data.slice(at,20);std::copy(literal.begin(),literal.end(),out.descriptor.begin());
        if(mcc_) {
          const auto size=data.u32(at+4),file=data.u32(at+8);
          const auto root_end=std::uint64_t(root)+tag.size;
          if(at<root || std::uint64_t(at)+20>root_end ||
              (size && (file<root || std::uint64_t(file)+size>root_end)))
            throw FormatError("Canonical animation resource escapes its graph owner");
          const auto bytes=size?data.slice(file,size):std::span<const std::byte>{};
          out.bytes.assign(bytes.begin(),bytes.end());
        } else out.bytes=raw_data(data.u32(at+8),data.u32(at+4));
    return out;
  }
  throw FormatError("Animation I/O request is not an actual jmad resource descriptor");
}
std::vector<std::byte> Cache::load_source_runtime_resource(std::uint32_t encoded_offset,std::uint32_t bytes)const {
  if(vista_)throw FormatError("Native source resource delivery requires the Xbox cache");
  if(mcc_) {
    // Canonical jmad resources are generated private data, beyond the donor's
    // original file extent. Admit only an exact declared group and its owned
    // sector padding; arbitrary appended metadata is never a file resource.
    const Bytes data(cache_bytes());
    for(const auto& tag:select_tags({.group=tag_groups::model_animation_graph,.require_address=true})) {
      const auto root=metadata_offset(tag.address,tag.size);
      const auto count=data.u32(root+0xac),first=data.u32(root+0xb0);
      if(!count)continue;
      const auto groups=metadata_offset(first,std::size_t(count)*20);
      if(groups<root || std::uint64_t(groups)+std::uint64_t(count)*20>std::uint64_t(root)+tag.size)
        throw FormatError("Canonical animation groups escape their graph owner");
      for(std::size_t n=0;n<count;++n) {
        const auto at=groups+n*20;
        const auto size=data.u32(at+4),file=data.u32(at+8);
        if(!size || file!=encoded_offset)continue;
        const auto padded=(std::uint64_t(size)+511)&~std::uint64_t(511);
        if((bytes!=size && bytes!=padded) || file%512 || file<root ||
            std::uint64_t(file)+padded>std::uint64_t(root)+tag.size)
          throw FormatError("Canonical animation I/O differs from its owned resource extent");
        const auto padding=data.slice(std::size_t(file)+size,std::size_t(padded-size));
        if(!std::ranges::all_of(padding,[](std::byte b){return b==std::byte{};}))
          throw FormatError("Canonical animation sector padding contains metadata");
        const auto raw=data.slice(file,bytes);return {raw.begin(),raw.end()};
      }
    }
  }
  return raw_data(encoded_offset,bytes);
}
} // namespace halo2
