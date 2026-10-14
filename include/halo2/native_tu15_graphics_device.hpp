#pragma once
#include "halo2/source_renderer_request_tu15.hpp"
#include "halo2/renderer_device_lease.hpp"
#include <thread>
namespace halo2 {
void mount_native_tu15_graphics_device_data(SourceRuntimeMemory&);
// Native graphics interface for the reached cold CreateDevice request. The
// caller supplies a genuine live port renderer; this module cannot forge one.
// The original handle is opaque and never truncates the native device address.
class NativeTu15GraphicsDevice final {
public:
    NativeTu15GraphicsDevice(SourceRuntimeMemory&,RendererDeviceLease);
    bool try_call(SourcePhysicsContactMemory&,const SourcePhysicsContactRuntimeCall&,SourcePhysicsWorldRuntimeAbi&);
    bool initialized() const;
    NativeRuntimeAddress native_address() const;
    std::uint32_t handle() const noexcept{return handle_;}
private:
    SourceRuntimeMemory& memory_;
    RendererDeviceLease renderer_;
    std::thread::id thread_;
    std::uint32_t handle_{};
};
}
