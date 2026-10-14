#pragma once
#include "halo2/model_selection.hpp"
#include <functional>

namespace halo2 {
// The live 16D660 region choices, including the actual allocated effect handles.
// Initial 16D280 selection deliberately leaves every looping handle at NONE.
struct SourceModelRegionBank {
    SourceInitialModelSelection selection;
    std::vector<std::uint32_t> looping_effects;
};
struct SourceModelRegionRequest {
    int region=-1,damage=-1;
    bool force{};
    int property_bit=-1;
    std::uint16_t property_regions{};
};
struct SourceModelRegionOwners {
    // Required only when an authored branch actually consumes that owner.
    std::uint32_t *random{};
    std::function<void(std::uint32_t)> stop_effect;
    std::function<std::uint32_t(std::uint32_t,std::uint32_t)> start_effect;
};
SourceModelRegionBank source_model_region_bank(SourceInitialModelSelection);
// Xbox16D660/16DAD0/16DCE0. Matching state choices retain their authored order;
// singleton choices consume no RNG. Effect allocation remains an explicit owner.
bool source_model_regions_change(const SourceInitialModelDefinition &,
    SourceModelRegionBank &,const SourceModelRegionRequest &,const SourceModelRegionOwners & = {});
// Unique preparation bank for this variant's reachable live region choices.
// This is a resource union; source_initial_model_sections retains draw order.
std::vector<unsigned> source_model_region_resource_sections(const SourceInitialModelDefinition &,
    const SourceInitialModelSelection &,unsigned lod);
}
