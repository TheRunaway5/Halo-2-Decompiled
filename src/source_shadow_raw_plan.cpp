#include "halo2/source_shadow_raw_plan.hpp"
#include <algorithm>
#include <cstring>
#include <limits>

namespace halo2 {
bool source_shadow_raw_phases_present(const std::array<std::uint32_t,2>& masks) {
    return (masks[0]&(1u<<10)) && (masks[1]&(1u<<11));
}
namespace {
bool same_texture(const Texture& a,const Texture& b) {
    if(a.width!=b.width || a.height!=b.height || a.depth!=b.depth || a.layers!=b.layers ||
       a.format!=b.format || a.pixels!=b.pixels || a.mips.size()!=b.mips.size())return false;
    for(std::size_t n=0;n<a.mips.size();++n) {
        const auto& x=a.mips[n];const auto& y=b.mips[n];
        if(x.width!=y.width || x.height!=y.height || x.depth!=y.depth ||
           x.offset!=y.offset || x.size!=y.size)return false;
    }
    return true;
}
const SourceShadowRawPlanMaterial& resident_material(
    std::span<const SourceShadowRawPlanMaterial> resident,unsigned slot) {
    const auto found=std::find_if(resident.begin(),resident.end(),
        [&](const auto& value){return value.material_slot==slot;});
    if(found==resident.end() || !found->authored)
        throw FormatError("raw shadow material slot is not resident");
    return *found;
}
void same_pass(const SourceShadowMaterial& a,const SourceShadowMaterial& b) {
    const auto& x=a.material;const auto& y=b.material;
    if(!x.forward_pass || !y.forward_pass || x.forward_pass_index>=x.passes.size() ||
       y.forward_pass_index>=y.passes.size() || x.source_shader_datum!=y.source_shader_datum ||
       x.selected_shader_lod!=y.selected_shader_lod || x.forward_pass_index!=y.forward_pass_index ||
       x.forward_vertex!=y.forward_vertex || x.forward_raster!=y.forward_raster ||
       x.forward_blend!=y.forward_blend || x.texture_states!=y.texture_states ||
       a.external_targets!=b.external_targets || x.vertex_program!=y.vertex_program ||
       x.vertex_declaration!=y.vertex_declaration ||
       x.world_vertex_input_transform!=y.world_vertex_input_transform)
        throw FormatError("raw shadow completed pass differs from its resident slot");
    const auto& p=x.passes[x.forward_pass_index];const auto& q=y.passes[y.forward_pass_index];
    if(p.template_pass!=q.template_pass || p.implementation!=q.implementation ||
       p.vertex_shader!=q.vertex_shader)
        throw FormatError("raw shadow selected template pass differs from residency");
    const Texture* actual[]{&x.diffuse,&x.detail,&x.bump,&x.auxiliary};
    const Texture* expected[]{&y.diffuse,&y.detail,&y.bump,&y.auxiliary};
    for(unsigned stage=0;stage<4;++stage) {
        if(a.external_targets[stage])continue;
        if(!same_texture(*actual[stage],*expected[stage]))
            throw FormatError("raw shadow completed image differs from its resident bitmap/mip bank");
        const auto& frames=x.animated_textures[stage];const auto& resident_frames=y.animated_textures[stage];
        if(frames.size()!=resident_frames.size() || !std::equal(frames.begin(),frames.end(),
            resident_frames.begin(),same_texture))
            throw FormatError("raw shadow animated image bank differs from residency");
    }
}
bool same_command(const SourceShadowRawCommandTu15& a,const SourceShadowRawCommandTu15& b) {
    return a.phase==b.phase && a.root_owner==b.root_owner && a.bank==b.bank &&
        a.category==b.category && a.layer==b.layer && a.sorted_ordinal==b.sorted_ordinal &&
        a.unit_ordinal==b.unit_ordinal && a.descriptor==b.descriptor &&
        a.command==b.command && a.unit==b.unit && a.geometry_owner==b.geometry_owner;
}
}
SourceShadowRawPlanStats validate_source_shadow_raw_plan(const SourceShadowRawPlan& plan,
    std::span<const SourceShadowRawPlanMaterial> resident) {
    for(std::size_t n=0;n<resident.size();++n) {
        if(!resident[n].authored)throw FormatError("raw shadow residency has no authored pass");
        for(std::size_t k=0;k<n;++k)if(resident[k].material_slot==resident[n].material_slot)
            throw FormatError("raw shadow resident material slot is duplicated");
    }
    SourceShadowRawPlanStats result;
    for(const auto& root:plan.roots) {
        if(!source_shadow_raw_phases_present(root.command_masks)) {
            if(!root.casters.empty() || !root.receivers.empty())
                throw FormatError("raw shadow draws precede the original two-phase command guard");
            continue;
        }
        if(root.root_owner==0xffffffffu || root.uniforms.texture_width!=128 ||
           root.uniforms.texture_height!=128)
            throw FormatError("raw shadow root lacks its actual owner/fixed target extent");
        ++result.roots;
        for(unsigned phase=0;phase<2;++phase) {
            const auto draws=phase?root.receivers:root.casters;
            const SourceShadowRawCommandTu15* previous=nullptr;
            for(const auto& draw:draws) {
                validate_source_shadow_raw_draw(draw,phase?SourceShadowPhase::Apply:
                    SourceShadowPhase::Generate,root.uniforms);
                if(draw.command.root_owner!=root.root_owner ||
                   std::memcmp(plan.camera.data(),draw.source.constants().data(),sizeof(plan.camera)))
                    throw FormatError("raw shadow draw differs from its root/main camera bank");
                if(previous && (draw.command.sorted_ordinal<previous->sorted_ordinal ||
                   (draw.command.sorted_ordinal==previous->sorted_ordinal &&
                    !same_command(*previous,draw.command))))
                    throw FormatError("raw shadow ranges lost their original sorted descriptor order");
                previous=&draw.command;
                same_pass(*draw.authored,*resident_material(resident,draw.material_slot).authored);
                const auto count=draw.source.draw().index_range().count;
                if(count>std::numeric_limits<std::size_t>::max()-result.indices)
                    throw FormatError("raw shadow index count overflows");
                result.indices+=count;
                if(phase)++result.receiver_draws;else ++result.caster_draws;
            }
        }
    }
    return result;
}
}
