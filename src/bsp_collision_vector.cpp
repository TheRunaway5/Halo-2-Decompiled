#include "halo2/bsp_surface_lighting.hpp"
#include "halo2/math.hpp"
#include "halo2/transform.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <functional>
#include <limits>
namespace halo2 {
namespace {
float component(Vec3 v,unsigned axis) {return axis==0?v.x:axis==1?v.y:v.z;}
constexpr unsigned projection[6][2]{{2,1},{1,2},{0,2},{2,0},{1,0},{0,1}};
}
BspVectorResult original_bsp_vector_query(const BspSurfaceLighting::CollisionBank& bank,Vec3 origin,
    Vec3 ray,float maximum,std::uint32_t flags,std::span<const std::uint32_t> broken) {
    BspVectorResult result;
    if(bank.nodes.empty())return result;
    int previous=-1,previous_class=0,crossed_plane=-1;
    const float initial=maximum<0?0:maximum;
    float closest=initial;
    const auto surface_at=[&](unsigned leaf,unsigned plane,float t,bool double_sided)->int {
        const auto& l=bank.collision_leaves.at(leaf);
        for(unsigned i=l.first;i<unsigned(l.first)+l.count;++i) {
            const auto& ref=bank.references2.at(i);
            if((ref.plane&0x7fff)!=plane)continue;
            const auto& p=bank.planes.at(plane);
            unsigned dominant;
            if(std::abs(p[2])>=std::abs(p[1]) && std::abs(p[2])>=std::abs(p[0]))dominant=2;
            else if(std::abs(p[1])>=std::abs(p[0]))dominant=1;
            else dominant=0;
            const bool positive=(p[dominant]>0)!=bool(ref.plane&0x8000);
            const auto* axes=projection[dominant*2+unsigned(positive)];
            const auto hit=origin+ray*t;
            const Vec2 projected{component(hit,axes[0]),component(hit,axes[1])};
            auto node=ref.node;
            std::size_t traversed=0;
            while(!(node&0x8000)) {
                if(++traversed>bank.nodes2.size())throw FormatError("source BSP2D vector traversal cycle");
                const auto& n=bank.nodes2.at(node&0x7fff);
                const float distance=(n.normal.y*projected.y+n.normal.x*projected.x)-n.distance;
                node=n.children[unsigned(distance>=0)];
            }
            if(node==0xffff)continue;
            const auto surface=unsigned(node&0x7fff);
            const auto& s=bank.surfaces.at(surface);
            if(double_sided) {
                if(!broken.empty() && (s.flags&8) && unsigned(s.breakable)<broken.size()*32 &&
                    !(broken[s.breakable>>5]&(1u<<(s.breakable&31))))continue;
                auto edge=s.edge;bool inside=true;
                std::size_t edge_count=0;
                do {
                    if(++edge_count>bank.edges.size())throw FormatError("source collision polygon edge cycle");
                    const auto& e=bank.edges.at(edge);
                    const auto right=unsigned(e.surface[1]==int(surface));
                    const auto a=bank.vertices.at(e.vertex[right]),b=bank.vertices.at(e.vertex[1-right]);
                    const float ax=component(a,axes[0]),ay=component(a,axes[1]);
                    const float px=projected.x-ax,py=projected.y-ay;
                    const float dx=component(b,axes[0])-ax,dy=component(b,axes[1])-ay;
                    if(dy*px-py*dx>0){inside=false;break;}
                    edge=e.next[right];
                } while(edge!=s.edge);
                if(!inside)continue;
            }
            return int(surface);
        }
        return -1;
    };
    std::size_t calls=0;
    std::function<bool(std::uint32_t,float,float,unsigned)> walk;
    walk=[&](std::uint32_t node,float begin,float end,unsigned depth)->bool {
        if(depth>1024 || ++calls>bank.nodes.size()*4+4)throw FormatError("source BSP vector traversal cycle");
        while(!(node&0x800000)) {
            const auto& n=bank.nodes.at(node&0x7fffff);
            const auto& p=bank.planes.at(n.plane);
            const float offset=((p[2]*origin.z+p[1]*origin.y)+p[0]*origin.x)-p[3];
            const float velocity=(ray.z*p[2]+ray.y*p[1])+ray.x*p[0];
            const bool start_positive=velocity*begin+offset>=0;
            const bool end_positive=velocity*end+offset>=0;
            if(start_positive!=end_positive) {
                const bool going_positive=velocity>0;
                const float crossing=-(offset/velocity);
                if(walk(going_positive?n.negative:n.positive,begin,crossing,depth+1))return true;
                if(crossing>=closest)return false;
                crossed_plane=n.plane;begin=crossing;
                node=going_positive?n.positive:n.negative;
            } else node=start_positive?n.positive:n.negative;
            if(++calls>bank.nodes.size()*4+4)throw FormatError("source BSP vector traversal cycle");
        }
        const int leaf=node==0xffffff?-1:int(node&0x7fffff);
        const int classification=leaf<0?3:(bank.collision_leaves.at(leaf).flags&1)?2:1;
        int tested_leaf=-1;bool double_sided=false;
        if((flags&1) && (previous_class==1 || previous_class==2) && classification==3)tested_leaf=previous;
        else if((flags&2) && previous_class==3 && (classification==1 || classification==2))tested_leaf=leaf;
        else if(!(flags&4) && previous_class==2 && classification==2) {
            tested_leaf=(flags&1)?previous:leaf;double_sided=true;
        }
        if(tested_leaf>=0 && crossed_plane>=0) {
            const auto surface=surface_at(unsigned(tested_leaf),unsigned(crossed_plane),begin,double_sided);
            if(surface>=0) {
                const auto& s=bank.surfaces.at(surface);
                if(!((s.flags&2)&&(flags&8)) && !((s.flags&8)&&(flags&16))) {
                    closest=begin;
                    result.contact=BspVectorContact{begin,unsigned(tested_leaf),unsigned(surface),unsigned(crossed_plane),s.plane,s.flags,s.breakable,s.material};
                    return true;
                }
            }
        }
        if(leaf>=0) {
            if(result.leaves.size()<256)result.leaves.push_back(unsigned(leaf));
            else result.leaves.back()=unsigned(leaf);
        }
        previous=leaf;previous_class=classification;return false;
    };
    walk(0,0,std::clamp(initial,0.f,1.f),0);
    return result;
}
std::optional<MapVectorContact> original_instance_vector_contact(const BspSurfaceLighting& bank,std::uint16_t index,
    Vec3 origin,Vec3 ray,float maximum,std::uint32_t test_flags) {
    if(index>=bank.instances.size())throw FormatError("source instance outside placement bank");
    const auto& instance=bank.instances[index];
    if(instance.definition>=bank.definition_collision.size() || instance.scale==0)return {};
    const auto& collision=bank.definition_collision[instance.definition];
    // 1689B0 rejects definitions marked by an invalid final surface.
    if(collision.surfaces.empty() || (collision.surfaces.back().flags&16))return {};
    // 11E5E0's segment/sphere rejection, in original scalar operation order.
    const auto delta=origin-instance.collision_center;
    const float distance=((delta.z*delta.z+delta.y*delta.y)+delta.x*delta.x)-instance.collision_radius*instance.collision_radius;
    if(!(distance<0)) {
        const float approach=(ray.z*delta.z+ray.y*delta.y)+ray.x*delta.x;
        if(!(approach<0))return {};
        const float length=(ray.z*ray.z+ray.y*ray.y)+ray.x*ray.x;
        const float discriminant=approach*approach-length*distance;
        if(!(discriminant>0))return {};
        const float beyond=(0.f-length)-approach;
        if(!(beyond<0) && !(discriminant>beyond*beyond))return {};
    }
    const auto inverse=inverse_transform({instance.scale,instance.axes,instance.translation});
    const auto rotate=[&](Vec3 p) {
        if(inverse.scale!=1)p=p*inverse.scale;
        return Vec3{(inverse.basis[2].x*p.z+inverse.basis[1].x*p.y)+inverse.basis[0].x*p.x,
            (inverse.basis[2].y*p.z+inverse.basis[1].y*p.y)+inverse.basis[0].y*p.x,
            (inverse.basis[2].z*p.z+inverse.basis[1].z*p.y)+inverse.basis[0].z*p.x};
    };
    const auto local_origin=rotate(origin)+inverse.position,local_ray=rotate(ray);
    const auto query=original_bsp_vector_query(collision,local_origin,local_ray,maximum,test_flags);
    if(!query.contact)return {};
    const auto& h=*query.contact;
    return MapVectorContact{{bank.bsp,int(instance.definition),int(h.surface),int(index),h.oriented_plane},origin+ray*h.fraction,h};
}
std::optional<MapVectorContact> original_map_vector_contact(const BspSurfaceLighting& bank,Vec3 origin,Vec3 ray,
    bool* unresolved_order,std::uint32_t source_flags,const MapInstanceCallbackObserver& observer) {
    // 168AE0's conversion. These are the three 14AF40 map paths; dynamic
    // object admission remains a separate source service and is not inferred.
    if(source_flags!=0x4480000d && source_flags!=0x04800c0d && source_flags!=0x04800005)
        throw FormatError("unported source map collision query flags");
    const std::uint32_t test_flags=(source_flags>>23)&63;
    if(unresolved_order)*unresolved_order=false;
    if(bank.collision.empty())return {};
    if(bank.cluster_accelerators.size()!=bank.clusters.size())
        throw FormatError("source cluster accelerator bank incomplete");
    const auto main=original_bsp_vector_query(bank.collision[0],origin,ray,std::numeric_limits<float>::max(),test_flags);
    std::optional<MapVectorContact> best;
    if(main.contact) {
        const auto& h=*main.contact;
        best=MapVectorContact{{bank.bsp,0,int(h.surface),-1,h.oriented_plane},origin+ray*h.fraction,h};
    }
    std::vector<bool> tested_instances(bank.instances.size()),tested_clusters(bank.clusters.size());
    for(const auto leaf:main.leaves) {
        if(leaf>=bank.collision[0].leaves.size())throw FormatError("source vector leaf has no render cluster");
        const auto cluster=bank.collision[0].leaves[leaf].cluster;
        if(cluster<0 || std::size_t(cluster)>=bank.cluster_accelerators.size())continue;
        if(tested_clusters[cluster])continue;
        tested_clusters[cluster]=true;
        const auto& accelerator=bank.cluster_accelerators[cluster];
        if(!accelerator)continue; // Literal244980 cluster+A8 zero gate.
        source_instance_accelerator_visit(*accelerator,origin,ray,best?best->contact.fraction:1.f,
            [&](std::uint32_t key,float fraction) {
            if(observer)observer({std::uint32_t(cluster),key,fraction,
                best?(best->source.instance<0?1u:3u):0u,
                best?std::uint32_t(best->source.instance):0xffffffffu});
            const auto signed_index=std::bit_cast<std::int16_t>(std::uint16_t(key));
            if(signed_index<0)throw FormatError("source accelerator instance key is negative");
            const auto index=unsigned(signed_index);
            if(index>=bank.instances.size())throw FormatError("source cluster instance outside placement bank");
            if(tested_instances[index])return fraction;
            tested_instances[index]=true;
            const auto query=original_instance_vector_contact(bank,std::uint16_t(index),origin,ray,fraction,test_flags);
            if(query)best=query;
            return best?best->contact.fraction:1.f;
        });
    }
    return best;
}
} // namespace halo2
