#pragma once
#include "halo2/effect_light_lifetime.hpp"
namespace halo2 {
class SourceRuntimeMemory;
namespace pstencil {
// Completed simulation state, including original spatial lists and full salts.
// A view may update its private fades/visit stamps; it must not allocate, tick,
// register or retire lights in this snapshot or publish it back to simulation.
struct NativeLightSnapshot {
    SourceEffectLightDatumPool lights;
    SourceEffectLightSpatialPool spatial;
    explicit NativeLightSnapshot(std::size_t clusters):spatial(clusters){}
};
NativeLightSnapshot native_lights_tu15(const SourceRuntimeMemory&,std::size_t clusters);
}
}
