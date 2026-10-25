#include "halo2/source_bsp_draw_collector.hpp"
#include <bit>

namespace halo2 {
namespace {
std::uint32_t flags(std::uint16_t queue,const SourceBspDrawContext& context) {
    std::uint32_t result=((queue>>2&0x2c00u)|(queue&0x4000u))>>9;
    result|=(queue&0x200u)<<5;
    if(context.override_byte<0)result|=1;
    const auto& fog=context.fog;
    if(fog.classification==3) {
        if(fog.planar_datum!=0xffffffffu && fog.eye_fraction==0.f &&
            !(queue&0x400u) && !(fog.planar_flags&2u))result|=0x100;
        if(!fog.planar_only || (queue&0x400u))result|=0x80;
    }
    return result;
}
void append(SourceBspDrawState& state,const SourceBspDrawContext& context,
    const SourceBspDrawGeometry& geometry,std::uint32_t classification,
    std::uint32_t unit_flags,unsigned cluster,unsigned instance,
    std::uint16_t association,std::uint8_t alpha,std::uint32_t transform,
    float coverage,const std::function<bool(bool)>& request_resident={}) {
    SourceDrawUnitInput input;input.geometry=cluster;input.instance=instance;
    input.stack={unit_flags,0xffffffffu,0xffffffffu,0,0,association,alpha,
        transform,0xffffffffu,std::bit_cast<std::uint32_t>(coverage),31};
    SourceDrawUnitServices services;
    services.geometry=[&](auto,auto,const auto&){return SourceDrawUnitGeometry{geometry.geometry_class,geometry.mask_subparts};};
    services.lighting=[&](auto,auto,auto){return classification;};
    services.resident=[&](auto,auto selector,auto,const auto&) {
        return request_resident?request_resident((selector&255u)!=0):geometry.resident;
    };
    source_draw_unit_append(state,input,{context.mode,context.mask_arena_address},services);
}
}
void source_bsp_collect_draw_units(SourceBspDrawState& state,SourceBspAdmission& admission,
    const SourceBspDrawWorld& world,const SourceBspDrawContext& context) {
    if(!context.mask_arena_address || state.main_prefix>850 || state.draw_count>850 ||
        world.clusters.size()!=world.bsp.clusters.size() ||
        world.definitions.size()!=world.bsp.definitions.size() ||
        world.instances.size()!=world.bsp.instances.size())
        throw FormatError("source BSP collector current owner bank is incomplete");
    source_draw_units_begin(state,context.mode);
    for(const auto& record:admission.clusters) {
        if(record.id>=world.clusters.size() || record.id>=512)
            throw FormatError("source BSP collector cluster escapes its metadata bank");
        const auto coverage=source_shader_cluster_coverage(world.bsp.clusters[record.id].bounds,context.camera);
        const auto entry=admission.cluster_entries[record.id];
        //133390 uses MOVSX byte, then2CBF0 stores its low16 association.
        const auto association=std::uint16_t(std::int16_t(std::bit_cast<std::int8_t>(entry)));
        append(state,context,world.clusters[record.id],world.lightmap_matches?0:3,
            flags(record.flags,context)|0x40u,record.id,0x7ff,association,255,0,coverage);
    }
    for(auto& record:admission.instances) {
        if(record.id>=world.instances.size() || record.id>=2048)
            throw FormatError("source BSP collector instance escapes its11-bit metadata bank");
        const auto& instance=world.bsp.instances[record.id];
        if(instance.definition<0 || unsigned(instance.definition)>=world.definitions.size())
            throw FormatError("source BSP collector instance definition is unavailable");
        const auto& owner=world.instances[record.id];
        const auto coverage=source_shader_instance_coverage(instance.center,instance.radius,context.camera);
        const auto alpha=source_shader_instance_alpha(coverage);
        if(!alpha){record.flags|=1;continue;}
        const unsigned classification=world.lightmap_matches && owner.lightmap_resident?
            unsigned(owner.lighting_policy!=0):3;
        const auto definition=unsigned(instance.definition);
        const auto resident=world.definition_resident?std::function<bool(bool)>([&](bool request) {
            return world.definition_resident(definition,request);
        }):std::function<bool(bool)>{};
        append(state,context,world.definitions[definition],classification,
            flags(record.flags,context)|0x1008u|((record.flags&0x800u)?1u:0x40u),
            0,record.id,record.association,alpha,owner.transform_address,coverage,resident);
    }
}
}
