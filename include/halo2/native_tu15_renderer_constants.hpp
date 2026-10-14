#pragma once
#include "halo2/native_tu15_graphics_device.hpp"
#include "halo2/renderer_constant_mode.hpp"
#include <optional>
namespace halo2 {
class NativeTu15RendererConstants final {
public:
    NativeTu15RendererConstants(SourceRuntimeMemory&,NativeTu15GraphicsDevice&,RendererConstantMode);
    bool try_call(SourcePhysicsContactMemory&,const SourcePhysicsContactRuntimeCall&,SourcePhysicsWorldRuntimeAbi&);
    const RendererConstantConfiguration& configuration() const;
private:
    SourceRuntimeMemory& memory_;
    NativeTu15GraphicsDevice& device_;
    RendererConstantMode control_;
    std::thread::id thread_;
    std::optional<RendererConstantConfiguration> configuration_;
};
}
