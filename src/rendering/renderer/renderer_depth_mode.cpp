#include "halo2/renderer_depth_mode.hpp"
#include <stdexcept>
namespace halo2 {
RendererDepthMode RendererDepthMode::issue(RendererDeviceLease device,
    std::function<RendererDepthConfiguration(RendererDepthSetting,std::uint32_t)> select) {
    const auto use=device.acquire();
    if(!select)throw std::invalid_argument("Native depth-mode implementation is unavailable");
    RendererDepthMode result;result.device_=std::move(device);result.select_=std::move(select);return result;
}
bool RendererDepthMode::valid() const {return bool(select_) && device_.valid();}
NativeRuntimeAddress RendererDepthMode::device_address() const {
    if(!valid())throw std::runtime_error("Native depth-mode renderer has retired");
    return device_.acquire().native_address();
}
RendererDepthConfiguration RendererDepthMode::select(std::uint32_t mode) const {
    return apply(RendererDepthSetting::Mode,mode);
}
RendererDepthConfiguration RendererDepthMode::write_enable(std::uint32_t value) const {
    return apply(RendererDepthSetting::WriteEnable,value);
}
RendererDepthConfiguration RendererDepthMode::function(std::uint32_t value) const {
    return apply(RendererDepthSetting::Function,value);
}
RendererDepthConfiguration RendererDepthMode::bias(std::uint32_t value) const {
    return apply(RendererDepthSetting::Bias,value);
}
RendererDepthConfiguration RendererDepthMode::apply(RendererDepthSetting setting,std::uint32_t value) const {
    if(!valid())throw std::runtime_error("Native depth-mode renderer has retired");
    const auto use=device_.acquire();return select_(setting,value);
}
}
