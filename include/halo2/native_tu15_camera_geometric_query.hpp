#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
namespace halo2 {
struct NativeTu15CameraGeometricQuery {
    std::uint32_t mask{},origin{},displacement{},ignore_first{},ignore_second{},result{};
};
struct NativeTu15CameraInstancedQuery {
    std::uint32_t cluster{},mask{},bsp_filter{},origin{},displacement{},result{};
};
struct NativeTu15CameraDynamicQuery {
    std::uint32_t datum{},recursive{},mask{},bsp_filter{},origin{},displacement{},ignore_first{},ignore_second{},result{};
};
struct NativeTu15CameraGeometricQueryScratch {
    std::uint32_t bsp_output{},bsp_context{},iterator{},payload{};
};
struct NativeTu15CameraGeometricQueryServices {
    std::function<bool(const NativeTu15CameraInstancedQuery&,NativeTu15PhysicsFrameFp&)> instanced;
    std::function<bool(const NativeTu15CameraDynamicQuery&,NativeTu15PhysicsFrameFp&)> dynamic;
    // Optional diagnostic child boundaries; never executes source instructions.
    std::function<void(std::uint32_t,bool)> native_child;
};
struct NativeTu15CameraGeometricQueryStats {
    std::uint64_t calls{},completed{},bsp_calls{},bsp_hits{},cluster_visits{},instanced_calls{},dynamic_calls{},bridge_calls{},clearance_calls{},unbound_services{};
};
// Native16C620 typed parent. Instanced/dynamic services are explicit native
// requirements; no virtual CPU fallback is available. Qualification domains
// and remaining child requirements are recorded separately.
bool native_tu15_query_camera_geometry(SourcePhysicsContactMemory&,
    const NativeTu15CameraGeometricQuery&,NativeTu15CameraGeometricQueryScratch,
    NativeTu15PhysicsFrameFp&,unsigned preserved_depth,
    const NativeTu15CameraGeometricQueryServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15CameraGeometricQueryStats* = nullptr);
} // namespace halo2
