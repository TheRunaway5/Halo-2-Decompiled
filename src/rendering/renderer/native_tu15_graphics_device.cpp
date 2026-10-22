#include "halo2/native_tu15_graphics_device.hpp"
namespace halo2 {
NativeTu15GraphicsDevice::NativeTu15GraphicsDevice(SourceRuntimeMemory& memory,RendererDeviceLease renderer,RendererTargets targets)
    :memory_(memory),renderer_(std::move(renderer)),targets_(std::move(targets)),thread_(std::this_thread::get_id()){}
bool NativeTu15GraphicsDevice::initialized() const {
    if(!handle_ || !renderer_.valid())return false;
    const auto bytes=memory_.view(0x515e90,4);std::uint32_t published=0;
    for(unsigned i=0;i<4;++i)published|=std::to_integer<std::uint32_t>(bytes[i])<<(8*i);
    return published==handle_;
}
NativeRuntimeAddress NativeTu15GraphicsDevice::native_address() const {
    if(!initialized())throw std::runtime_error("Native source graphics device has no live published binding");
    return renderer_.acquire().native_address();
}
RendererTargets::Use NativeTu15GraphicsDevice::target_resources() const {
    if(std::this_thread::get_id()!=thread_ || !initialized())
        throw std::runtime_error("Native graphics target request lacks its live device/thread owner");
    auto use=targets_.acquire();
    if(use.device_address()!=native_address())throw std::invalid_argument("Native graphics targets belong to another renderer device");
    return use;
}
NativeTu15FramebufferAliases NativeTu15GraphicsDevice::framebuffer_aliases()const {
    return NativeTu15FramebufferAliases(target_resources());
}
bool NativeTu15GraphicsDevice::try_call(SourcePhysicsContactMemory& bank,
    const SourcePhysicsContactRuntimeCall& call,SourcePhysicsWorldRuntimeAbi& abi) {
    if(call.target!=0x3ff320u)return false;
    if(&bank!=&memory_ || std::this_thread::get_id()!=thread_)
        throw std::invalid_argument("Native graphics device request has a foreign bank or thread");
    const auto frame=native_tu15_renderer_device_frame(memory_,call,abi);
    constexpr std::array<std::uint32_t,17> admitted{640,480,6,0,0,1,0,0,1,42,1,60,1,0,0,0,0};
    if(frame.presentation!=admitted || handle_)
        throw std::invalid_argument("Native graphics device request has an unowned configuration or duplicate cold creation");
    for(const auto byte:memory_.view(frame.output_token,4))if(byte!=std::byte{})
        throw std::invalid_argument("Native graphics cold creation would replace a published device");
    const auto use=renderer_.acquire(); // Retain the actual Vulkan implementation through publication.
    if(!use.native_address().value)throw std::runtime_error("Native graphics device owner has no real identity");
    if(targets_.acquire().device_address()!=use.native_address())
        throw std::invalid_argument("Native source color buffers belong to another renderer device");
    const auto buffers=targets_.source_buffers(2);
    if(!buffers.bank().source_double_buffer || buffers.bank().color.size()!=2)
        throw std::runtime_error("Native graphics device lacks its original two physical color banks");
    // A native logical handle, distinct from both Xbox SDK object addresses
    // and the actual Vulkan device. Later native graphics methods resolve it
    // through this same live renderer owner.
    constexpr std::uint32_t token=0xf9000001u;
    std::array<std::byte,4> bytes{};for(unsigned i=0;i<4;++i)bytes[i]=std::byte(token>>(8*i));
    memory_.write(frame.output_token,bytes);
    handle_=token;abi.gp[0]=0;abi.gp[4]+=8;
    return true;
}
}
