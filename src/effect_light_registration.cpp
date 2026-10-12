#include "halo2/effect_light_registration.hpp"

namespace halo2 {
namespace {
void register_light(const EffectLightDefinition& definition,
    SourceEffectLightRegistration& light,const BspAtmosphere& location_bank,
    std::int16_t selected_bsp,const ObjectLightSpatialBsp& membership_bank,
    SourceEffectLightSpatialPool& pool,const SourceSphereContactBank* contacts,const RawCollisionBsp* terrain) {
    if(!(light.flags&2))return;
    if(light.flags&4)throw FormatError("Effect light registration requires original segment-bound refresh");
    if(light.identity==0xffffffffu || membership_bank.clusters.size()!=pool.heads.size())
        throw FormatError("Effect light registration requires actual identity and matching BSP heads");
    const Bytes bytes(definition.bytes);
    if(bytes.u16(4)!=0)throw FormatError("Effect light registration requires original frustum shape");
    float radius=definition.radius;
    if(!(definition.flags&2) && radius<definition.specular_radius)radius=definition.specular_radius;
    radius*=bytes.f32(12);
    if(radius<=bytes.f32(0x98))radius=bytes.f32(0x98);
    if(light.extension_radius<=0) {light.spatial_center=light.position;light.spatial_radius=radius;}
    else {light.spatial_center=light.extension_center;light.spatial_radius=light.extension_radius+radius;}
    light.cull_center=light.location_center=light.spatial_center;light.cull_radius=light.spatial_radius;
    if(!(light.flags&8) && light.location.cluster==-1 && (definition.flags&0x8000) && light.spatial_radius>0) {
        SourceSphereContactStorage storage;SourceSphereContactBank native_contacts;
        if(terrain) {storage=source_light_spawn_contacts(*terrain,light.spatial_center,light.spatial_radius);native_contacts=storage.view();contacts=&native_contacts;}
        if(!contacts)throw FormatError("Initial effect light registration requires actual16A440 contact bank");
        light.spatial_center=source_sphere_position(light.spatial_center,light.spatial_radius,*contacts).position;
        light.location_center=light.spatial_center;
    }
    light.location=source_object_point_location(location_bank,selected_bsp,light.location_center);
    if(light.location.cluster==-1 && (!(light.flags&1) || (definition.flags&8)))
        light.location=source_object_point_location(location_bank,selected_bsp,light.spatial_center);
    const auto membership=source_object_light_spatial_membership(membership_bank,light.location.cluster,
        light.spatial_center,light.spatial_radius);
    pool.insert(light.identity,light.reference_head,membership.clusters);
    light.flags|=8;
}
}
void source_effect_light_register(const EffectLightDefinition& definition,
    SourceEffectLightRegistration& light,const BspAtmosphere& locations,std::int16_t selected,
    const ObjectLightSpatialBsp& memberships,SourceEffectLightSpatialPool& pool,const RawCollisionBsp& terrain) {
    register_light(definition,light,locations,selected,memberships,pool,nullptr,&terrain);
}
void source_effect_light_register_from_contacts(const EffectLightDefinition& definition,
    SourceEffectLightRegistration& light,const BspAtmosphere& locations,std::int16_t selected,
    const ObjectLightSpatialBsp& memberships,SourceEffectLightSpatialPool& pool,const SourceSphereContactBank* contacts) {
    register_light(definition,light,locations,selected,memberships,pool,contacts,nullptr);
}
void source_effect_light_unregister(SourceEffectLightRegistration& light,SourceEffectLightSpatialPool& pool) {
    if(light.flags&2) {pool.erase(light.identity,light.reference_head);light.flags&=std::uint16_t(~8u);}
}
}
