#pragma once
#include "halo2/renderer_targets.hpp"
#include <span>
#include <vulkan/vulkan.h>

namespace halo2 {
struct RendererDebugTextureContext {VkPhysicalDevice physical{};VkDevice device{};std::uint32_t queue_family{};};
class RendererDebugTextureFactory;
// Original1EFD0's256x256 linear A4R4G4B4 texture. Construction allocates
// genuine GPU storage but leaves its content and layout undefined. The first
// producer must supply every source texel before sampling is admitted.
// Packed core formats preserve the source words using an image-view channel
// permutation. RGBA8 fallback expands each nibble exactly by17. Neither path
// changes shaders or infers original sampler state.
class RendererDebugTexture final {
    struct Impl;
public:
    RendererDebugTexture(RendererDebugTexture&&) noexcept;
    RendererDebugTexture& operator=(RendererDebugTexture&&) noexcept;
    ~RendererDebugTexture();
    bool valid() const;
    const RendererTargetImage& image() const;
    VkComponentMapping components() const;
    NativeRuntimeAddress device_address() const;
    RendererDebugTextureContext context() const;
    bool content_initialized() const;
    // Complete original tightly packed linear surface. Undefined content is
    // never copied or sampled; this explicit producer owns the first upload.
    void upload_linear(std::span<const std::byte>);
    // Synchronous diagnostic source-word readback after a completed upload.
    std::vector<std::byte> pixels() const;
private:
    friend class RendererDebugTextureFactory;
    explicit RendererDebugTexture(std::unique_ptr<Impl>);
    std::unique_ptr<Impl> impl_;
};
class RendererDebugTextureFactory final {
    struct State;
public:
    RendererDebugTextureFactory()=default;
    bool valid() const;
    NativeRuntimeAddress device_address() const;
    RendererDebugTexture create() const;
private:
    friend class Renderer;
    static RendererDebugTextureFactory issue(RendererDeviceLease,VkPhysicalDevice,std::uint32_t);
    std::shared_ptr<State> state_;
};
}
