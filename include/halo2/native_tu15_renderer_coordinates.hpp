#pragma once
#include "halo2/native_tu15_renderer_output.hpp"
#include "halo2/renderer_texture_coordinates.hpp"
namespace halo2 {
class NativeTu15RendererCoordinates final {
public:
    NativeTu15RendererCoordinates(SourceRuntimeMemory&,NativeTu15GraphicsDevice&,NativeTu15RendererOutput&,RendererTextureCoordinates);
    bool try_call(SourcePhysicsContactMemory&,const SourcePhysicsContactRuntimeCall&,SourcePhysicsWorldRuntimeAbi&);
    const RendererTextureCoordinateConfiguration& configuration() const;
    unsigned calls() const noexcept{return calls_;}
private:
    SourceRuntimeMemory& memory_;
    NativeTu15GraphicsDevice& device_;
    NativeTu15RendererOutput& output_;
    RendererTextureCoordinates control_;
    std::thread::id thread_;
    std::optional<RendererTextureCoordinateConfiguration> configuration_;
    unsigned calls_{};
};
}
