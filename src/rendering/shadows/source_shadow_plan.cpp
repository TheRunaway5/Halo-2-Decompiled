#include "halo2/source_shadow_plan.hpp"
#include "halo2/vertex_program.hpp"
#include "halo2/texture_program.hpp"
#include <algorithm>
#include <limits>

namespace halo2 {
namespace {
void world_stream(const Material &material) {
    const auto program=decode_vertex_program(material.vertex_program);
    if(program.relative_constants)
        throw FormatError("shadow world-stream plan cannot bind a relative palette");
    for(unsigned i=0;i<program.count;++i) {
        const auto &t=program.instructions[i];
        const unsigned mac=(t[1]>>21)&15,ilu=(t[1]>>25)&7;
        const bool active[]{mac!=0,mac==2 || (mac>=4 && mac<=12),ilu!=0 || mac==3 || mac==4};
        const unsigned mux[]{(t[2]>>26)&3,(t[2]>>11)&3,(t[3]>>28)&3};
        for(unsigned operand=0;operand<3;++operand)if(active[operand] && mux[operand]==3) {
            const unsigned index=(t[1]>>13)&255;
            if((index>=12 && index<=17) || (index>=37 && index<=39) ||
               (index>=49 && index!=172))
                throw FormatError("shadow plan requires an unrepresented source register/stream");
        }
    }
}
const SourceShadowPlanMaterial &find_material(std::span<const SourceShadowPlanMaterial> bank,
                                            unsigned slot) {
    const auto it=std::find_if(bank.begin(),bank.end(),[&](const auto &m){return m.material_slot==slot;});
    if(it==bank.end() || !it->authored)throw FormatError("shadow plan material slot is not resident");
    return *it;
}
}
void validate_source_shadow_plan_material(const SourceShadowMaterial &authored,
                                          SourceShadowPhase phase) {
    const bool receiver=phase==SourceShadowPhase::Apply;
    const auto &m=authored.material;
    if(!m.forward_pass || m.forward_pass_index>=m.passes.size())
        throw FormatError("shadow draw lacks selected authored pass");
    if(m.forward_vertex!=10)
        throw FormatError("shadow draw does not consume original vertex oD0");
    const auto &pass=m.passes[m.forward_pass_index];
    const std::vector<std::uint16_t> semantics=receiver?
        std::vector<std::uint16_t>{0,4}:std::vector<std::uint16_t>{0,3};
    if(!m.vertex_declaration || m.vertex_declaration->class_index!=0 ||
       m.vertex_declaration->semantics!=semantics || m.vertex_declaration!=pass.vertex_declaration ||
       m.world_vertex_input_transform!=SourceWorldVertexInputTransform::Preserve)
        throw FormatError("shadow draw lacks its original world-class declaration");
    if(!pass.vertex_shader.ends_with(receiver?"\\shadow_buffer_application_lightmap":"\\shadow_buffer_generation"))
        throw FormatError("shadow draw uses a different authored vertex family");
    if(pass.implementation!=(receiver?4u:0u))
        throw FormatError("shadow draw is outside fixed target10 implementation");
    if(receiver) {
        if(!m.forward_raster.depth_enable || m.forward_raster.depth_function!=0x202 || m.forward_raster.depth_write ||
           m.forward_raster.color_write!=0x10101 || m.forward_blend!=9)
            throw FormatError("shadow receiver raster differs from original implementation4");
        if(authored.external_targets[0]!=10)
            throw FormatError("shadow receiver lacks original stage0 external8 target10");
        for(unsigned stage=1;stage<4;++stage)if(authored.external_targets[stage])
            throw FormatError("shadow receiver references an unsupported external stage");
    } else if(m.forward_raster.depth_enable || m.forward_blend ||
              m.forward_raster.color_write!=0x1010101 ||
              std::any_of(authored.external_targets.begin(),authored.external_targets.end(),
                          [](unsigned target){return target!=0;})) {
        throw FormatError("shadow caster raster/targets differ from original implementation0");
    }
}
SourceShadowPlanDraw source_shadow_plan_draw(unsigned slot,
    const SourceShadowMaterial &material,std::span<const Vertex> vertices,
    SourceShadowRasterInput raster) {
    return {slot,vertices,material.material.forward_program,material.material.program_constants,raster};
}
SourceShadowPlanStats validate_source_shadow_plan(const SourceShadowPlan &plan,
    std::span<const SourceShadowPlanMaterial> resident) {
    for(unsigned i=0;i<resident.size();++i) {
        if(!resident[i].authored)throw FormatError("shadow resident slot has no authored material");
        for(unsigned j=0;j<i;++j)if(resident[i].material_slot==resident[j].material_slot)
            throw FormatError("shadow resident material slot is duplicated");
    }
    SourceShadowPlanStats result;
    for(const auto &root:plan.roots) {
        if(root.casters.empty() || root.receivers.empty())continue;
        if(root.uniforms.texture_width!=128 || root.uniforms.texture_height!=128)
            throw FormatError("shadow plan differs from fixed target10 dimensions");
        ++result.roots;
        for(unsigned phase=0;phase<2;++phase) {
            const auto draws=phase?root.receivers:root.casters;
            for(const auto &draw:draws) {
                if(!draw.raster || (draw.raster->cull!=0 && draw.raster->cull!=0x900 && draw.raster->cull!=0x901))
                    throw FormatError("shadow draw lacks actual caller raster controls");
                if(!texture_program_supported(draw.program))
                    throw FormatError("completed shadow draw requires unsupported texture operations");
                const auto &material=find_material(resident,draw.material_slot);
                validate_source_shadow_plan_material(*material.authored,
                    phase?SourceShadowPhase::Apply:SourceShadowPhase::Generate);
                world_stream(material.authored->material);
                if(draw.vertices.empty() || draw.vertices.size()%3 ||
                   draw.vertices.size()>std::numeric_limits<std::uint32_t>::max())
                    throw FormatError("shadow plan has no complete draw triangles");
                for(const auto &vertex:draw.vertices)if(vertex.material!=draw.material_slot)
                    throw FormatError("shadow plan geometry differs from its resident material slot");
                if(result.vertices>std::numeric_limits<std::size_t>::max()-draw.vertices.size())
                    throw FormatError("shadow plan vertex count overflows");
                result.vertices+=draw.vertices.size();
                if(phase)++result.receiver_draws;else ++result.caster_draws;
            }
        }
    }
    return result;
}
SourceShadowPlanBindings source_shadow_plan_bindings(const SourceShadowPlan &plan,
    const SourceShadowPlanRoot &root,const SourceShadowPlanDraw &draw,
    const SourceShadowPlanMaterial &material) {
    if(draw.material_slot!=material.material_slot || !material.authored)
        throw FormatError("shadow draw binding uses a different resident material");
    if(root.uniforms.texture_width!=128 || root.uniforms.texture_height!=128)
        throw FormatError("shadow bindings differ from fixed target10 dimensions");
    world_stream(material.authored->material);
    SourceShadowPlanBindings result{draw.program,draw.constants,plan.camera,{},material.authored->external_targets};
    std::copy_n(root.uniforms.projection.begin(),3,result.constants[1].begin()+13);
    result.globals.first_constant=34;
    result.globals.constants.resize(15);
    std::copy_n(root.uniforms.projection.begin()+3,3,result.globals.constants.begin());
    std::copy(root.uniforms.constants.begin(),root.uniforms.constants.end(),result.globals.constants.begin()+6);
    return result;
}
VertexProgramBank source_shadow_plan_vertex_bank(const SourceShadowPlanBindings &bindings) {
    if(bindings.globals.first_constant!=34 || bindings.globals.constants.size()!=15 ||
       bindings.globals.prt_inputs || bindings.globals.camera_inputs)
        throw FormatError("shadow binding globals differ from fixed source c34..48 bank");
    VertexProgramBank result;
    result.first_constant=0;
    result.constants.resize(49);
    result.camera_inputs=true;
    std::copy(bindings.camera.begin(),bindings.camera.end(),result.constants.begin());
    std::copy(bindings.constants[1].begin(),bindings.constants[1].end(),result.constants.begin()+18);
    std::copy(bindings.globals.constants.begin(),bindings.globals.constants.end(),result.constants.begin()+34);
    return result;
}
}
