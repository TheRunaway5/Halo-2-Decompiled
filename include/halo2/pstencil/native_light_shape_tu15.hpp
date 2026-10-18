#pragma once
#include "halo2/physics_world_runtime.hpp"
#include "halo2/source_shadow_view.hpp"
namespace halo2 {
bool pstencil_native_light_shape_tu15_owns_entry(std::uint32_t);
void pstencil_run_native_light_shape_tu15(SourcePhysicsContactMemory&,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
class PhysicsLiveWorldTu15;
namespace pstencil {
// Literal C3820 caller output. Unwritten caller bytes retain their actual
// scratch contents and are marked; they are not initialized shape parameters.
struct NativeLightShape {
    std::array<std::uint32_t,31> words;
    std::array<std::uint8_t,124> written{};
    std::uint16_t type()const;
};
NativeLightShape native_light_shape_tu15(PhysicsLiveWorldTu15&,std::uint32_t light);
// Whole native C3DC0 over the original completed shape and actual light
// frame. Returns its fully written camera bank, without inferred diagnostics.
std::vector<SourceFrustumCamera> native_light_cameras_tu15(PhysicsLiveWorldTu15&,
    std::uint32_t light);
}
}
