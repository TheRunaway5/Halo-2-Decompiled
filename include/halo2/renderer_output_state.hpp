#pragma once
#include "halo2/renderer_device_lease.hpp"
#include <array>
#include <functional>
namespace halo2 {
class Renderer;
enum class RendererOutputSetting : unsigned {
    FillOffsetEnable, AlphaTestEnable, AlphaFunction, AlphaReference,
    BlendEnable, BlendSource, BlendDestination, BlendEquation
};
// Cold12560 state, before authored passes replace their own output policy.
inline constexpr std::array<std::uint32_t,8> renderer_cold_output_values{0,0,0x204,0,0,1,0,0x8006};
struct RendererOutputConfiguration {
    std::array<std::uint32_t,8> values{};
    std::uint64_t pipeline_handle{};
    NativeRuntimeAddress state_backing,material_backing;
};
class RendererOutputState {
public:
    RendererOutputState()=default;
    bool valid() const;
    NativeRuntimeAddress device_address() const;
    RendererOutputConfiguration select(RendererOutputSetting,std::uint32_t) const;
private:
    friend class Renderer;
    static RendererOutputState issue(RendererDeviceLease,
        std::function<RendererOutputConfiguration(RendererOutputSetting,std::uint32_t)>);
    RendererDeviceLease device_;
    std::function<RendererOutputConfiguration(RendererOutputSetting,std::uint32_t)> select_;
};
}
