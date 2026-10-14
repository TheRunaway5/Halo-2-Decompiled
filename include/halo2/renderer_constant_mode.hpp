#pragma once
#include "halo2/renderer_device_lease.hpp"
#include <functional>
namespace halo2 {
class Renderer;
struct RendererConstantConfiguration {
    std::uint32_t mode{},registers{};
    bool application_owned{};
    std::uint64_t material_bytes{},extended_bytes{};
    NativeRuntimeAddress material_backing,extended_backing;
};
// A live Renderer alone issues this control. The native backend already uses
// application-owned logical c0..191; selection checks its real storage and
// descriptor capacity rather than constructing an Xbox SDK device bank.
class RendererConstantMode {
public:
    RendererConstantMode()=default;
    bool valid() const;
    NativeRuntimeAddress device_address() const;
    RendererConstantConfiguration select(std::uint32_t) const;
private:
    friend class Renderer;
    static RendererConstantMode issue(RendererDeviceLease,
        std::function<RendererConstantConfiguration(std::uint32_t)>);
    RendererDeviceLease device_;
    std::function<RendererConstantConfiguration(std::uint32_t)> select_;
};
}
