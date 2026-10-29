#pragma once
#include "halo2/cache.hpp"

namespace halo2::detail {
inline bool shadow_bitmap_page_matches(const Texture& source,const Texture& shape,
    std::uint64_t fingerprint,std::uint32_t mip_count,bool source_image,bool mutated) {
    if(source.pixels.empty() || !source.width || !source.height || !source.layers ||
       !source_image || mutated || !shape.pixels.empty() ||
       source.width!=shape.width || source.height!=shape.height || source.format!=shape.format ||
       source.layers!=shape.layers || source.depth!=shape.depth || source.mips.size()!=shape.mips.size() ||
       (!source.mips.empty() && source.mips.size()%source.layers) ||
       mip_count!=(source.mips.empty()?1:source.mips.size()/source.layers) ||
       fingerprint!=source.pixels.fingerprint())return false;
    for(std::size_t n=0;n<source.mips.size();++n) {
        const auto& a=source.mips[n];const auto& b=shape.mips[n];
        if(a.width!=b.width || a.height!=b.height || a.depth!=b.depth ||
           a.offset!=b.offset || a.size!=b.size)return false;
    }
    return true;
}
}
