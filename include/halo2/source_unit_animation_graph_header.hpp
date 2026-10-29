#pragma once
#include "halo2/cache.hpp"
#include <algorithm>
#include <array>

namespace halo2 {
// Owned literal header for synchronous native consumers that only inspect
// graph counts and original VA words. Validate the same complete tag extent
// as Cache's owned metadata loader without copying its resource records.
inline std::array<std::byte,0xc4> load_source_unit_animation_graph_header(
    const Cache &cache,std::uint32_t graph) {
    if(cache.is_vista())
        throw FormatError("Vista animation graph requires verified Xbox unit-animation metadata conversion");
    const auto *tag=cache.find_tag({graph},tag_groups::model_animation_graph);
    if(!tag || !tag->address || tag->size<0xc4)
        throw FormatError("source unit animation graph metadata is unavailable");
    const auto source=cache.read_tag_bytes(tag->address,tag->size).first(0xc4);
    std::array<std::byte,0xc4> header;
    std::copy(source.begin(),source.end(),header.begin());
    return header;
}
}
