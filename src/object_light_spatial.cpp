#include "halo2/object_light_spatial.hpp"
#include "halo2/object_light_contacts.hpp"
#include "halo2/math.hpp"
#include <algorithm>
#include <cmath>
#include <functional>

namespace halo2 {
namespace {
float axis(Vec3 p,unsigned n) {return n==0?p.x:n==1?p.y:p.z;}
constexpr unsigned projection[6][2]{{2,1},{1,2},{0,2},{2,0},{1,0},{0,1}};
}
ObjectLightSpatialBounds source_object_light_spatial_bounds(const ObjectLightSpatialDefinition& def,
    ObjectLightSpatialSphere query,float scale) {
    const auto dynamic=def.dynamic_radius>0?ObjectLightSpatialSphere{
        query.center+def.dynamic_offset,scale*def.dynamic_radius}:query;
    return {dynamic,(def.flags&0x1000)?dynamic:query};
}
bool source_object_light_spatial_bounds_changed(ObjectLightSpatialSphere previous,
    ObjectLightSpatialSphere next,bool registered) {
    if(!registered)return true;
    const auto delta=next.center-previous.center;
    const float squared=(delta.z*delta.z+delta.x*delta.x)+delta.y*delta.y;
    return squared>.010000000707805157f || previous.radius!=next.radius;
}
bool source_object_light_portal_intersects(const ObjectLightSpatialBsp& bank,
    std::uint16_t index,Vec3 point,float radius) {
    if(index>=bank.portals.size())throw FormatError("spatial portal outside authored BSP");
    const auto& portal=bank.portals[index];
    if(portal.plane>=bank.planes.size())throw FormatError("spatial portal plane outside BSP");
    const auto& p=bank.planes[portal.plane];
    //14A370 compares the retained x87 PC53 intermediate, then uses the
    // rounded single-precision stored distance in projection/cross-section.
    // A wider host long double changes cancellation and plane tangencies.
    const double extended=(double(p[1])*point.y+
        double(p[2])*point.z)+double(p[0])*point.x-p[3];
    const float distance=float(extended);
    if(!(std::abs(extended)<radius))return false;
    const float dz=portal.center.z-point.z,dx=portal.center.x-point.x,dy=portal.center.y-point.y;
    const float expanded=portal.radius+radius;
    if(!((dz*dz+dx*dx)+dy*dy<expanded*expanded))return false;
    unsigned dominant;
    if(std::abs(p[2])>=std::abs(p[1]) && std::abs(p[2])>=std::abs(p[0]))dominant=2;
    else if(std::abs(p[1])>=std::abs(p[0]))dominant=1;
    else dominant=0;
    const auto* axes=projection[dominant*2+unsigned(p[dominant]>0)];
    const float negative=0.f-distance;
    const Vec3 projected{p[0]*negative+point.x,p[1]*negative+point.y,p[2]*negative+point.z};
    const Vec2 center{axis(projected,axes[0]),axis(projected,axes[1])};
    const float disk=float(std::sqrt(double(radius)*radius-
        double(distance)*distance));
    const float disk_squared=disk*disk;
    // 23A160 uses the directed authored edges and the projected circle, not
    // just a point-in-polygon test. Empty and degenerate polygons are admitted.
    for(std::size_t i=0;i<portal.vertices.size();++i) {
        const auto a=portal.vertices[i],b=portal.vertices[(i+1)%portal.vertices.size()];
        const float px=center.x-axis(a,axes[0]),py=center.y-axis(a,axes[1]);
        const float dx=axis(b,axes[0])-axis(a,axes[0]),dy=axis(b,axes[1])-axis(a,axes[1]);
        const float length_squared=dx*dx+dy*dy;
        if(length_squared!=0) {
            const float cross=px*dy-py*dx;
            if(cross>0 && cross*cross>length_squared*disk_squared)return false;
        }
    }
    return true;
}
ObjectLightSpatialMembership source_object_light_spatial_membership(
    const ObjectLightSpatialBsp& bank,std::int16_t initial,Vec3 center,float radius,
    std::span<const std::uint32_t> forced,std::uint32_t storage_capacity) {
    ObjectLightSpatialMembership out;
    const auto append=[&](std::int16_t cluster) {
        ++out.total;if(out.clusters.size()<storage_capacity)out.clusters.push_back(cluster);
    };
    if(bank.clusters.size()>32767)throw FormatError("spatial cluster count escapes source signed indices");
    if(!forced.empty()) {
        if(forced.size()<(bank.clusters.size()+31)/32)throw FormatError("forced spatial bitset truncated");
        for(std::size_t i=0;i<bank.clusters.size();++i)
            if(forced[i/32]&(1u<<(i&31)))append(std::int16_t(i));
    } else if(initial!=-1) {
        if(initial<0 || std::size_t(initial)>=bank.clusters.size())throw FormatError("spatial location outside BSP");
        if(radius<=0)append(initial);
        else {
            std::vector<bool> visited(bank.clusters.size());
            std::function<void(std::int16_t)> visit=[&](std::int16_t cluster) {
                append(cluster);visited[cluster]=true;
                for(auto index:bank.clusters[cluster]) {
                    if(index>=bank.portals.size())throw FormatError("cluster spatial portal outside BSP");
                    const auto& portal=bank.portals[index];
                    const auto next=portal.back==cluster?portal.front:portal.back;
                    if(next<0 || std::size_t(next)>=bank.clusters.size())throw FormatError("spatial portal adjacent cluster outside BSP");
                    if(!visited[next] && source_object_light_portal_intersects(bank,index,center,radius))visit(next);
                }
            };
            visit(initial);
        }
    }
    out.truncated=out.total>out.clusters.size();return out;
}
bool source_object_light_segment_intersects(Vec3 origin,Vec3 ray,Vec3 center,float radius) {
    const auto delta=origin-center;
    const float distance=((delta.z*delta.z+delta.y*delta.y)+delta.x*delta.x)-radius*radius;
    if(distance<0)return true;
    const float approach=(ray.z*delta.z+ray.y*delta.y)+ray.x*delta.x;
    if(!(approach<0))return false;
    const float length=(ray.z*ray.z+ray.y*ray.y)+ray.x*ray.x;
    const float discriminant=approach*approach-length*distance;
    if(!(discriminant>0))return false;
    const float beyond=(0.f-length)-approach;
    return beyond<0 || discriminant>beyond*beyond;
}
void ObjectLightSpatialLists::insert(std::uint32_t object,const ObjectLightSpatialPrimitive& primitive,
    std::span<const std::int16_t> memberships) {
    if(object==0xffffffffu)throw FormatError("spatial insertion has NONE identity");
    for(const auto& list:clusters_)if(std::any_of(list.begin(),list.end(),[&](const auto& r){return r.object==object;}))
        throw FormatError("object spatial membership already registered");
    std::vector<bool> seen(clusters_.size());
    for(auto cluster:memberships) {
        if(cluster<0 || std::size_t(cluster)>=clusters_.size())throw FormatError("spatial insertion cluster outside BSP");
        if(seen[cluster])throw FormatError("duplicate object spatial cluster registration");
        seen[cluster]=true;
    }
    for(auto cluster:memberships)clusters_[cluster].insert(clusters_[cluster].begin(),{object,primitive});
}
void ObjectLightSpatialLists::update(std::uint32_t object,const ObjectLightSpatialPrimitive& primitive) {
    for(auto& list:clusters_)for(auto& record:list)if(record.object==object)record.primitive=primitive;
}
void ObjectLightSpatialLists::erase(std::uint32_t object) {
    for(auto& list:clusters_)std::erase_if(list,[&](const auto& record){return record.object==object;});
}
std::span<const ObjectLightSpatialRecord> ObjectLightSpatialLists::cluster(std::int16_t cluster) const {
    if(cluster<0 || std::size_t(cluster)>=clusters_.size())throw FormatError("spatial query cluster outside BSP");
    return clusters_[cluster];
}
bool source_object_light_spatial_scan(const ObjectLightSpatialLists& lists,
    std::span<const std::int16_t> leaf_clusters,std::uint32_t query,
    Vec3 origin,Vec3 delta,std::uint32_t excluded_a,std::uint32_t excluded_b,
    bool contact,const std::function<bool(std::int16_t)>& before,
    const std::function<bool(const ObjectLightSpatialRecord&)>& target) {
    const bool first=(query&0x10000000u)!=0;
    if(!(query&14) || (first && contact))return contact;
    if((query&6) && !before)throw FormatError("source plane/instance contact service missing");
    if((query&8) && !target)throw FormatError("source posed object contact service missing");
    std::vector<bool> visited(lists.size()),objects(65536);
    for(auto cluster:leaf_clusters) {
        if(cluster<0 || std::size_t(cluster)>=lists.size())throw FormatError("source vector leaf cluster outside spatial lists");
        if(visited[cluster])continue;
        visited[cluster]=true;
        if(query&6) {
            if(before(cluster))contact=true;
            if(first && contact)return true;
        }
        if(query&8)for(const auto& record:lists.cluster(cluster)) {
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
ObjectLightSpatialDefinition Cache::load_object_light_spatial_definition(std::uint32_t datum) const {
    const auto object=query_tag({.handle=TagHandle{datum}, .require_address=true});
    if(!object || object->size<0x30)throw FormatError("object spatial definition missing");
    const Bytes data(storage_);const auto p=metadata_offset(object->address,object->size);
    const auto type=std::int16_t(data.u16(p));
    if(type<0 || type>12)throw FormatError("spatial definition is not an object");
    return {data.u16(p+2),data.f32(p+0x20),{data.f32(p+0x24),data.f32(p+0x28),data.f32(p+0x2c)}};
}
ObjectLightSpatialBsp Cache::load_object_light_spatial_bsp(std::size_t index) const {
    if(index>=bsps_.size())throw FormatError("object spatial BSP outside scenario");
    const auto& bsp=bsps_[index];const Bytes data(Bytes(storage_).slice(bsp.file_offset,bsp.size));
    
    const auto root=TagAddressSpace(data, bsp.memory_address).offset(data.u32(4), 0);data.require(root,0xa4);
    
    ObjectLightSpatialBsp out;
    const auto [banks,collision]=TagAddressSpace(data, bsp.memory_address).block(root+0x14, 64);
    if(banks!=1)throw FormatError("object spatial membership requires first structure collision bank");
    const auto [planes,plane]=TagAddressSpace(data, bsp.memory_address).block(collision+8, 16);
    for(std::size_t i=0;i<planes;++i)out.planes.push_back({data.f32(plane+i*16),data.f32(plane+i*16+4),
        data.f32(plane+i*16+8),data.f32(plane+i*16+12)});
    const auto [clusters,cluster]=TagAddressSpace(data, bsp.memory_address).block(root+0x9c, 176);
    for(std::size_t i=0;i<clusters;++i) {
        std::vector<std::uint16_t> refs;const auto [count,p]=TagAddressSpace(data, bsp.memory_address).block(cluster+i*176+0x8c, 2);
        for(std::size_t j=0;j<count;++j)refs.push_back(data.u16(p+j*2));
        out.clusters.push_back(std::move(refs));
    }
    const auto vec=[&](std::size_t p){return Vec3{data.f32(p),data.f32(p+4),data.f32(p+8)};};
    const auto [portals,portal]=TagAddressSpace(data, bsp.memory_address).block(root+0x5c, 36);
    for(std::size_t i=0;i<portals;++i) {
        const auto p=portal+i*36;
        ObjectLightSpatialBsp::Portal decoded{std::int16_t(data.u16(p)),std::int16_t(data.u16(p+2)),
            data.u32(p+4),vec(p+8),data.f32(p+20),{}};
        const auto [vertices,v]=TagAddressSpace(data, bsp.memory_address).block(p+0x1c, 12);
        if(vertices>128)throw FormatError("object spatial portal exceeds original projection scratch");
        for(std::size_t j=0;j<vertices;++j)decoded.vertices.push_back(vec(v+j*12));
        out.portals.push_back(std::move(decoded));
    }
    return out;
}
} // namespace halo2
