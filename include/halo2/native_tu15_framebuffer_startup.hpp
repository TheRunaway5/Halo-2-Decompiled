#pragma once
#include "halo2/native_tu15_renderer_coordinates.hpp"

namespace halo2 {
void mount_native_tu15_framebuffer_publication_data(SourceRuntimeMemory&);
// Native145E0 owner. Source cells carry opaque GPU identities for the current
// source ABI; every descriptor, GPU resource and native address stays wide.
// These identities MUST be resolved here, never dereferenced as SDK objects.
// Only successful owned creation is admitted. A platform allocation/format
// error remains an exception at the pending call, not a supplied game BOOL.
class NativeTu15FramebufferStartup final {
public:
    NativeTu15FramebufferStartup(SourceRuntimeMemory&,NativeTu15GraphicsDevice&,NativeTu15RendererCoordinates&);
    NativeTu15FramebufferStartup(const NativeTu15FramebufferStartup&)=delete;
    NativeTu15FramebufferStartup& operator=(const NativeTu15FramebufferStartup&)=delete;
    bool try_call(SourcePhysicsContactMemory&,const SourcePhysicsContactRuntimeCall&,SourcePhysicsWorldRuntimeAbi&);
    bool initialized() const;
    std::uint32_t handle(std::uint32_t source_cell) const;
    const NativeTu15FramebufferBinding& resolve(std::uint32_t handle) const;
    NativeRuntimeAddress native_address(std::uint32_t handle) const;
    // Quiescent cleanup: clear only this owner's complete publication. GPU
    // commands/borrowed descriptors must have retired before releasing it.
    void release();
private:
    SourceRuntimeMemory& memory_;
    NativeTu15GraphicsDevice& device_;
    NativeTu15RendererCoordinates& coordinates_;
    std::thread::id thread_;
    std::optional<NativeTu15FramebufferAliases> resources_;
};
}
