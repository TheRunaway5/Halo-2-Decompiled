#pragma once
#include "halo2/renderer_device_lease.hpp"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
namespace halo2 {
class Renderer;
struct Texture;
class RendererPaletteFactory;
// Vulkan samples expanded P8 texels. This resource owns the exact immutable
// colors used before filtering and belongs to one live renderer factory.
class RendererPalette {
    struct Impl;
public:
    RendererPalette()=default;
    bool valid() const;
    NativeRuntimeAddress native_address() const;
    std::span<const std::byte,1024> colors() const;
    Texture decode(std::span<const std::byte>,unsigned width,unsigned height,
        bool swizzled,unsigned additional_mips=0) const;
private:
    friend class RendererPaletteFactory;
    std::shared_ptr<Impl> impl_;
};
// Only Renderer issues a live factory. Copies share its actual stage bindings;
// presentation/device retirement invalidates new resource access.
class RendererPaletteFactory {
    struct State;
public:
    RendererPaletteFactory()=default;
    bool valid() const;
    NativeRuntimeAddress device_address() const;
    RendererPalette create(std::span<const std::byte,1024>) const;
    void bind(unsigned stage,const RendererPalette&) const;
    RendererPalette bound(unsigned stage) const;
private:
    friend class Renderer;
    static RendererPaletteFactory issue(RendererDeviceLease);
    std::shared_ptr<State> state_;
};
}
