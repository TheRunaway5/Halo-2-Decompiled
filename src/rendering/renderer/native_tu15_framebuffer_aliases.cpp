#include "halo2/native_tu15_framebuffer_aliases.hpp"
#include "halo2/source_weather_depth_alias.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
NativeTu15FramebufferAliases::NativeTu15FramebufferAliases(RendererTargets::Use owner):owner_(std::move(owner)) {
    const auto& bank=owner_.bank();
    if(!bank.source_double_buffer || bank.color.size()!=2 || bank.depth_stencil.format!=VK_FORMAT_D24_UNORM_S8_UINT)
        throw std::invalid_argument("Native source aliases require two owned colors and the actual D24S8 surface");
    const auto descriptor=[](std::uint32_t cell,NativeTu15FramebufferKind kind,const RendererTargetImage& image,
                             std::uint32_t source_format=0) {
        return NativeTu15FramebufferBinding{cell,kind,image.image,image.view,0,image.image,
            image.format,image.width,image.height,source_format};
    };
    bindings_[0]=descriptor(0x515e34,NativeTu15FramebufferKind::ColorTexture,bank.color[0],0x12);
    bindings_[1]=descriptor(0x515e38,NativeTu15FramebufferKind::ColorTexture,bank.color[1],0x12);
    bindings_[2]=descriptor(0x515e3c,NativeTu15FramebufferKind::ColorSurface,bank.color[0]);
    bindings_[3]=descriptor(0x515e40,NativeTu15FramebufferKind::ColorSurface,bank.color[1]);
    bindings_[4]=descriptor(0x515e44,NativeTu15FramebufferKind::DepthSurface,bank.depth_stencil);
    auto& producer=owner_.depth_aliases();
    for(unsigned i=0;i<2;++i) {
        const auto format=i?SourceWeatherDepthAliasFormat::linear_a8b8g8r8:SourceWeatherDepthAliasFormat::linear_a8r8g8b8;
        const auto alias=producer.prepared_binding(format);
        bindings_[5+i]={0x515e48+i*4,NativeTu15FramebufferKind::DepthTexture,
            std::bit_cast<std::uint64_t>(alias.image),std::bit_cast<std::uint64_t>(alias.view),
            std::bit_cast<std::uint64_t>(alias.sampler),bank.depth_stencil.image,
            std::uint32_t(alias.format),alias.extent.width,alias.extent.height,std::uint32_t(format)};
    }
    bindings_[7]=bindings_[5];bindings_[7].source_cell=0x515e50;
    bindings_[7].kind=NativeTu15FramebufferKind::DepthColorSurface;bindings_[7].sampler=0;
    bindings_[7].source_texture_format=0;
}
bool NativeTu15FramebufferAliases::valid()const{return owner_.valid();}
const NativeTu15FramebufferBinding& NativeTu15FramebufferAliases::resolve(std::uint32_t cell)const {
    const auto& bank=owner_.bank();
    // Original SDK403B50 rotates surface Data fields at presentation while
    // the texture headers created at145E0 retain fixed physical addresses.
    if(cell==0x515e3c || cell==0x515e40) {
        const auto index=cell==0x515e3c?2u:3u;
        const auto& image=bank.color[owner_.current_source_color()^(index-2)];
        auto& binding=bindings_[index];binding.image=binding.parent_image=image.image;binding.view=image.view;
    }
    for(const auto& binding:bindings_)if(binding.source_cell==cell)return binding;
    throw std::invalid_argument("Original framebuffer identity has no native alias binding");
}
NativeRuntimeAddress NativeTu15FramebufferAliases::native_address(std::uint32_t cell)const {
    return NativeRuntimeAddress{std::uint64_t(reinterpret_cast<std::uintptr_t>(&resolve(cell)))};
}
SourceWeatherDepthAlias& NativeTu15FramebufferAliases::depth_producer()const{return owner_.depth_aliases();}
const RendererTargetBank& NativeTu15FramebufferAliases::targets()const{return owner_.bank();}
}
