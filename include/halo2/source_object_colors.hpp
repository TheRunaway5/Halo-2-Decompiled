#pragma once
#include "halo2/appearance.hpp"

namespace halo2 {
struct SourceObjectColorPermutation {
    float weight{};
    Vec3 lower,upper;
    std::uint32_t variant_name{};
};
struct SourceObjectColorChannel {
    std::vector<SourceObjectColorPermutation> permutations;
    std::uint32_t function_count{};
};
struct SourceObjectColorDefinition {std::vector<SourceObjectColorChannel> channels;};
// Profile name-group2B00F0: original1318D0/131A00 with authored text saturation
// and value constants45E19C/45E198; input is the raw primary profile color.
Vec3 source_profile_name_color(Vec3);
// Original BE240. Its hashes use the constructor's object+64 position and
// channel ordinal, not the global primary random stream or posed query center.
// actual_variant_name is the selected hlmt variant's name (object+B1), not an
// unresolved requested variant. Overrides reproduce B7B40's supplied mask/bank.
// BE8E0's later color functions are separate; their count is retained so a
// caller cannot mistake this initial bank for a current function-modulated bank.
ObjectColors source_object_initial_colors(const SourceObjectColorDefinition&,
    Vec3 constructor_position,std::uint32_t actual_variant_name,
    std::uint32_t override_mask=0,
    const ObjectColors& overrides={{{1,1,1},{1,1,1},{1,1,1},{1,1,1}}});
}
