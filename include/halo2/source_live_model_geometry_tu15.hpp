#pragma once
#include "halo2/source_bsp_part_commands.hpp"

namespace halo2 {
class PhysicsLiveWorldTu15;
// Read the selected unit's actual resident resource and current1D690 palette.
// The palette belongs to this live cache token, never to a shared model tag.
SourceModelCommandGeometry source_live_model_command_geometry_tu15(
    PhysicsLiveWorldTu15&,const SourceCullDrawUnit&);
// Current45550 banks, including BAC30 WORLD nodes and authored inverse binds.
// The borrowed ranges must remain alive through the cull operation.
SourceCullDrawBanks source_live_model_cull_geometry_tu15(
    PhysicsLiveWorldTu15&,const SourceCullDrawUnit&,const SourceBspRangeGeometry&);
// TU1.5 4D820/15280 c50 upload selection, including the rigid single-matrix
// branch and the selected region's actual remapped matrix range.
std::vector<std::array<float,12>> source_live_model_palette_rows_tu15(
    PhysicsLiveWorldTu15&,const SourceCullDrawUnit&,const SourceModelCommandGeometry&);
}
