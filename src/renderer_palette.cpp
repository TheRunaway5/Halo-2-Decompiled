#include "halo2/renderer_palette.hpp"
#include "halo2/texture.hpp"
#include <algorithm>
#include <mutex>
#include <stdexcept>
namespace halo2 {
struct RendererPalette::Impl {
    RendererDeviceLease device;
    std::weak_ptr<void> factory;
    std::array<std::byte,1024> colors;
};
struct RendererPaletteFactory::State {
    explicit State(RendererDeviceLease value):device(std::move(value)){}
    RendererDeviceLease device;
    mutable std::mutex gate;
    std::array<RendererPalette,4> stages;
};
bool RendererPalette::valid() const {
    return impl_ && !impl_->factory.expired() && impl_->device.valid();
}
NativeRuntimeAddress RendererPalette::native_address() const {
    if(!valid())throw std::runtime_error("Native renderer palette has retired");
    const auto use=impl_->device.acquire();
    return NativeRuntimeAddress{std::uint64_t(reinterpret_cast<std::uintptr_t>(impl_->colors.data()))};
}
std::span<const std::byte,1024> RendererPalette::colors() const {
    if(!valid())throw std::runtime_error("Native renderer palette has retired");
    const auto use=impl_->device.acquire();return impl_->colors;
}
Texture RendererPalette::decode(std::span<const std::byte> raw,unsigned width,unsigned height,
    bool swizzled,unsigned mips) const {
    if(!valid())throw std::runtime_error("Native renderer palette has retired");
    const auto use=impl_->device.acquire();
    return prepare_palettized_texture(Bytes(raw),width,height,swizzled,impl_->colors,mips);
}
RendererPaletteFactory RendererPaletteFactory::issue(RendererDeviceLease device) {
    const auto use=device.acquire();
    RendererPaletteFactory result;result.state_=std::make_shared<State>(std::move(device));return result;
}
bool RendererPaletteFactory::valid() const {return state_ && state_->device.valid();}
NativeRuntimeAddress RendererPaletteFactory::device_address() const {
    if(!valid())throw std::runtime_error("Native renderer palette factory has retired");
    return state_->device.acquire().native_address();
}
RendererPalette RendererPaletteFactory::create(std::span<const std::byte,1024> colors) const {
    if(!valid())throw std::runtime_error("Native renderer palette factory has retired");
    const auto use=state_->device.acquire();
    RendererPalette result;result.impl_=std::make_shared<RendererPalette::Impl>();
    result.impl_->device=state_->device;result.impl_->factory=state_;
    std::ranges::copy(colors,result.impl_->colors.begin());return result;
}
void RendererPaletteFactory::bind(unsigned stage,const RendererPalette& palette) const {
    if(stage>=4 || !palette.valid() || !valid() || palette.impl_->factory.lock().get()!=state_.get())
        throw std::invalid_argument("Native palette binding has a foreign owner or stage");
    const auto use=state_->device.acquire();
    std::lock_guard lock(state_->gate);state_->stages[stage]=palette;
}
RendererPalette RendererPaletteFactory::bound(unsigned stage) const {
    if(stage>=4 || !valid())throw std::invalid_argument("Native palette stage is unavailable");
    const auto use=state_->device.acquire();std::lock_guard lock(state_->gate);
    if(!state_->stages[stage].valid())throw std::runtime_error("Native palette stage is unbound");
    return state_->stages[stage];
}
}
