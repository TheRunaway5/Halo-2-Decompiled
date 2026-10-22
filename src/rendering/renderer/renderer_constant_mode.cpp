#include "halo2/renderer_constant_mode.hpp"
#include <stdexcept>
namespace halo2 {
RendererConstantMode RendererConstantMode::issue(RendererDeviceLease device,
    std::function<RendererConstantConfiguration(std::uint32_t)> select) {
    const auto use=device.acquire();
    if(!select)throw std::invalid_argument("Native constant-mode implementation is unavailable");
    RendererConstantMode result;result.device_=std::move(device);result.select_=std::move(select);return result;
}
bool RendererConstantMode::valid() const {return bool(select_) && device_.valid();}
NativeRuntimeAddress RendererConstantMode::device_address() const {
    if(!valid())throw std::runtime_error("Native constant-mode renderer has retired");
    return device_.acquire().native_address();
}
RendererConstantConfiguration RendererConstantMode::select(std::uint32_t mode) const {
    if(!valid())throw std::runtime_error("Native constant-mode renderer has retired");
    const auto use=device_.acquire();return select_(mode);
}
}
