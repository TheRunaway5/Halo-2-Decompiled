#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include "halo2/source_draw_unit.hpp"

namespace halo2 {
struct NativeTu15RenderLightingServices {
    // Literal130980 flags3; the request result is consumed through AL.
    std::function<bool(std::uint32_t header,NativeTu15PhysicsFrameFp&)> request;
    // Optional genuineBCDF0 boundary; absent, use the native leaf below.
    // EAX=object, with the all-kinds maskFFFFFFFF.
    std::function<std::uint32_t(std::uint32_t object,NativeTu15PhysicsFrameFp&)> object;
};
std::uint32_t native_tu15_object_body_bcdf0(const SourcePhysicsContactMemory&,
    std::uint32_t object,std::uint32_t kind_mask);
// Whole2CFE0. Object/model/instance are EAX/EDX/ECX at the original entry.
// The initial lightmap/BSP roots are read in source order. A successful BSP
// bucket request reloads the global BSP root before reading instance+56.
// Missing lightmap gates alone permit BCDF0 fallback; checksum/request
// refusals after the model branch return3 directly.
std::uint32_t native_tu15_render_lighting_2cfe0(const SourcePhysicsContactMemory&,
    std::uint32_t object,std::uint32_t model,std::uint32_t instance,
    NativeTu15PhysicsFrameFp&,const NativeTu15RenderLightingServices&);
class PhysicsLiveWorldTu15;
// Live44930/2CFE0/449D0 at the native append's original service sites.
// The world and explicit arithmetic state must outlive the returned services.
SourceDrawUnitServices source_draw_resource_services_tu15(PhysicsLiveWorldTu15&,NativeTu15PhysicsFrameFp&);
}
