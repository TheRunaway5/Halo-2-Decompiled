#pragma once
#include "halo2/native_tu15_graphics_device.hpp"
#include "halo2/renderer_depth_mode.hpp"
#include <optional>
namespace halo2 {
void mount_native_tu15_renderer_depth_state_data(SourceRuntimeMemory&);
class NativeTu15RendererDepth final {
public:
    NativeTu15RendererDepth(SourceRuntimeMemory&,NativeTu15GraphicsDevice&,RendererDepthMode);
    bool try_call(SourcePhysicsContactMemory&,const SourcePhysicsContactRuntimeCall&,SourcePhysicsWorldRuntimeAbi&);
    const RendererDepthConfiguration& configuration() const;
    unsigned state_calls() const noexcept{return state_calls_;}
private:
    SourceRuntimeMemory& memory_;
    NativeTu15GraphicsDevice& device_;
    RendererDepthMode control_;
    std::thread::id thread_;
    std::optional<RendererDepthConfiguration> configuration_;
    unsigned state_calls_{};
};
}
