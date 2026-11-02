#pragma once
#include "halo2/source_draw_unit_cull.hpp"

namespace halo2 {
struct SourcePortalVisibilitySet;
struct NativeTu15FrustumDrawCall {
    std::uint16_t unit{},entry{};
    std::uint32_t argument{}; // Full index word pushed before44E80 signs low16.
};
struct NativeTu15FrustumBound {std::uint32_t subpart{};Vec3 center{};float radius{};std::uint8_t node{};};
using NativeTu15FrustumBoundObserver=std::function<void(const NativeTu15FrustumBound&)>;
//44E80 for one already-selected constructed cluster entry. The linear
// model branch eagerly composes every current node, consumes literal bound
// indices (FFFF is not a sentinel), transforms centres and retains radii.
// Positive accelerators use the separately qualified range/plane owner.
// Unavailable consumed banks reject before publishing unit/mask changes.
void native_tu15_frustum_draw_cull_entry_44e80(SourceCullDrawUnit&,SourceCullDrawBanks&,
    SourceBspRangeArena&,std::uint32_t mask_arena_address,std::uint32_t pass_mode,
    const SourcePortalVisibilitySet&,unsigned entry,const NativeTu15FrustumBoundObserver& observe={});
//44710 mode0/44680: index order, low16 endpoints, signed association and
// ascending128-bit entry walk. Once selected, every entry runs even after an
// earlier entry sets flag1. Resolve geometry only when an entry is consumed.
std::vector<NativeTu15FrustumDrawCall> native_tu15_frustum_draw_cull_44710(
    std::span<SourceCullDrawUnit>,SourceBspRangeArena&,std::uint32_t mask_arena_address,
    const SourceDrawUnitCullContext&,const SourcePortalVisibilitySet&,
    std::span<const std::array<std::uint32_t,4>> associations,
    const std::function<SourceCullDrawBanks(unsigned,const SourceCullDrawUnit&)>& resolve);
}
