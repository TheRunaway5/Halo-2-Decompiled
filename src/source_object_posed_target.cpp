#include "halo2/source_object_posed_target.hpp"
#include "halo2/bsp_surface_lighting.hpp"
#include "halo2/math.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <limits>

namespace halo2 {
namespace {
BspSurfaceLighting::CollisionBank source_bank(const RawCollisionBsp& raw) {
    BspSurfaceLighting::CollisionBank out;
    for(const auto& n:raw.nodes)out.nodes.push_back({std::uint16_t(n.plane),
        std::uint32_t(n.children[0])&0xffffffu,std::uint32_t(n.children[1])&0xffffffu});
    for(const auto& p:raw.planes)out.planes.push_back({p.normal.x,p.normal.y,p.normal.z,p.distance});
    for(const auto& l:raw.leaves)out.collision_leaves.push_back({l.flags,l.count,l.first});
    for(const auto& r:raw.references)out.references2.push_back({std::uint16_t(r.plane),std::uint16_t(r.node)});
    for(const auto& n:raw.nodes2)out.nodes2.push_back({n.normal,n.distance,
        {std::uint16_t(n.children[0]),std::uint16_t(n.children[1])}});
    for(const auto& s:raw.surfaces)out.surfaces.push_back({std::uint16_t(s.plane),s.first_edge,s.flags,s.breakable,s.material});
    for(const auto& e:raw.edges)out.edges.push_back({e.vertices,e.next,
        {std::bit_cast<std::int16_t>(e.surfaces[0]),std::bit_cast<std::int16_t>(e.surfaces[1])}});
    out.vertices=raw.vertices;return out;
}
Vec3 local_vector(const Transform& inverse,Vec3 p) {
    if(inverse.scale!=1.f)p=p*inverse.scale;
    const auto& b=inverse.basis;
    //1EFF40 has a distinct X accumulation order; preserve every scalar add.
    return {(b[1].x*p.y+b[0].x*p.x)+b[2].x*p.z,
        (b[2].y*p.z+b[1].y*p.y)+b[0].y*p.x,
        (b[2].z*p.z+b[1].z*p.y)+b[0].z*p.x};
}
}
CollisionPlane source_collision_world_plane(const Transform* matrix,CollisionPlane p,bool reversed) {
    if(!matrix) {
        if(reversed)return {{0.f-p.normal.x,0.f-p.normal.y,0.f-p.normal.z},0.f-p.distance};
        return p;
    }
    const auto& m=*matrix;
    const auto& b=m.basis;const auto n=p.normal;
    Vec3 normal{(b[2].x*n.z+b[1].x*n.y)+b[0].x*n.x,
        (b[2].y*n.z+b[1].y*n.y)+b[0].y*n.x,
        (b[2].z*n.z+b[1].z*n.y)+b[0].z*n.x};
    float distance=((m.position.z*normal.z+m.scale*p.distance)+m.position.y*normal.y)+m.position.x*normal.x;
    if(reversed){normal={0.f-normal.x,0.f-normal.y,0.f-normal.z};distance=0.f-distance;}
    return {normal,distance};
}
std::optional<PosedCollisionHit> source_object_posed_target(const ModelCollision& model,
    std::span<const Transform> nodes,std::optional<std::span<const std::uint8_t>> active,
    Vec3 origin,Vec3 delta,std::uint32_t lower_flags) {
    if(model.regions.size()>16)throw FormatError("source collision iterator exceeds16 selections");
    std::array<std::int8_t,16> selections;selections.fill(active?-1:0);
    if(active) {
        const auto count=std::min<std::size_t>(model.model_regions.size(),16);
        if(active->size()<count)throw FormatError("source active collision permutation bank incomplete");
        for(std::size_t i=0;i<count;++i) {
            const auto& r=model.model_regions[i];const auto p=(*active)[i];
            if(r.collision==255 || p>=128 || p>=r.permutations.size())continue;
            if(r.collision>=16)throw FormatError("source collision selection index outside bank");
            const auto selection=r.permutations[p];
            if(selection!=255)selections[r.collision]=std::bit_cast<std::int8_t>(selection);
        }
    }
    std::optional<PosedCollisionHit> best;
    float fraction=std::numeric_limits<float>::max();
    for(std::size_t region=0;region<model.regions.size();++region) {
        const int permutation=selections[region];
        if(permutation<0 || std::size_t(permutation)>=model.regions[region].size())continue;
        const auto& bsps=model.regions[region][permutation];
        if(bsps.size()>256)throw FormatError("source collision BSP index exceeds byte");
        for(std::size_t i=0;i<bsps.size();++i) {
            const auto& raw=bsps[i];
            //1EFCA0 packs the item's first byte;1EFF40 uses its low8 node.
            const auto node=std::uint8_t(raw.node);
            if(node>=nodes.size())throw FormatError("source current collision WORLD node missing");
            const auto& matrix=nodes[node];const auto inverse=inverse_transform(matrix);
            const auto local_origin=local_vector(inverse,origin)+inverse.position;
            const auto local_delta=local_vector(inverse,delta);
            const auto query=original_bsp_vector_query(source_bank(raw),local_origin,local_delta,fraction,lower_flags);
            if(!query.contact)continue;
            const auto& h=*query.contact;fraction=h.fraction;
            //1DE6D0 retains the crossed geometric plane pointer. The surface
            //plane contributes only orientation to168B40's transformation.
            const auto plane_index=h.plane;
            if(plane_index>=raw.planes.size())throw FormatError("source collision surface plane missing");
            if(h.material<0 || std::size_t(h.material)>=model.materials.size())
                throw FormatError("source collision material mapping missing");
            best=PosedCollisionHit{h.fraction,
                source_collision_world_plane(&matrix,raw.planes[plane_index],bool(h.oriented_plane&0x8000)),
                std::uint32_t((region<<24)|(std::size_t(permutation)<<16)|(i<<8)|node),
                std::int32_t(h.leaf),std::int32_t(h.surface),std::int16_t(h.oriented_plane),
                std::int16_t(region),std::int16_t(node),h.material,model.materials[h.material].global_material,
                h.flags,h.breakable};
        }
    }
    return best;
}
}
