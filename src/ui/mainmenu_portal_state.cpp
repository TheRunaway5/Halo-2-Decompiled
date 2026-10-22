#include "halo2/mainmenu_portal_state.hpp"
#include <algorithm>

namespace halo2 {
void initialize_mainmenu_portal_state(SourcePortalMaskState &state,
    const SourcePortalMaskDefinition &definition) {
    if(!definition.associations.empty() || !definition.portal_indices.empty() ||
       std::any_of(definition.portal_flags.begin(),definition.portal_flags.end(),
           [](std::uint32_t flags){return (flags&8u)!=0;}))
        throw FormatError("mainmenu portal state requires the source owner-free unblocked BSP bank");
    // Original210DF0 fills the16 words, runs210F00/210E80, then210E20.
    // No owner inputs are needed to obtain this empty-association mask. This
    // does not initialize or claim to reset a separate live machine pool.
    state.initialize(definition,{});
}
void initialize_mainmenu_portal_state(SourcePortalMaskState &state,
    const Cache &cache,std::size_t bsp) {
    if(cache.load_source_geometry_scenario_type()!=2)
        throw FormatError("mainmenu portal state requires source scenario type2");
    initialize_mainmenu_portal_state(state,cache.load_source_portal_mask_definition(bsp));
}
}
