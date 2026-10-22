#include "halo2/renderer_output_state.hpp"
#include <stdexcept>
namespace halo2 {
RendererOutputState RendererOutputState::issue(RendererDeviceLease device,
    std::function<RendererOutputConfiguration(RendererOutputSetting,std::uint32_t)> select) {
    const auto use=device.acquire();
    if(!select)throw std::invalid_argument("Native output-state implementation is unavailable");
    RendererOutputState result;result.device_=std::move(device);result.select_=std::move(select);return result;
}
bool RendererOutputState::valid() const {return bool(select_) && device_.valid();}
NativeRuntimeAddress RendererOutputState::device_address() const {
    if(!valid())throw std::runtime_error("Native output-state renderer has retired");
    return device_.acquire().native_address();
}
RendererOutputConfiguration RendererOutputState::select(RendererOutputSetting setting,std::uint32_t value) const {
    if(!valid())throw std::runtime_error("Native output-state renderer has retired");
    const auto use=device_.acquire();return select_(setting,value);
}
}
