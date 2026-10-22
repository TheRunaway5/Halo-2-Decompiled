#include "halo2/native_tu15_renderer_palette.hpp"
namespace halo2 {
namespace {
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t at) {
    const auto bytes=memory.view(at,4);std::uint32_t value=0;
    for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(i*8);
    return value;
}
}
NativeTu15RendererPalette::NativeTu15RendererPalette(SourceRuntimeMemory& memory,
    NativeTu15GraphicsDevice& device,RendererPaletteFactory factory)
    :memory_(memory),device_(&device),factory_(std::move(factory)),thread_(std::this_thread::get_id()){}
NativeTu15RendererPalette::NativeTu15RendererPalette(SourceRuntimeMemory& memory,RendererPaletteFactory factory)
    :memory_(memory),device_(nullptr),factory_(std::move(factory)),thread_(std::this_thread::get_id()) {
    if(!factory_.valid() || !factory_.device_address().value)
        throw std::runtime_error("Native world palette requires its actual live renderer factory");
    if(word(memory_,0x4915dcu))throw std::invalid_argument("Native world palette would replace a published source resource");
    const auto colors=memory_.view(0x474da8u,1024);
    auto palette=factory_.create(std::span<const std::byte,1024>(colors.data(),1024));
    constexpr std::uint32_t handle=0xf9000101u;
    std::array<std::byte,4> bytes{};for(unsigned i=0;i<4;++i)bytes[i]=std::byte(handle>>(8*i));
    memory_.write(0x4915dcu,bytes);palette_=std::move(palette);handle_=handle;
}
bool NativeTu15RendererPalette::initialized() const {
    return handle_ && (!device_ || device_->initialized()) && factory_.valid() &&
        palette_.valid() && word(memory_,0x4915dcu)==handle_;
}
NativeRuntimeAddress NativeTu15RendererPalette::native_address() const {
    if(!initialized())throw std::runtime_error("Native source palette has no live renderer resource");
    return palette_.native_address();
}
std::span<const std::byte,1024> NativeTu15RendererPalette::colors() const {
    if(!initialized())throw std::runtime_error("Native source palette has no live renderer resource");
    return palette_.colors();
}
bool NativeTu15RendererPalette::try_call(SourcePhysicsContactMemory& bank,
    const SourcePhysicsContactRuntimeCall& call,SourcePhysicsWorldRuntimeAbi& abi) {
    if(call.target!=0x1d3d0u && call.target!=0x4025b0u)return false;
    if(&bank!=&memory_ || call.tail || std::this_thread::get_id()!=thread_)
        throw std::invalid_argument("Native palette request has a foreign bank, thread or tail frame");
    if((device_ && !device_->initialized()) || !factory_.valid() ||
       (device_ && factory_.device_address()!=device_->native_address()))
        throw std::runtime_error("Native palette request lacks its live renderer device owner");
    const auto sp=abi.gp[4];
    if(call.target==0x1d3d0u) {
        if(call.instruction!=0x125e6u || word(memory_,sp)!=0x125ebu || abi.gp[0]!=3u ||
           word(memory_,sp+4)!=0x474da8u || word(memory_,sp+8)!=0x4915dcu || handle_ || word(memory_,0x4915dcu))
            throw std::invalid_argument("Native cold palette creation caller or configuration differs");
        const auto colors=memory_.view(0x474da8,1024);
        auto palette=factory_.create(std::span<const std::byte,1024>(colors.data(),1024));
        constexpr std::uint32_t handle=0xf9000101u;
        std::array<std::byte,4> bytes{};for(unsigned i=0;i<4;++i)bytes[i]=std::byte(handle>>(i*8));
        memory_.write(0x4915dc,bytes);palette_=std::move(palette);handle_=handle;
        // Native stdcall graphics ABI: Boolean EAX and two arguments;
        // other GP, defined FP and flags are retained.
        abi.gp[0]=1;abi.gp[4]+=12;return true;
    }
    constexpr std::array<std::uint32_t,4> sites{0x125fd,0x1260e,0x1261f,0x1262f};
    const auto stage=abi.gp[0];
    const bool bsp=call.instruction==0x15b68u;
    if(stage>=4 || (!bsp && (!device_ || call.instruction!=sites[stage])) || word(memory_,sp)!=call.instruction+5 ||
       !initialized() || word(memory_,sp+4)!=handle_ || stage!=(bsp?bindings_%4u:bindings_))
        throw std::invalid_argument("Native palette stage binding caller, order or resource differs");
    factory_.bind(stage,palette_);++bindings_;abi.gp[4]+=8;return true;
}
}
