#pragma once
#include "halo2/source_object_admission.hpp"
#include "halo2/source_object_render_entry.hpp"

namespace halo2 {
class PhysicsLiveWorldTu15;
struct SourceLiveModelDrawContextTu15 {
    SourceObjectRenderEntryContext entry;
    SourceObjectDrawContext draw;
    Vec3 forward;
    unsigned view{},local_players=1;
    bool view_modifier{};
    std::uint32_t children=1;
    // Observe the completed native builder before its presentation/children
    // consume it. The collector's caller storage expires after collection.
    std::function<void(const SourceObjectDrawCollector&)> built;
};
// TU1.5 136230 orchestration with the native original entry/presentation/draw
// owners and live4B5C0 builder. Shares the caller's BSP prefix, arena, coverage,
// contexts and overrides. It neither resets the frame nor renders a GPU pass.
void source_live_model_draws_tu15(PhysicsLiveWorldTu15&,SourceObjectAdmission&,
    SourceDrawUnitState&,const SourceLiveModelDrawContextTu15&);
}
