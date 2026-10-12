#pragma once
#include "halo2/source_portal_mask.hpp"

namespace halo2 {
// Bounded mainmenu mask initialization. The literal title BSP has no owner
// associations, no associated portal indices and no flag8 portals. Reject a
// changed bank rather than invent machine lifetimes or later mask updates.
// The caller owns the persistent state; the separate acoustic bank is retained.
// This supplies only the mask, not visibility, a containing cluster or a draw.
void initialize_mainmenu_portal_state(SourcePortalMaskState &,
    const SourcePortalMaskDefinition &);
// Requires the actual source mainmenu scenario type2, then extracts the BSP
// definition through Cache. Neither a file name nor a screen ID admits it.
void initialize_mainmenu_portal_state(SourcePortalMaskState &,const Cache &,
    std::size_t bsp);
}
