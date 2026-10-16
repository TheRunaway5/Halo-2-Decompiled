#pragma once
#include "halo2/native_tu15_framebuffer_startup.hpp"
#include "halo2/renderer_texture_pairs.hpp"

namespace halo2 {
void mount_native_tu15_texture_pair_data(SourceRuntimeMemory&);
struct NativeTu15TexturePairBinding {
    std::uint32_t source_cell{},source_texture_format{},width{},height{};
    std::uint64_t image{},view{};
    NativeRuntimeAddress image_backing{},view_backing{};
    bool unbound_linear_surface{};
};
// Native successful14830. Four identities share two actual GPU images;
// the fifth is original491770's unbound512x512 linear surface metadata.
// No image is fabricated for that unbound surface; its later producer must
// attach a real resource before it can be drawn. No SDK object bytes imported.
class NativeTu15TexturePairStartup final {
public:
    NativeTu15TexturePairStartup(SourceRuntimeMemory&,NativeTu15GraphicsDevice&,
        NativeTu15FramebufferStartup&,RendererTexturePairFactory);
    NativeTu15TexturePairStartup(const NativeTu15TexturePairStartup&)=delete;
    NativeTu15TexturePairStartup& operator=(const NativeTu15TexturePairStartup&)=delete;
    bool try_call(SourcePhysicsContactMemory&,const SourcePhysicsContactRuntimeCall&,SourcePhysicsWorldRuntimeAbi&);
    bool initialized() const;
    std::uint32_t handle(std::uint32_t source_cell) const;
    const NativeTu15TexturePairBinding& resolve(std::uint32_t handle) const;
    NativeRuntimeAddress native_address(std::uint32_t handle) const;
    void release(); // Quiescent: discard unsubmitted borrowers first.
private:
    SourceRuntimeMemory& memory_;
    NativeTu15GraphicsDevice& device_;
    NativeTu15FramebufferStartup& framebuffers_;
    RendererTexturePairFactory factory_;
    std::thread::id thread_=std::this_thread::get_id();
    std::optional<RendererTexturePairs> resources_;
    std::array<NativeTu15TexturePairBinding,5> records_{};
};
}
