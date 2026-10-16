#pragma once
#include "halo2/renderer_targets.hpp"
#include <array>
#include <vulkan/vulkan.h>

namespace halo2 {
struct RendererAuxTextureContext {VkPhysicalDevice physical{};VkDevice device{};std::uint32_t queue_family{};};
struct RendererAuxTextureImage {
    RendererTargetImage image{0,0,NativeRuntimeAddress{},NativeRuntimeAddress{},0,0,0,0,0,0,0};
    std::uint32_t depth{},source_format_word{};
    bool cube{};
};
class RendererAuxTextureFactory;
// Original1D0C0's4x4 2D,4x4x4 volume and six-face4x4 cube. One mip each.
// Source A4R4G4B4 texels are losslessly expanded to RGBA8 before GPUupload;
// original0x0F00/0xF0F0 includes transparent red and opaque green. Images
// return only after actual upload completion, in SHADER_READ_ONLY_OPTIMAL.
// No sampler state or game draw/shader behavior is inferred by construction.
class RendererAuxTextures final {
    struct Impl;
public:
    RendererAuxTextures(RendererAuxTextures&&) noexcept;
    RendererAuxTextures& operator=(RendererAuxTextures&&) noexcept;
    ~RendererAuxTextures();
    bool valid() const;
    const RendererAuxTextureImage& image(unsigned) const;
    NativeRuntimeAddress device_address() const;
    RendererAuxTextureContext context() const;
    // Synchronous diagnostic readback, preserving the sampled image layout.
    std::vector<std::byte> pixels(unsigned) const;
private:
    friend class RendererAuxTextureFactory;
    explicit RendererAuxTextures(std::unique_ptr<Impl>);
    std::unique_ptr<Impl> impl_;
};
class RendererAuxTextureFactory final {
    struct State;
public:
    RendererAuxTextureFactory()=default;
    bool valid() const;
    NativeRuntimeAddress device_address() const;
    RendererAuxTextures create() const;
private:
    friend class Renderer;
    static RendererAuxTextureFactory issue(RendererDeviceLease,VkPhysicalDevice,std::uint32_t);
    std::shared_ptr<State> state_;
};
}
