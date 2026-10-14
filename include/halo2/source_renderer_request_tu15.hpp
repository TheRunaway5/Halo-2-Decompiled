#pragma once
#include "halo2/physics_world_runtime.hpp"
#include "halo2/source_runtime_memory.hpp"
namespace halo2 {
void mount_native_tu15_renderer_request_data(SourceRuntimeMemory&);
bool source_renderer_request_tu15_owns_entry(std::uint32_t);
void source_run_renderer_request_tu15(SourcePhysicsContactMemory&,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
// Exact source-produced device request. Opaque presentation words are retained
// until each field's native renderer mapping is admitted. Decoding this frame
// neither creates a GPU device nor publishes a device/completion result.
struct NativeTu15RendererDeviceFrame {
    std::uint32_t output_token{},parameters_token{},flags{};
    NativeRuntimeAddress output_backing{},parameters_backing{};
    std::array<std::uint32_t,17> presentation{};
};
NativeTu15RendererDeviceFrame native_tu15_renderer_device_frame(
    const SourceRuntimeMemory&,const SourcePhysicsContactRuntimeCall&,const SourcePhysicsWorldRuntimeAbi&);
}
