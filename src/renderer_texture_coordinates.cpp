#include "halo2/renderer_texture_coordinates.hpp"
#include <stdexcept>
namespace halo2 {
RendererTextureCoordinates RendererTextureCoordinates::issue(RendererDeviceLease device,
    std::function<RendererTextureCoordinateConfiguration(unsigned,std::uint32_t)> select) {
    const auto use=device.acquire();
    if(!select)throw std::invalid_argument("Native texture-coordinate implementation is unavailable");
    RendererTextureCoordinates result;result.device_=std::move(device);result.select_=std::move(select);return result;
}
bool RendererTextureCoordinates::valid() const {return bool(select_) && device_.valid();}
NativeRuntimeAddress RendererTextureCoordinates::device_address() const {
    if(!valid())throw std::runtime_error("Native texture-coordinate renderer has retired");
    return device_.acquire().native_address();
}
RendererTextureCoordinateConfiguration RendererTextureCoordinates::select(unsigned stage,std::uint32_t value) const {
    if(!valid())throw std::runtime_error("Native texture-coordinate renderer has retired");
    const auto use=device_.acquire();return select_(stage,value);
}
}
