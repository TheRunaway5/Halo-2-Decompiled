#include "halo2/source_vector_contact.hpp"
#include "halo2/source_object_posed_target.hpp"
#include "halo2/math.hpp"
#include <bit>
#include <cmath>
#include <limits>

namespace halo2 {
namespace {
std::uint32_t bits(float value){return std::bit_cast<std::uint32_t>(value);}
float scalar(std::uint32_t value){return std::bit_cast<float>(value);}
void short_at(SourceVectorContactPacket& p,unsigned byte,std::int16_t value){
    const unsigned shift=(byte&2)*8;const std::uint32_t mask=65535u<<shift;
    p[byte/4]=(p[byte/4]&~mask)|(std::uint32_t(std::uint16_t(value))<<shift);
}
void plane_at(SourceVectorContactPacket& p,CollisionPlane plane){
    p[10]=bits(plane.normal.x);p[11]=bits(plane.normal.y);
    p[12]=bits(plane.normal.z);p[13]=bits(plane.distance);
}
void surface_at(SourceVectorContactPacket& p,const BspVectorContact& h){
    p[1]=bits(h.fraction);p[19]=h.leaf;p[20]=h.surface;
    p[21]=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(h.oriented_plane)));
    p[22]=h.flags|(std::uint32_t(h.breakable)<<8)|(std::uint32_t(std::uint16_t(h.material))<<16);
}
std::int16_t material(const SourceVectorContactBank& bank,std::int16_t index){
    if(index==-1)return bank.default_material;
    if(index<0 || std::size_t(index)>=bank.global_materials.size())
        throw FormatError("source vector material mapping missing");
    return bank.global_materials[index];
}
}
SourceVectorContactResult source_vector_contact(const SourceVectorContactBank& bank,
    const SourceObjectSpatialPool& pool,std::span<const SourceObjectVectorOwner> owners,
    const SourceObjectVectorInput& supplied,const SourceObjectVectorTarget& target,
    SourceVectorContactPacket caller) {
    if(supplied.flags&0x20000000u)throw FormatError("source vector location repair service missing");
    if(bank.bsp.collision.empty())throw FormatError("source vector main collision bank missing");
    auto in=supplied;
    if(!(in.flags&0x01800000u))in.flags|=0x01800000u;
    in.lower_flags=((in.flags>>23)&31u)|((in.flags&0x10000000u)?32u:0u);
    SourceVectorContactResult out{false,caller,{}};auto& p=out.packet;
    p[0]=0;p[1]=bits(1.f);p[5]=p[7]=p[14]=p[15]=p[16]=0xffffffffu;
    short_at(p,0x18,-1);short_at(p,0x1a,bank.bsp_index);
    short_at(p,0x20,-1);short_at(p,0x22,bank.bsp_index);
    const auto& main_bank=bank.bsp.collision[0];
    const auto main=original_bsp_vector_query(main_bank,in.origin,in.delta,
        std::numeric_limits<float>::max(),in.lower_flags,bank.breakable_bits);
    if(main.contact && (in.flags&1)) {
        const auto& h=*main.contact;const auto index=h.plane;
        if(index>=main_bank.planes.size())throw FormatError("source vector main plane missing");
        const auto& raw=main_bank.planes[index];
        p[0]=1;short_at(p,0x24,material(bank,h.material));surface_at(p,h);
        plane_at(p,source_collision_world_plane(nullptr,{{raw[0],raw[1],raw[2]},raw[3]},bool(h.oriented_plane&32768)));
        out.hit=true;
    }
    std::vector<std::int16_t> clusters;
    for(const auto leaf:main.leaves) {
        if(leaf>=main_bank.leaves.size())throw FormatError("source vector leaf cluster mapping missing");
        clusters.push_back(main_bank.leaves[leaf].cluster);
    }
    if(!main.leaves.empty()) {
        p[5]=main.leaves.front();p[7]=main.leaves.back();
        short_at(p,0x18,clusters.front());short_at(p,0x20,clusters.back());
    }
    if((in.flags&8) && !(in.flags&0x1fff0u))in.flags|=0x1fff0u;
    std::vector<bool> tested_instances(bank.bsp.instances.size());
    const bool first=(in.flags&0x10000000u)!=0;
    const auto before=[&](std::int16_t cluster) {
        bool changed=false;
        if(in.flags&2) {
            if(cluster<0 || std::size_t(cluster)>=bank.cluster_fog.size())
                throw FormatError("source vector cluster fog designator missing");
            const auto designator=bank.cluster_fog[cluster];
            if(designator<-1) {
                const auto index=std::uint8_t(designator)&127u;
                if(index>=bank.fog_planes.size())throw FormatError("source vector fog plane missing");
                const auto& f=bank.fog_planes[index];const auto n=f.plane.normal;
                const float offset=((n.z*in.origin.z+n.y*in.origin.y)+in.origin.x*n.x)-f.plane.distance;
                const float velocity=(n.z*in.delta.z+n.y*in.delta.y)+in.delta.x*n.x;
                if(f.material!=-1 && ((offset>0)!=(velocity>0)) &&
                    std::abs(velocity)>std::abs(offset) && std::abs(velocity)>=std::bit_cast<float>(0x38d1b717u)) {
                    const float fraction=0.f-offset/velocity;
                    if(fraction<scalar(p[1])) {
                        p[0]=2;p[1]=bits(fraction);short_at(p,0x24,f.material);
                        plane_at(p,source_collision_world_plane(nullptr,f.plane,offset<0));
                        p[14]=index;p[15]=p[16]=0xffffffffu;out.hit=changed=true;
                    }
                }
            }
            if(first && out.hit)return changed;
        }
        if(in.flags&4) {
            if(cluster<0 || std::size_t(cluster)>=bank.bsp.cluster_accelerators.size())
                throw FormatError("source vector cluster accelerator missing");
            const auto& accelerator=bank.bsp.cluster_accelerators[cluster];
            if(accelerator)source_instance_accelerator_visit(*accelerator,in.origin,in.delta,scalar(p[1]),
                [&](std::uint32_t key,float limit) {
                    const auto index=std::bit_cast<std::int16_t>(std::uint16_t(key));
                    if(index<0 || std::size_t(index)>=tested_instances.size())
                        throw FormatError("source vector accelerator key outside instance bank");
                    if(tested_instances[index])return limit;
                    tested_instances[index]=true;
                    const auto contact=original_instance_vector_contact(bank.bsp,std::uint16_t(index),in.origin,in.delta,limit,in.lower_flags);
                    if(!contact)return limit;
                    const auto& h=contact->contact;const auto& instance=bank.bsp.instances[index];
                    const auto& cb=bank.bsp.definition_collision.at(instance.definition);
                    const auto plane_index=h.plane;
                    if(plane_index>=cb.planes.size())throw FormatError("source vector instance plane missing");
                    const auto& raw=cb.planes[plane_index];const Transform matrix{instance.scale,instance.axes,instance.translation};
                    p[0]=3;short_at(p,0x24,material(bank,h.material));p[14]=p[16]=0xffffffffu;p[15]=unsigned(index);
                    surface_at(p,h);plane_at(p,source_collision_world_plane(&matrix,{{raw[0],raw[1],raw[2]},raw[3]},bool(h.oriented_plane&32768)));
                    out.hit=changed=true;
                    //244C10 writes an admitted packet even at equal fraction;
                    // only a strict improvement changes the MOPP collector.
                    return first?0.f:(h.fraction<limit?h.fraction:limit);
                });
        }
        return changed;
    };
    const auto dynamic=[&](const ObjectLightSpatialRecord& record) {
        const auto result=source_object_vector_contact(owners,record.object,true,in,scalar(p[1]),target);
        out.targets.insert(out.targets.end(),result.targets.begin(),result.targets.end());
        if(!result.contact)return false;
        const auto& c=*result.contact;const auto& h=c.target;
        p[0]=4;p[1]=bits(h.fraction);short_at(p,0x24,h.global_material);
        p[14]=p[15]=0xffffffffu;p[16]=c.object;
        short_at(p,0x44,h.region);short_at(p,0x46,h.node);p[18]=h.packed_bsp;
        plane_at(p,h.plane);p[19]=std::uint32_t(h.leaf);p[20]=std::uint32_t(h.surface);p[21]=std::uint32_t(h.signed_plane);
        p[22]=h.flags|(std::uint32_t(h.breakable)<<8)|(std::uint32_t(std::uint16_t(h.model_material))<<16);
        out.hit=true;return true;
    };
    out.hit=source_object_light_spatial_scan(pool,clusters,in.flags,in.origin,in.delta,in.excluded_a,in.excluded_b,
        out.hit,before,dynamic);
    const auto point=in.origin+in.delta*scalar(p[1]);p[2]=bits(point.x);p[3]=bits(point.y);p[4]=bits(point.z);
    return out;
}
}
