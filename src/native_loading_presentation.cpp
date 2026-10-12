#include "halo2/native_loading_presentation.hpp"
#include <bit>
#include <cmath>

namespace halo2 {
namespace {
constexpr std::array<unsigned,12> texture_slots{0,3,4,3,4,3,4,5,5,11,1,1};
constexpr std::array<unsigned,12> shader_slots{0,1,1,1,1,1,1,2,2,0,0,3};
VistaLoadingMaterialBlend blend(VistaLoadingBlend value) {
    switch(value) {
    case VistaLoadingBlend::one_one:return VistaLoadingMaterialBlend::OneOne;
    case VistaLoadingBlend::destination_color_zero:return VistaLoadingMaterialBlend::DestinationColorZero;
    case VistaLoadingBlend::source_alpha_inverse_source_alpha:return VistaLoadingMaterialBlend::SourceAlphaInverseSourceAlpha;
    }
    throw FormatError("unsupported original loading blend");
}
float real(std::uint32_t word) { return std::bit_cast<float>(word); }
float channel(std::uint32_t argb,unsigned shift) { return float((argb>>shift)&255u)/255.f; }
}
NativeLoadingPresentation::NativeLoadingPresentation(const VistaLoadingAssets& assets) {
    mesh_.materials.reserve(texture_slots.size());vertices_.reserve(72);
    for(unsigned i=0;i<texture_slots.size();++i) {
        const auto& asset=assets[texture_slots[i]];
        if(!asset.source_width || !asset.source_height || !asset.texture.width || !asset.texture.height ||
           asset.texture.format!=11 || asset.texture.pixels.empty())
            throw FormatError("missing original D3DX loading image");
        Material material;
        material.name="Vista loading draw "+std::to_string(i);
        material.diffuse=asset.texture;
        material.ui_full_surface=true;
        material.vista_loading=VistaLoadingMaterial{shader_slots[i],VistaLoadingMaterialBlend::OneOne,{},{}};
        auto& sampler=material.texture_states[0];
        sampler.address_w=3;
        if(shader_slots[i]==1)sampler.address_u=sampler.address_v=3;
        mesh_.materials.push_back(std::move(material));
    }
}
void NativeLoadingPresentation::update(std::span<const VistaLoadingDraw> draws,std::uint32_t width,
                                      std::uint32_t height,Vec2 offset) {
    if((draws.size()!=11 && draws.size()!=12) || !width || !height ||
       !std::isfinite(offset.x) || !std::isfinite(offset.y))
        throw FormatError("unsupported normal-mode loading presentation");
    vertices_.clear();
    for(unsigned i=0;i<draws.size();++i) {
        const auto& draw=draws[i];
        if(draw.shader!=shader_slots[i] || (draw.shader!=3 && draw.texture!=texture_slots[i]) ||
           (draw.primitive!=5 && draw.primitive!=6))
            throw FormatError("original normal-mode loading draw catalogue changed");
        auto& material=*mesh_.materials[i].vista_loading;
        material.shader=draw.shader;material.blend=blend(draw.blend);material.raster_offset_pixels=offset;
        if(draw.pixel_constant0)for(unsigned k=0;k<4;++k)material.constant0[k]=real((*draw.pixel_constant0)[k]);
        const std::array<unsigned,6> strip{0,1,2,2,1,3},fan{0,1,2,0,2,3};
        for(const auto index:draw.primitive==5?strip:fan) {
            const auto& source=draw.vertices[index];
            Vertex vertex;
            vertex.position={real(source.position[0]),real(source.position[1]),channel(source.argb,24)};
            vertex.color={channel(source.argb,16),channel(source.argb,8),channel(source.argb,0)};
            vertex.uv={real(source.texcoord0[0]),real(source.texcoord0[1])};
            vertex.lightmap_uv={real(source.texcoord1[0]),real(source.texcoord1[1])};
            vertex.material=i;vertices_.push_back(vertex);
        }
    }
}
}
