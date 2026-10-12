#include "halo2/source_object_spatial_scan.hpp"
#include "halo2/object_light_contacts.hpp"

namespace halo2 {
bool source_object_light_spatial_scan(const SourceObjectSpatialPool& pool,
    std::span<const std::int16_t> leaves,std::uint32_t query,
    Vec3 origin,Vec3 delta,std::uint32_t excluded_a,std::uint32_t excluded_b,
    bool contact,const std::function<bool(std::int16_t)>& before,
    const std::function<bool(const ObjectLightSpatialRecord&)>& target) {
    const bool first=(query&0x10000000u)!=0;
    if(!(query&14) || (first && contact))return contact;
    if((query&6) && !before)throw FormatError("source plane/instance contact service missing");
    if((query&8) && !target)throw FormatError("source posed object contact service missing");
    std::vector<bool> visited(pool.heads.size()),objects(65536);
    for(auto cluster:leaves) {
        if(cluster<0 || std::size_t(cluster)>=pool.heads.size())
            throw FormatError("source vector leaf cluster outside spatial pool");
        if(visited[cluster])continue;
        visited[cluster]=true;
        if(query&6) {
            if(before(cluster))contact=true;
            if(first && contact)return true;
        }
        if(query&8)for(const auto&record:pool.cluster(cluster)) {
            const auto index=record.object&0xffffu;
            if(objects[index])continue;
            objects[index]=true;
            if(!source_object_light_spatial_contact_admitted(query,std::uint8_t(record.primitive.type),
                record.primitive.flags,record.object,excluded_a,excluded_b))continue;
            if(!source_object_light_segment_intersects(origin,delta,record.primitive.center,record.primitive.radius))continue;
            if(target(record))contact=true;
            if(first && contact)return true;
        }
    }
    return contact;
}
}
