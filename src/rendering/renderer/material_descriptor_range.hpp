#pragma once
#include <map>
#include <limits>
#include <ranges>
#include <utility>

namespace halo2 {
// Retain ascending lightmap order and every descriptor belonging to this
// material. The caller still owns update timing and descriptor contents.
template<class Descriptor>
auto material_descriptor_range(
    const std::map<std::pair<unsigned,unsigned>,Descriptor>& descriptors,unsigned material) {
#ifdef HALO2_REFERENCE_FULL_DESCRIPTOR_SCAN
    return descriptors | std::views::filter([material](const auto& entry){return entry.first.first==material;});
#else
    return std::ranges::subrange(descriptors.lower_bound({material,0}),
        descriptors.upper_bound({material,std::numeric_limits<unsigned>::max()}));
#endif
}
}
