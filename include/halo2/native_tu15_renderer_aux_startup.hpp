#pragma once
#include "halo2/native_tu15_graphics_device.hpp"
#include "halo2/renderer_aux_textures.hpp"

namespace halo2 {
void mount_native_tu15_renderer_aux_data(SourceRuntimeMemory&);
struct NativeTu15RendererAuxBinding {
    std::uint32_t source_cell{},source_texture_format{};
    RendererAuxTextureImage texture;
};
// Successful native1D0C0. Actual2D,volume,cube owners have completed their
// original texel upload before the three opaque source identities publish.
// The source handles never truncate GPU handles or actual native addresses.
class NativeTu15RendererAuxStartup final {
public:
    NativeTu15RendererAuxStartup(SourceRuntimeMemory&,NativeTu15GraphicsDevice&,RendererAuxTextureFactory);
    NativeTu15RendererAuxStartup(const NativeTu15RendererAuxStartup&)=delete;
    NativeTu15RendererAuxStartup& operator=(const NativeTu15RendererAuxStartup&)=delete;
    bool try_call(SourcePhysicsContactMemory&,const SourcePhysicsContactRuntimeCall&,SourcePhysicsWorldRuntimeAbi&);
    bool initialized() const;
    std::uint32_t handle(std::uint32_t source_cell) const;
    const NativeTu15RendererAuxBinding& resolve(std::uint32_t handle) const;
    NativeRuntimeAddress native_address(std::uint32_t handle) const;
    std::vector<std::byte> pixels(std::uint32_t handle) const;
    void release(); // Quiescent: discard unsubmitted borrowers first.
private:
    SourceRuntimeMemory& memory_;
    NativeTu15GraphicsDevice& device_;
    RendererAuxTextureFactory factory_;
    std::thread::id thread_=std::this_thread::get_id();
    std::optional<RendererAuxTextures> resources_;
    std::array<NativeTu15RendererAuxBinding,3> records_{};
};
}
