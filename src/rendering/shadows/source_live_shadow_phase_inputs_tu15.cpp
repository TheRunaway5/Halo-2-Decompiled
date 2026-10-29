#include "halo2/source_live_shadow_phase_inputs_tu15.hpp"
#include "halo2/menu_world_vertex_catalog.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/source_view_projection.hpp"
#include <vulkan/vulkan_core.h>
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>

namespace halo2 {
namespace {
template<class T>T read(const SourceRuntimeMemory& memory,std::uint32_t address) {
    T value;std::memcpy(&value,memory.view(address,sizeof(value)).data(),sizeof(value));return value;
}
template<class T>bool same(const T& a,const T& b) {
    return std::memcmp(&a,&b,sizeof(T))==0;
}
void attachments(const RendererTargets::Use& use) {
    if(!use.valid() || use.device_address().is_null())
        throw FormatError("live shadow phases require a live native Renderer attachment lease");
    const auto& bank=use.bank();
    const auto width=bank.depth_stencil.width,height=bank.depth_stencil.height;
    const auto valid=[&](const RendererTargetImage& image,std::uint32_t format,std::uint32_t usage) {
        return image.image && image.view && !image.image_backing.is_null() && !image.view_backing.is_null() &&
            image.format==format && width && height && image.width==width && image.height==height && image.samples==1 &&
            image.mips==1 && image.layers==1 && (image.usage&usage)==usage;
    };
    if(!bank.generation || bank.color.empty() ||
       !valid(bank.depth_stencil,VK_FORMAT_D24_UNORM_S8_UINT,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) ||
       std::any_of(bank.color.begin(),bank.color.end(),[&](const auto& image) {
           return !valid(image,VK_FORMAT_B8G8R8A8_UNORM,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
       }))
        throw FormatError("live shadow phases require matching actual single-sample BGRA8/integerD24S8 attachments");
}
NativeRuntimeAddress effect_backing(const SourceRuntimeMemory& memory,std::uint32_t address) {
    const auto arena=read<std::uint32_t>(memory,0x4f2950u),used=read<std::uint32_t>(memory,0x4f2954u);
    if(!address || address==0xffffffffu || !arena || address<arena ||
       std::uint64_t(address)+0x3fcu>std::uint64_t(arena)+used)
        throw FormatError("live shadow timed-effect body is outside the actually allocated game arena");
    const auto native=memory.native_address(address,0x3fcu);
    if(native.is_null() || memory.source_address(native,0x3fcu)!=address)
        throw FormatError("live shadow timed-effect body lacks complete native backing");
    return native;
}
void selected_states(const Cache& cache,const MenuWorldSourcePass& pass) {
    //16DCE/16DD5 and16E0E/16E16 send the default and main packed ranges to
    //188E0. MaterialPass::states retains only main; both real ranges matter.
    const Bytes impl(pass.implementation_record());
    if(ShaderRange{impl.u16(0x11e)}.count())
        throw FormatError("live shadow selected state extern requires its original function owner");
    const auto descriptor=read_tag_block_descriptor(
        Bytes(cache.read_tag_bytes(pass.pass_postprocess_address()+16u,8)),0);
    const auto extent=descriptor.extent(5,65536);
    const auto bytes=descriptor.count?cache.read_tag_bytes(descriptor.address,extent):std::span<const std::byte>{};
    for(unsigned offset:{2u,0x11cu}) {
        const ShaderRange range{impl.u16(offset)};
        const auto count=std::size_t(descriptor.count);
        if(range.first()>count || range.count()>count-range.first())
            throw FormatError("live shadow selected state range escapes its original pass owner");
        for(unsigned n=0;n<range.count();++n) {
            const auto index=std::to_integer<unsigned>(bytes[(range.first()+n)*5u]);
            //44540 always sets47361C=1. First4D9B0→1BBD0 resets both
            //CULL/FRONTFACE900, overriding earlier496E0Cull901. A selected
            //fixed/default override or AA writer needs a different owner.
            if(index==146 || index==147 || index==154 || index==155)
                throw FormatError("live shadow selected pass overrides the bounded cull/frontface/AA contract");
        }
    }
}
void completed_program(const SourceShadowMaterial& authored) {
    const auto& m=authored.material;
    const auto& p=m.passes.at(m.forward_pass_index);
    if(!p.state_externs.empty() || !p.vertex_externs.empty() ||
       std::any_of(p.pixel_externs.begin(),p.pixel_externs.end(),[](const auto& remap) {
           return (remap.source!=17 && remap.source!=22) || (remap.component&7u)>=4;
       }))
        throw FormatError("live shadow selected material requires an unresolved extern owner");
    if(std::any_of(p.states.begin(),p.states.end(),[](const auto& state) {
        return state.index==146 || state.index==147 || state.index==154 || state.index==155;
    }))throw FormatError("live shadow completed material changed bounded cull/frontface/AA state");
    //The raw VS FogParams only feeds location4. GS copies it; the current
    //fragment never loads it. Inspect FINAL completed selectors, including
    //E/F product and G alpha, as validate_menu_world_fragment does at record.
    const auto& words=m.forward_program.words;const auto count=words[53]&255u;
    if(count>8)throw FormatError("live shadow completed program exceeds original combiner stages");
    const auto operand=[](std::uint32_t value) {
        if((value&15u)==3)throw FormatError("live shadow completed program consumes unowned fog residue");
    };
    for(unsigned i=0;i<count;++i)for(unsigned shift=0;shift<32;shift+=8) {
        operand(words[i]>>shift);operand(words[34+i]>>shift);
    }
    if(!words[8] && !words[9])throw FormatError("live shadow requires its completed final combiner");
    for(unsigned shift=0;shift<32;shift+=8)operand(words[8]>>shift);
    for(unsigned shift:{8u,16u,24u})operand(words[9]>>shift);
}
}
void SourceLiveShadowPhaseInputsTu15::validate_live(const RendererTargets::Use& use) const {
    attachments(use);
    if(!memory_ || use.bank().generation!=generation_ || use.device_address()!=device_ ||
       effect_backing(*memory_,timed_effect_source_)!=timed_effect_)
        throw FormatError("live shadow phase owner lost its native memory or attachment identity");
    for(const auto& input:retained_)if(!std::equal(input.bytes.begin(),input.bytes.end(),
        memory_->view(input.address,input.bytes.size()).begin()))
        throw FormatError("live shadow phase camera/clip inputs changed before synchronous consumption");
}
std::array<ShaderVector,12> SourceLiveShadowPhaseInputsTu15::camera(const RendererTargets::Use& use)const {
    validate_live(use);std::array<ShaderVector,12> result;
    std::copy_n(phases_[0].constants.begin(),result.size(),result.begin());return result;
}
const SourceShadowScenePhaseInput& SourceLiveShadowPhaseInputsTu15::phase(
    const RendererTargets::Use& use,const Cache& cache,SourceShadowPhase phase,
    const MenuWorldSourcePass& pass,unsigned vertex_class,const SourceShadowMaterial& completed)const {
    validate_live(use);
    if(phase!=SourceShadowPhase::Generate && phase!=SourceShadowPhase::Apply)
        throw FormatError("live shadow phase is outside the original two-phase path");
    const bool apply=phase==SourceShadowPhase::Apply;
    validate_source_shadow_raw_material(completed,phase);
    const auto& m=completed.material;const auto& selected=m.passes.at(m.forward_pass_index);
    if(vertex_class>6 || pass.choice().layer!=(apply?11u:10u) ||
       pass.choice().implementation!=(apply?4u:0u) || pass.shader()!=m.source_shader_datum ||
       pass.choice().shader_lod!=m.selected_shader_lod || selected.template_pass!=pass.template_pass() ||
       selected.implementation!=pass.choice().implementation)
        throw FormatError("live shadow phase lost its actual selected material/pass/class identity");
    //Re-resolve the retained choice against this Cache; a pass from another
    //cache must not import its state ranges or declaration into this owner.
    const auto actual=load_menu_world_source_pass(cache,pass.shader(),pass.choice());
    if(actual.implementation_address()!=pass.implementation_address() ||
       actual.pass_postprocess_address()!=pass.pass_postprocess_address() ||
       actual.vertex_shader()!=pass.vertex_shader() ||
       actual.implementation_record()!=pass.implementation_record())
        throw FormatError("live shadow phase selected pass differs from its immutable cache owner");
    selected_states(cache,actual);completed_program(completed);
    const auto bytes=cache.load_vertex_shader_code(actual.vertex_shader(),vertex_class);
    const auto declaration=cache.load_source_vertex_declaration(actual.vertex_shader(),vertex_class);
    if(declaration.class_index!=vertex_class || declaration.semantics.empty())
        throw FormatError("live shadow selected vertex class lacks its authored declaration");
    const auto& source=menu_world_vertex_metadata_lookup(bytes,declaration);
    for(unsigned reg:source.consumed_inputs)if(reg>=declaration.semantics.size())
        throw FormatError("live shadow selected program requires an unowned constant attribute");
    for(unsigned index:source.consumed_constants)
        if(index>=192 || (index>=12 && index<=17) || (index>=37 && index<=39) ||
           index==49 || index>=173)
            throw FormatError("live shadow selected program consumes unowned constant residue");
    return phases_[apply?1u:0u];
}
SourceLiveShadowPhaseInputsTu15 source_live_shadow_phase_inputs_tu15(
    const SourceRuntimeMemory& memory,const RendererTargets::Use& use,
    const SourceLiveShadowPhaseResidueTu15& residue,std::array<std::uint32_t,2> logical_surface) {
    attachments(use);SourceLiveShadowPhaseInputsTu15 output;
    output.memory_=&memory;output.generation_=use.bank().generation;output.device_=use.device_address();
    if(logical_surface==std::array<std::uint32_t,2>{})
        logical_surface={use.bank().depth_stencil.width,use.bank().depth_stencil.height};
    if(!logical_surface[0] || !logical_surface[1] || logical_surface[0]>32767 || logical_surface[1]>32767)
        throw FormatError("live shadow logical surface exceeds its positive signed-short camera bank");
    const auto retain=[&](std::uint32_t address,std::size_t extent) {
        const auto bytes=memory.view(address,extent);
        output.retained_.push_back({address,{bytes.begin(),bytes.end()}});
    };
    if(read<std::int16_t>(memory,0x491e22u) || read<std::uint8_t>(memory,0x491e27u))
        throw FormatError("live shadow phases require the actual ordinary unscaled submitting view");
    const auto global_viewport=read<std::array<std::int16_t,4>>(memory,0x4c65f0u);
    const auto viewport=read<std::array<std::int16_t,4>>(memory,0x491e68u);
    const auto players=read<std::int32_t>(memory,0x4c686cu);
    //226F0's canonical640x480 check controls the timed split in13660;
    //it does not constrain the ordinary mode0 fallback at13768. The actual
    //zero-effect/override bank below establishes that fallback at any full
    //native surface. Preserve the submitting shorts and camera unchanged.
    if(!same(global_viewport,viewport) || viewport!=std::array<std::int16_t,4>{0,0,
        std::int16_t(logical_surface[1]),std::int16_t(logical_surface[0])} ||
       players<0 || players>1)
        throw FormatError("live shadow phases lack a matching full unsplit source viewport");
    const auto global_eye=read<Vec3>(memory,0x4c65c0u),eye=read<Vec3>(memory,0x491e38u);
    const auto global_planes=read<std::array<float,2>>(memory,0x4c6600u);
    const auto planes=read<std::array<float,2>>(memory,0x491e78u);
    const auto configured=read<std::array<float,2>>(memory,0x4922f4u);
    if(!same(global_eye,eye) || !same(global_planes,planes) || !same(planes,configured))
        throw FormatError("live shadow global/submitting/configured camera aliases differ");
    for(float value:{eye.x,eye.y,eye.z,planes[0],planes[1]})if(!std::isfinite(value))
        throw FormatError("live shadow actual camera fields are not finite");
    if(planes[0]<=0 || planes[1]<=planes[0] || !std::isfinite(1.f/planes[1]))
        throw FormatError("live shadow actual near/far inputs do not admit ordinary fixed-W clipping");
    output.timed_effect_source_=read<std::uint32_t>(memory,0x515ec0u);
    output.timed_effect_=effect_backing(memory,output.timed_effect_source_);
    for(unsigned offset:{0xacu,0x3cu,0x40u,0x190u,0x194u})
        if(read<std::uint32_t>(memory,output.timed_effect_source_+offset))
            throw FormatError("live shadow timed-boundary or near/far override requires its unresolved source owner");
    //E0 is the current requested main mode; E4 is a separate retained
    //offscreen mode.495D0 setsE0=2 AFTER binding10;49690 resetsE0=0 BEFORE
    //Apply. Neither transition rewritesE4. No SDK device is synthesized.
    if(read<std::uint32_t>(memory,0x4117e0u) || read<std::uint32_t>(memory,0x4117e4u))
        throw FormatError("live shadow phases require retained ordinary main/offscreen AA zero");
    for(const auto& vector:residue.constants)for(float value:vector)if(!std::isfinite(value))
        throw FormatError("live shadow explicit unused constant residue is not finite");
    for(float value:residue.fog_parameters)if(!std::isfinite(value))
        throw FormatError("live shadow explicit unused FogParams residue is not finite");
    const auto bank=read<SourceViewProjectionBank>(memory,0x4c6634u);
    std::array<float,48> bank_words;std::memcpy(bank_words.data(),&bank,sizeof(bank));
    for(float value:bank_words)if(!std::isfinite(value))
        throw FormatError("live shadow completed source projection bank is not finite");
    if(!same(bank.camera_to_world.position,eye))
        throw FormatError("live shadow completed projection and submitting eye aliases differ");
    const RenderViewport actual_viewport{float(viewport[1]),float(viewport[0]),
        float(viewport[3]-viewport[1]),float(viewport[2]-viewport[0]),1,false,false,0};
    const auto camera=source_view_shader_bank(bank,planes[1],eye,actual_viewport,false);
    for(const auto& vector:camera)for(float value:vector)if(!std::isfinite(value))
        throw FormatError("live shadow completed c0..11 camera bank is not finite");
    //1345B..134BA ordinary boundary0: ordered float32 DIV/MUL/MUL, then
    //clamp. clipRange.xy comes from actual fixed NVZ24S8 (not MinZ/MaxZ).
    //401950 NULL-depth bind still407940 selects128h target10 format and
    //400470 clearsfloatingZ; it retains CPU depth-scale but not floatingZ.
    const float reciprocal=1.f/configured[1],maximum=16777215.f;
    const float minimum=std::clamp((reciprocal*configured[0])*maximum,0.f,maximum);
    const float upper=std::clamp((reciprocal*configured[1])*maximum,0.f,maximum);
    MenuWorldRasterContext raster;
    //Authored1D818/1D822 target10 dimensions are128; the separate real raw
    //backend checks its actual128 attachment before recording. This owner
    //does not publish an SDK surface header or claim target10 construction.
    raster.viewport_reset({128,128});raster.shadow_begin();
    raster.layer_begin(MenuWorldRasterLayer::shadow_10);raster.entry_begin({false,false,false});
    const auto generation=raster.snapshot({MenuWorldRasterPrimitive::ordinary_triangles,
        MenuWorldRasterPolygon::fill,std::array<std::int32_t,2>{1,1},false});
    raster.entry_end();raster.shadow_end(); //Actual445C0→49690, beforeApply.
    raster.viewport_reset(logical_surface);
    raster.layer_begin(MenuWorldRasterLayer::shadow_apply_11);
    raster.entry_begin({false,false,false});
    const auto application=raster.snapshot({MenuWorldRasterPrimitive::ordinary_triangles,
        MenuWorldRasterPolygon::fill,std::array<std::int32_t,2>{1,1},true});raster.entry_end();
    for(unsigned n=0;n<2;++n) {
        auto& phase=output.phases_[n];phase.constants=residue.constants;
        std::copy(camera.begin(),camera.end(),phase.constants.begin());
        phase.controls={0x900,true,n?std::array<std::uint32_t,4>{0,0,logical_surface[0],logical_surface[1]}:
            std::array<std::uint32_t,4>{0,0,128,128}};
        phase.raster=n?application:generation;phase.clip_range={0,maximum,minimum,upper};
        phase.fog_parameters=residue.fog_parameters;
        phase.surface_size=n?std::array<float,2>{float(logical_surface[0]),float(logical_surface[1])}:
            std::array<float,2>{128,128};
    }
    for(const auto [address,extent]:std::array<std::pair<std::uint32_t,std::size_t>,13>{{
        {0x491e22u,2},{0x491e27u,1},{0x4c65f0u,8},{0x491e68u,8},
        {0x4c65c0u,12},{0x491e38u,12},{0x4c6600u,8},{0x491e78u,8},
        {0x4922f4u,8},{0x4c6634u,192},{0x4c686cu,4},{0x4117e0u,8},{0x515ec0u,4}}})retain(address,extent);
    for(unsigned offset:{0xacu,0x3cu,0x40u,0x190u,0x194u})retain(output.timed_effect_source_+offset,4);
    return output;
}
} // namespace halo2
