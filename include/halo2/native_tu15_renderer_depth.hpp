#pragma once
#include "halo2/native_tu15_graphics_device.hpp"
#include "halo2/renderer_depth_mode.hpp"
#include <optional>
namespace halo2 {
class NativeTu15RendererDepth final {
public:
    NativeTu15RendererDepth(SourceRuntimeMemory&,NativeTu15GraphicsDevice&,RendererDepthMode);
    bool try_call(SourcePhysicsContactMemory&,const SourcePhysicsContactRuntimeCall&,SourcePhysicsWorldRuntimeAbi&);
    const RendererDepthConfiguration& configuration() const;
private:
    SourceRuntimeMemory& memory_;
    NativeTu15GraphicsDevice& device_;
    RendererDepthMode control_;
    std::thread::id thread_;
    std::optional<RendererDepthConfiguration> configuration_;
};
}
