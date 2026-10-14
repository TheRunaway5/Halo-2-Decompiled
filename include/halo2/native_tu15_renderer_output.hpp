#pragma once
#include "halo2/native_tu15_renderer_depth.hpp"
#include "halo2/renderer_output_state.hpp"
namespace halo2 {
void mount_native_tu15_renderer_output_data(SourceRuntimeMemory&);
// Owns the complete reached cold output-state block as one graphics API seam.
class NativeTu15RendererOutput final {
public:
    NativeTu15RendererOutput(SourceRuntimeMemory&,NativeTu15GraphicsDevice&,NativeTu15RendererDepth&,RendererOutputState);
    bool try_call(SourcePhysicsContactMemory&,const SourcePhysicsContactRuntimeCall&,SourcePhysicsWorldRuntimeAbi&);
    const RendererOutputConfiguration& configuration() const;
    unsigned calls() const noexcept{return calls_;}
private:
    SourceRuntimeMemory& memory_;
    NativeTu15GraphicsDevice& device_;
    NativeTu15RendererDepth& depth_;
    RendererOutputState control_;
    std::thread::id thread_;
    std::optional<RendererOutputConfiguration> configuration_;
    unsigned calls_{};
};
}
