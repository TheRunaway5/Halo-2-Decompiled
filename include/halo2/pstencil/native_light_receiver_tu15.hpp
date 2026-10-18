#pragma once
#include "halo2/effect_light_receiver.hpp"
#include "halo2/source_object_admission.hpp"
#include "halo2/source_live_effect_light_commands_tu15.hpp"

namespace halo2 {
class PhysicsLiveWorldTu15;
namespace pstencil {
struct NativeLightReceiver {
    SourceEffectLightReceiverView view;
    SourceObjectAdmission objects;
    std::uint32_t context{},visibility_bank{};
    bool shadow_enabled{}; // Actual31E15/31650 caller byte, before31177's layer10 gate.
};
// Exact31E27..31E9A caller of whole134C00. Consumes a live full light
// generation and wholeC3DC0 cameras. The ordinary camera/queues must already
// have their real native publication. This advances native query state;
// consume/copy its queues before another secondary query reuses them.
NativeLightReceiver native_light_receiver_tu15(PhysicsLiveWorldTu15&,
    std::uint32_t light);
// Whole31570/31500/C5170/C3820 after the actual main-view light update.
// Native shape, local-owner and cinematic feature gates are retained.
bool native_light_render_admission_tu15(PhysicsLiveWorldTu15&,std::uint32_t light);
struct NativeLightGeometry {
    SourceLiveEffectLightStateTu15 retained;
    SourceBspPartCommandState commands;
};
// Whole31EDA..31172 owners. Native BSP/model collection, PRT, culling and
// command generation consume the already completed secondary query directly.
NativeLightGeometry native_light_geometry_tu15(PhysicsLiveWorldTu15&,
    const NativeLightReceiver&);
}
}
