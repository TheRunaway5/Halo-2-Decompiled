#pragma once
#include "halo2/native_tu15_graphics_device.hpp"
#include "halo2/renderer_palette.hpp"
namespace halo2 {
void mount_native_tu15_renderer_palette_data(SourceRuntimeMemory&);
// Explicit native graphics API boundaries. Handles refer to real renderer
// color resources; Xbox SDK object layouts/driver banks are not imported.
class NativeTu15RendererPalette final {
public:
    NativeTu15RendererPalette(SourceRuntimeMemory&,NativeTu15GraphicsDevice&,RendererPaletteFactory);
    // The ordinary native world already has a live Renderer. Admit its actual
    // immutable color resource without claiming the Xbox cold device sequence.
    NativeTu15RendererPalette(SourceRuntimeMemory&,RendererPaletteFactory);
    bool try_call(SourcePhysicsContactMemory&,const SourcePhysicsContactRuntimeCall&,SourcePhysicsWorldRuntimeAbi&);
    bool initialized() const;
    NativeRuntimeAddress native_address() const;
    std::span<const std::byte,1024> colors() const;
    std::uint32_t handle() const noexcept{return handle_;}
    unsigned bindings() const noexcept{return bindings_;}
private:
    SourceRuntimeMemory& memory_;
    NativeTu15GraphicsDevice* device_;
    RendererPaletteFactory factory_;
    RendererPalette palette_;
    std::thread::id thread_;
    std::uint32_t handle_{};
    unsigned bindings_{};
};
}
