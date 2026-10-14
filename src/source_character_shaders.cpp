#include "halo2/source_character_shaders.hpp"
#include "halo2/appearance.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/simulation.hpp"

namespace halo2 {
SourceCharacterShaderInputs::SourceCharacterShaderInputs(const Cache& cache,std::uint32_t unit_definition)
  :bounds_(cache.load_object_shadow_definition(unit_definition)),
   functions_(cache.load_object_functions(unit_definition)),tables_(cache.shader_function_tables()) {
    const auto unit=cache.load_source_tag_metadata(unit_definition);
    // Character tags also reference the Sentinel Enforcer's vehicle body.
    // Its ordinary model passes use the same authored material owner; AI and
    // vehicle physics remain independent of this residency service.
    if(unit.group()!="bipd" && unit.group()!="crea" && unit.group()!="vehi")
        throw FormatError("Character shader owner requires an authored unit or creature");
    const auto root=Bytes(unit.bytes_at(unit.source_address(),0x3c));
    lighting_.object_type=root.u16(0);
    lighting_.definition_flags=root.u16(2);
    const auto expected=unit.group()=="bipd"?0:unit.group()=="vehi"?1:12;
    if(lighting_.object_type!=expected)
        throw FormatError("Character object type differs from its source tag group");
    if(lighting_.object_type==1)
        lighting_.vehicle_type=std::to_integer<std::uint8_t>(unit.bytes_at(unit.source_address()+0x1f0,1).front());
    if(lighting_.object_type==12)
        lighting_.creature_flags=Bytes(unit.bytes_at(unit.source_address()+0xd4,4)).u32(0);
}
SourceCharacterShaders::SourceCharacterShaders(const Cache& cache,RenderModel& model,
    std::uint32_t unit_definition):SourceCharacterShaderInputs(cache,unit_definition) {
    const auto unit=cache.load_source_tag_metadata(unit_definition);
    const auto hlmt=Bytes(unit.bytes_at(unit.source_address()+0x38,4)).u32(0);
    const auto definition=cache.load_source_tag_metadata(hlmt);
    if(definition.group()!="hlmt")throw FormatError("Character shader owner lost its model definition");
    const auto mode=Bytes(definition.bytes_at(definition.source_address(),8)).u32(4);
    for(auto& material:model.mesh.materials)if(!material.lods.empty()) {
        const auto colors=material.object_render_colors;
        // Bind resident images before Vulkan creation. Zero here is only an
        // initial uniform bank; every live actor supplies its own draw light.
        if(!prepare_dynamic_material(cache,material,{},true,0.f))
            throw FormatError("Character shader requires unresolved authored inputs: "+material.name);
        if(colors)apply_object_render_colors(material,*colors);
    }
    resident_=std::make_unique<SourceDynamicModelShaders>(cache,model,mode,2,0.f);
}
ShadowCasterSphere SourceCharacterShaderInputs::sphere(const Transform& node0,float scale)const {
    return source_shadow_sphere(bounds_,node0,scale);
}
void SourceCharacterShaders::select(std::vector<Vertex>& vertices,ShadowCasterSphere sphere,
    const SourceModelShaderView& view)const {
    resident_->select(vertices,source_model_shader_collector(sphere.center,sphere.radius,view),view);
}
void SourceCharacterShaders::select(std::vector<Vertex>& vertices,const SourceModelShaderCollector& collector,
    const SourceModelShaderView& view)const {
    resident_->select(vertices,collector,view);
}
ShaderInput SourceCharacterShaderInputs::input(const Unit& unit,float seconds)const {
    return [this,&unit,seconds](std::uint32_t name) {
        return damage_feedback_shader_input(unit.damage_feedback,unit.vitality.body,unit.vitality.shield,
            unit.vitality.dead,functions_,*tables_,name,seconds);
    };
}
ObjectLightQueryContext SourceCharacterShaderInputs::light_query_context(const Unit& unit)const {
    auto context=lighting_;
    if(context.object_type==0)
        context.biped_state=source_biped_light_query_initial_state(unit.definition.physics_flags,unit.source_unit_10a);
    return context;
}
}
