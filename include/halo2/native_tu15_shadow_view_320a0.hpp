#pragma once
#include "halo2/source_shadow_view.hpp"
#include "halo2/native_tu15_physics_frame.hpp"
namespace halo2 {
// Typed 320A0/1681C0 receiver-volume path: twelve transformed edges, near
// clipping, projection and rectangle admission. The genuine constant bank
// and empty-bound template remain explicit inputs; no GPU/visibility result
// or source-address execution belongs to this value kernel.
SourceShadowViewVolume native_tu15_shadow_view_volume_320a0(
    const SourcePhysicsContactMemory&,const SourceFrustumCamera&,
    const SourceShadowViewBox&,NativeTu15PhysicsFrameFp&);
}
