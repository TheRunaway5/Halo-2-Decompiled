#include "halo2/renderer_targets.hpp"
#include <stdexcept>
namespace halo2 {
RendererTargets RendererTargets::issue(RendererDeviceLease device,
    std::function<RendererTargetBank()> query,std::function<std::uint64_t()> generation,
    std::function<void(std::uint32_t)> source_buffers,std::function<std::vector<std::byte>(std::uint32_t)> readback,
    std::function<SourceWeatherDepthAlias&()> depth_aliases) {
    (void)device.acquire();
    if(!query || !generation || !source_buffers || !readback || !depth_aliases)throw std::invalid_argument("Native renderer target implementation is unavailable");
    RendererTargets result;result.device_=std::move(device);result.query_=std::move(query);
    result.generation_=std::move(generation);result.source_buffers_=std::move(source_buffers);
    result.readback_=std::move(readback);
    result.depth_aliases_=std::move(depth_aliases);
    result.thread_=std::this_thread::get_id();return result;
}
bool RendererTargets::valid() const {return bool(query_) && device_.valid();}
RendererTargets::Use RendererTargets::acquire() const {
    if(!valid())throw std::runtime_error("Native renderer target owner is unavailable");
    if(std::this_thread::get_id()!=thread_)
        throw std::invalid_argument("Native renderer targets belong to another thread");
    auto owner=device_.acquire();auto bank=query_();
    if(!bank.generation || bank.generation!=generation_() || bank.color.empty() || !bank.depth_stencil.image)
        throw std::runtime_error("Native renderer target bank has no live attachments");
    return Use(device_,std::move(owner),std::move(bank),generation_,readback_,depth_aliases_,
        [query=query_]{return query().next_source_color;},thread_);
}
RendererTargets::Use RendererTargets::source_buffers(std::uint32_t count) const {
    if(!valid())throw std::runtime_error("Native source color buffers lack their renderer owner");
    if(std::this_thread::get_id()!=thread_ || count!=2)
        throw std::invalid_argument("Native source color-buffer request has another thread or unsupported count");
    const auto owner=device_.acquire();source_buffers_(count);return acquire();
}
RendererTargets::Use::Use(RendererDeviceLease device,RendererDeviceLease::Use owner,
    RendererTargetBank bank,std::function<std::uint64_t()> generation,
    std::function<std::vector<std::byte>(std::uint32_t)> readback,
    std::function<SourceWeatherDepthAlias&()> depth_aliases,std::function<std::uint32_t()> current_color,std::thread::id thread)
    :device_(std::move(device)),owner_(std::move(owner)),bank_(std::move(bank)),
     generation_(std::move(generation)),readback_(std::move(readback)),depth_aliases_(std::move(depth_aliases)),current_color_(std::move(current_color)),thread_(thread){}
bool RendererTargets::Use::valid() const {
    if(std::this_thread::get_id()!=thread_ || !generation_ || !device_.valid())return false;
    return generation_()==bank_.generation;
}
const RendererTargetBank& RendererTargets::Use::bank() const {
    if(!valid())throw std::runtime_error("Native renderer targets have retired or been recreated");
    return bank_;
}
NativeRuntimeAddress RendererTargets::Use::device_address() const {
    (void)bank();return owner_.native_address();
}
std::vector<std::byte> RendererTargets::Use::source_color_pixels(std::uint32_t slot) const {
    if(!bank().source_double_buffer || slot>=2)throw std::invalid_argument("Source color readback needs a native two-buffer slot");
    return readback_(slot);
}
SourceWeatherDepthAlias& RendererTargets::Use::depth_aliases() const {
    if(!bank().source_double_buffer || bank().color.size()!=2)
        throw std::invalid_argument("Native depth aliases require the original two physical color banks");
    return depth_aliases_();
}
std::uint32_t RendererTargets::Use::current_source_color() const {
    if(!bank().source_double_buffer || bank().color.size()!=2)
        throw std::invalid_argument("Native surface rotation requires the original two physical color banks");
    const auto slot=current_color_();
    if(slot>=2)throw std::runtime_error("Native source frame ordinal selected an unavailable color bank");
    return slot;
}
}
