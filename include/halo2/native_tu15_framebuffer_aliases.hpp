#pragma once
#include "halo2/renderer_targets.hpp"
#include <array>

namespace halo2 {
enum class NativeTu15FramebufferKind { ColorTexture, ColorSurface, DepthSurface, DepthTexture, DepthColorSurface };
struct NativeTu15FramebufferBinding {
    // Original global is an identity key, never a native pointer or SDK blob.
    std::uint32_t source_cell{};
    NativeTu15FramebufferKind kind{};
    std::uint64_t image{},view{},sampler{},parent_image{};
    std::uint32_t format{},width{},height{},source_texture_format{};
};
// Native registry for all eight original145E0 identities. Holds the genuine
// renderer use; its records and resource handles expire on resize/retirement.
// No source pointer cells are published and no image is claimed ready merely
// because its producer and descriptors have been prepared. Borrowed records
// and native addresses also expire when this registry moves or is destroyed.
class NativeTu15FramebufferAliases final {
public:
    explicit NativeTu15FramebufferAliases(RendererTargets::Use);
    NativeTu15FramebufferAliases(NativeTu15FramebufferAliases&&) noexcept=default;
    NativeTu15FramebufferAliases& operator=(NativeTu15FramebufferAliases&&) noexcept=default;
    bool valid() const;
    const NativeTu15FramebufferBinding& resolve(std::uint32_t source_cell) const;
    NativeRuntimeAddress native_address(std::uint32_t source_cell) const;
    SourceWeatherDepthAlias& depth_producer() const;
    const RendererTargetBank& targets() const;
private:
    RendererTargets::Use owner_;
    mutable std::array<NativeTu15FramebufferBinding,8> bindings_{};
};
}
