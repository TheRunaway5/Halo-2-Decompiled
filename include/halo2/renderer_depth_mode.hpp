#pragma once
#include "halo2/renderer_device_lease.hpp"
#include <functional>
namespace halo2 {
class Renderer;
struct RendererDepthConfiguration {
    std::uint32_t mode{},format{},depth_bits{},stencil_bits{};
    std::uint64_t attachment_handle{};
    NativeRuntimeAddress state_backing;
};
// Issued only by a live Renderer. Selects the existing world W-depth path;
// each authored pass retains its own depth/stencil write and compare policy.
class RendererDepthMode {
public:
    RendererDepthMode()=default;
    bool valid() const;
    NativeRuntimeAddress device_address() const;
    RendererDepthConfiguration select(std::uint32_t) const;
private:
    friend class Renderer;
    static RendererDepthMode issue(RendererDeviceLease,
        std::function<RendererDepthConfiguration(std::uint32_t)>);
    RendererDeviceLease device_;
    std::function<RendererDepthConfiguration(std::uint32_t)> select_;
};
}
