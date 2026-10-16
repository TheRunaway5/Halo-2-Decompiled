#pragma once
#include "halo2/renderer_targets.hpp"
#include <vulkan/vulkan.h>

namespace halo2 {
class RendererTexturePairFactory;
struct RendererTexturePairContext {VkPhysicalDevice physical{};VkDevice device{};std::uint32_t queue_family{};};
// Two independent64x64 A8R8G8B8 textures from original14830. Each image is
// both sampled texture and color surface; allocation leaves content/layout
// undefined, just as the original constructor does. Native texel bytes are
// unswizzled BGRA; Vulkan handles the physical tiling. Consumers supply their
// original sampler/draw state and must keep this owner alive through commands.
class RendererTexturePairs final {
    struct Impl;
public:
    RendererTexturePairs(RendererTexturePairs&&) noexcept;
    RendererTexturePairs& operator=(RendererTexturePairs&&) noexcept;
    ~RendererTexturePairs();
    bool valid() const;
    const RendererTargetImage& image(unsigned) const;
    NativeRuntimeAddress device_address() const;
    RendererTexturePairContext context() const;
private:
    friend class RendererTexturePairFactory;
    explicit RendererTexturePairs(std::unique_ptr<Impl>);
    std::unique_ptr<Impl> impl_;
};
// Only Renderer issues this factory. Instances retain its genuine device,
// reject foreign threads/retirement, and own independent GPU allocations.
class RendererTexturePairFactory final {
    struct State;
public:
    RendererTexturePairFactory()=default;
    bool valid() const;
    NativeRuntimeAddress device_address() const;
    RendererTexturePairs create() const;
private:
    friend class Renderer;
    static RendererTexturePairFactory issue(RendererDeviceLease,VkPhysicalDevice,std::uint32_t);
    std::shared_ptr<State> state_;
};
}
