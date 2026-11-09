#pragma once
#include "halo2/native_tu15_cull_walk_44710.hpp"
#include "halo2/source_bsp_ranges.hpp"
#include "halo2/source_draw_unit_cull.hpp"
#include <map>
#include <string_view>

namespace halo2 {
class Cache;
struct SourcePortalVisibilitySet;

// Ordinary context 556E18 mode 0. Model range metadata belongs to the
// retained cache; each entry still binds the current World geometry and pose.
// The caller may supply an independently admitted BSP entry owner.
class SourceMainFrustumCullTu15 {
public:
    using RangeLoader=std::function<SourceBspRangeGeometry(std::uint32_t,unsigned)>;
    using BspEntry=std::function<bool(SourceCullDrawUnit&,SourceBspRangeArena&,
        std::uint32_t arena_address,std::uint32_t entry,NativeTu15PhysicsFrameFp&)>;
    SourceMainFrustumCullTu15(const Cache&,std::string_view scope);
    SourceMainFrustumCullTu15(RangeLoader,std::string_view scope);
    ~SourceMainFrustumCullTu15();
    NativeTu15PhysicsFrameFp run(PhysicsLiveWorldTu15&,const SourcePortalVisibilitySet*,const BspEntry& = {});
    std::uint64_t native_model_entries() const noexcept {return stats_.native_model_entries;}
private:
    RangeLoader ranges_;
    std::string_view scope_;
    std::map<std::pair<std::uint32_t,unsigned>,SourceBspRangeGeometry> model_ranges_;
    std::map<std::uint32_t,std::uint64_t> native_models_;
    struct Stats {
        std::uint64_t views{},native_entries{},residual_entries{},fallbacks{},
            unit_mismatches{},mask_mismatches{},fp_mismatches{},native_model_entries{},residual_model_entries{};
    } stats_;
};
}
