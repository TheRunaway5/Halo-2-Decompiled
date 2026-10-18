#pragma once
#include "halo2/physics_world_runtime.hpp"
#include "halo2/effect_light.hpp"
namespace halo2 {
bool pstencil_native_light_constants_tu15_owns_entry(std::uint32_t);
void pstencil_run_native_light_constants_tu15(SourcePhysicsContactMemory&,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
class PhysicsLiveWorldTu15;
namespace pstencil {
// Complete49790 consumes actual native shape/frame and the original4A1D0
// completed fade inputs. Caller constants retain the original unwritten words.
EffectLightUniforms native_light_constants_tu15(PhysicsLiveWorldTu15&,
    std::uint32_t light,const EffectLightViewFades&,float selected_fade,
    const std::array<ShaderVector,9>& previous);
}
}
