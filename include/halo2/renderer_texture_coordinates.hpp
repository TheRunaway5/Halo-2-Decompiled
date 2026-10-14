#pragma once
#include "halo2/renderer_device_lease.hpp"
#include <array>
#include <functional>
namespace halo2 {
class Renderer;
inline constexpr std::array<std::uint32_t,4> renderer_passthrough_registers{9,10,11,12};
struct RendererTextureCoordinateConfiguration {
    std::array<std::uint32_t,4> registers{};
    std::uint64_t pipeline_handle{},material_bytes{};
    NativeRuntimeAddress state_backing,material_backing;
};
// The retained programmable world shader routes oT0..3 from o9..12. This
// control admits its exact cold identity selection and disabled texgen only.
class RendererTextureCoordinates {
public:
    bool valid() const;
    NativeRuntimeAddress device_address() const;
    RendererTextureCoordinateConfiguration select(unsigned,std::uint32_t) const;
private:
    friend class Renderer;
    static RendererTextureCoordinates issue(RendererDeviceLease,
        std::function<RendererTextureCoordinateConfiguration(unsigned,std::uint32_t)>);
    RendererDeviceLease device_;
    std::function<RendererTextureCoordinateConfiguration(unsigned,std::uint32_t)> select_;
};
}
