#pragma once
#include "halo2/native_tu15_graphics_device.hpp"
#include "halo2/renderer_debug_texture.hpp"
#include "halo2/source_renderer_debug_storage_tu15.hpp"
#include "halo2/physics_pool_host_storage.hpp"

namespace halo2 {
struct NativeTu15DebugTextureBinding {
    std::uint32_t source_bitmap{},source_handle{},source_texture_format{},source_size_word{},source_pitch{};
    NativeRuntimeAddress bitmap{};
    RendererTargetImage texture{0,0,NativeRuntimeAddress{},NativeRuntimeAddress{},0,0,0,0,0,0,0};
    VkComponentMapping components{};
};
// Owns the reached1CFE0->158D0 native texture boundary while original CPU
// code constructs the bitmap and all512 debugger rows. The opaque identity
// never truncates a native pointer or imports Xbox SDK object bytes.
class NativeTu15DebugTextureStartup final {
public:
    NativeTu15DebugTextureStartup(SourceRuntimeMemory&,NativeTu15GraphicsDevice&,RendererDebugTextureFactory,SourcePhysicsPoolHostStorage&);
    NativeTu15DebugTextureStartup(const NativeTu15DebugTextureStartup&)=delete;
    NativeTu15DebugTextureStartup& operator=(const NativeTu15DebugTextureStartup&)=delete;
    bool try_call(SourcePhysicsContactMemory&,const SourcePhysicsContactRuntimeCall&,SourcePhysicsWorldRuntimeAbi&);
    bool initialized() const;
    std::uint32_t handle() const;
    const NativeTu15DebugTextureBinding& resolve(std::uint32_t) const;
    NativeRuntimeAddress native_address(std::uint32_t) const;
    // Constructor admission verifies all512{-1,0} rows after original1EFD0
    // returns. This initial-state view is not a general later-frame validator.
    NativeRuntimeAddress initial_rows() const;
    bool content_initialized() const;
    void release(); // Quiescent: discard unsubmitted borrowers first.
private:
    SourceRuntimeMemory& memory_;
    NativeTu15GraphicsDevice& device_;
    RendererDebugTextureFactory factory_;
    SourcePhysicsPoolHostStorage& raw_;
    std::thread::id thread_=std::this_thread::get_id();
    std::optional<RendererDebugTexture> resources_;
    NativeTu15DebugTextureBinding record_{};
};
}
