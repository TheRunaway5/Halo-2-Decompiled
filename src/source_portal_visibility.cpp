#include "halo2/source_portal_visibility.hpp"
#include <algorithm>
#include <deque>
#include <limits>
#include <numeric>

namespace halo2 {
namespace {
constexpr std::size_t arena_capacity=0x1400;
constexpr float largest=std::numeric_limits<float>::max();
ShaderVector rectangle(std::span<const Vec2> polygon) {
    ShaderVector out{largest,-largest,largest,-largest};
    for(const auto p:polygon) {
        if(p.x<out[0])out[0]=p.x;
        if(p.x>out[1])out[1]=p.x;
        if(p.y<out[2])out[2]=p.y;
        if(p.y>out[3])out[3]=p.y;
    }
    return out;
}
struct Node {
    unsigned group{};int parent=-1,portal=-1,cluster=-1,merged=-1;
    ShaderVector rectangle{};std::vector<Vec2> polygon;
    float metric{};std::array<std::uint32_t,16> visited{};
};
void validate(const SourcePortalBsp &bsp) {
    if(bsp.cluster_portals.size()>512 || bsp.portals.size()>512)
        throw FormatError("source portal BSP exceeds512 entries");
    for(const auto &p:bsp.portals) {
        if(p.plane_index>=bsp.planes.size() || p.vertices.size()>64)
            throw FormatError("source portal geometry exceeds authored bank");
        if(p.back<0 || p.front<0 || std::size_t(p.back)>=bsp.cluster_portals.size() || std::size_t(p.front)>=bsp.cluster_portals.size())
            throw FormatError("source portal endpoint outside cluster bank");
    }
    for(const auto &cluster:bsp.cluster_portals)for(const auto p:cluster)
        if(p>=bsp.portals.size())throw FormatError("source ordered portal index outside BSP");
}
}
SourcePortalBsp Cache::load_source_portal_bsp(std::size_t index) const {
    if(index>=bsps_.size())throw FormatError("source portal BSP index outside scenario");
    const auto &bsp=bsps_[index];const Bytes data(cache_bytes());
    const Bytes metadata(data.slice(bsp.file_offset,bsp.size));
    
    SourcePortalBsp out;
    const auto [collision_count,collision]=TagAddressSpace(metadata, bsp.memory_address).block(16+0x14, 64);
    if(collision_count!=1)throw FormatError("source portals require one collision BSP");
    const auto [plane_count,plane]=TagAddressSpace(metadata, bsp.memory_address).block(collision+8, 16);
    for(std::size_t i=0;i<plane_count;++i){const auto p=plane+i*16;
        out.planes.push_back({metadata.f32(p),metadata.f32(p+4),metadata.f32(p+8),metadata.f32(p+12)});}
    const auto [portal_count,portal]=TagAddressSpace(metadata, bsp.memory_address).block(16+0x5c, 36);
    for(std::size_t i=0;i<portal_count;++i) {
        const auto p=portal+i*36;SourcePortalGeometry entry;
        entry.back=std::int16_t(metadata.u16(p));entry.front=std::int16_t(metadata.u16(p+2));
        entry.plane_index=metadata.u32(p+4);entry.flags=metadata.u32(p+0x18);
        const auto [vertices,vertex]=TagAddressSpace(metadata, bsp.memory_address).block(p+0x1c, 12);
        for(std::size_t j=0;j<vertices;++j){const auto v=vertex+j*12;
            entry.vertices.push_back({metadata.f32(v),metadata.f32(v+4),metadata.f32(v+8)});}
        out.portals.push_back(std::move(entry));
    }
    const auto [cluster_count,cluster]=TagAddressSpace(metadata, bsp.memory_address).block(16+0x9c, 0xb0);
    for(std::size_t i=0;i<cluster_count;++i){const auto [count,start]=TagAddressSpace(metadata, bsp.memory_address).block(cluster+i*0xb0+0x8c, 2);
        std::vector<std::uint16_t> entries;for(std::size_t j=0;j<count;++j)entries.push_back(metadata.u16(start+j*2));
        out.cluster_portals.push_back(std::move(entries));}
    validate(out);return out;
}
SourceClusterFrustumSet SourcePortalVisibilitySet::sphere_bank() const {
    SourceClusterFrustumSet out;out.group_count=std::int16_t(groups.size());out.clusters=clusters;
    for(const auto &f:frustums)out.frustums.push_back({f.bounds,f.planes});
    return out;
}
SourcePortalVisibilitySet source_portal_visibility(const SourcePortalBsp &bsp,
    std::span<const SourceFrustumCamera> groups,int containing_cluster,
    std::span<const std::uint32_t> open_portals) {
    validate(bsp);
    if(groups.size()>6 || open_portals.size()!=16 || containing_cluster< -1 ||
        (containing_cluster>=0 && std::size_t(containing_cluster)>=bsp.cluster_portals.size()))
        throw FormatError("source portal visibility inputs exceed original banks");
    SourcePortalVisibilitySet output;output.groups.assign(groups.begin(),groups.end());
    if(containing_cluster==-1)return output;
    std::vector<int> output_cluster(bsp.cluster_portals.size(),-1);
    for(std::size_t group=0;group<groups.size();++group) {
        const auto &camera=groups[group];
        if(camera.polygon_count>4)throw FormatError("source initial camera polygon exceeds1BC bank");
        SourcePortalProjectionView view{camera.inverse_view,camera.camera_to_world.position,
            camera.full_frustum.rectangle,{camera.polygon.begin(),camera.polygon.begin()+camera.polygon_count},
            camera.far_plane_enabled!=0,camera.far_distance,{0,0,-1,0}};
        std::vector<std::optional<SourcePortalProjection>> projections(bsp.portals.size());
        std::size_t projected_arena=0,traversal_arena=camera.polygon_count;
        std::vector<Node> nodes;nodes.reserve(256);nodes.push_back({unsigned(group),-1,-1,containing_cluster,-1,
            view.rectangle,view.initial_polygon,0,{}});
        std::vector<int> heads(bsp.cluster_portals.size(),-1),links(256,-1);heads[containing_cluster]=0;
        std::deque<unsigned> queue{0};
        while(!queue.empty()) {
            const unsigned current=queue.front();queue.pop_front();
            // Nodes have a fixed256-entry backing store in172840; copying the
            // current fields avoids host vector growth invalidating references.
            const auto node=nodes[current];
            for(const auto pi:bsp.cluster_portals[node.cluster]) {
                if(!projections[pi]) {
                    if(arena_capacity-projected_arena<64) {
                        output.exhausted_limits|=SourcePortalVisibilitySet::projected_polygon_arena;continue;
                    }
                    projections[pi]=source_project_portal(view,bsp.portals[pi],bsp.planes[bsp.portals[pi].plane_index]);
                    projected_arena+=projections[pi]->arena_entries;
                }
                const auto &p=*projections[pi];const unsigned word=pi/32,bit=1u<<(pi%32);
                if((node.visited[word]&bit) || !(open_portals[word]&bit))continue;
                if((p.classification==1 && (p.flags&2)) || (p.classification==2 && (p.flags&16)))continue;
                bool admitted=(p.classification==1 && p.back==node.cluster)||(p.classification==2 && p.front==node.cluster);
                if(p.classification==3) {
                    admitted=true;for(int ancestor=int(current);ancestor!=-1;ancestor=nodes[ancestor].parent)
                        if(nodes[ancestor].portal==pi){admitted=false;break;}
                }
                if(!admitted)continue;
                const auto &a=node.rectangle,&b=p.rectangle;
                if(b[1]<a[0] || a[1]<b[0] || b[3]<a[2] || a[3]<b[2])continue;
                if(nodes.size()==256){output.exhausted_limits|=SourcePortalVisibilitySet::nodes;continue;}
                if(arena_capacity-traversal_arena<64){output.exhausted_limits|=SourcePortalVisibilitySet::traversal_polygon_arena;continue;}
                auto polygon=source_intersect_portal_polygons(p.polygon,node.polygon);
                if(polygon.size()<3)continue;
                Node child;child.group=unsigned(group);child.parent=int(current);child.portal=pi;
                child.cluster=p.back==node.cluster?p.front:p.back;child.rectangle=rectangle(polygon);
                child.polygon=std::move(polygon);child.metric=p.minimum_depth;child.visited=node.visited;child.visited[word]|=bit;
                traversal_arena+=child.polygon.size();const auto ordinal=unsigned(nodes.size());nodes.push_back(std::move(child));
                links[ordinal]=heads[nodes.back().cluster];heads[nodes.back().cluster]=int(ordinal);
                if(queue.size()==255)output.exhausted_limits|=SourcePortalVisibilitySet::queue;
                else queue.push_back(ordinal);
            }
        }
        // Original172C70 compares each cluster's linked nodes in newest-first
        // order, merging into its first qualifying older successor.
        for(std::size_t cluster=0;cluster<heads.size();++cluster)for(int a=heads[cluster];a!=-1;a=links[a]) {
            for(int b=links[a];b!=-1;b=links[b]) {
                const auto &x=nodes[a].rectangle,&y=nodes[b].rectangle;
                const ShaderVector combined{x[0]<y[0]?x[0]:y[0],y[1]>x[1]?y[1]:x[1],
                    x[2]<y[2]?x[2]:y[2],y[3]>x[3]?y[3]:x[3]};
                const float union_area=(combined[3]-combined[2])*(combined[1]-combined[0]);
                const float summed_area=(y[3]-y[2])*(y[1]-y[0])+(x[3]-x[2])*(x[1]-x[0]);
                if(union_area<summed_area*1.5f) {
                    nodes[a].merged=b;nodes[b].metric=nodes[a].metric<=nodes[b].metric?nodes[a].metric:nodes[b].metric;
                    nodes[b].rectangle=combined;break;
                }
            }
        }
        bool stopped=false;
        for(std::size_t i=0;i<nodes.size()&&!stopped;++i) {
            const auto cluster=nodes[i].cluster;if(heads[cluster]!=int(i))continue;
            int entry=output_cluster[cluster];
            if(entry==-1) {
                if(output.clusters.size()==128){output.exhausted_limits|=SourcePortalVisibilitySet::clusters_limit;break;}
                entry=int(output.clusters.size());output_cluster[cluster]=entry;
                SourceClusterFrustumRanges ranges;ranges.source_cluster=std::int16_t(cluster);ranges.first_indices.fill(-1);
                output.clusters.push_back(ranges);
            }
            for(int n=int(i);n!=-1;n=links[n]) {
                if(output.frustums.size()==512){output.exhausted_limits|=SourcePortalVisibilitySet::frustums_limit;stopped=true;break;}
                if(nodes[n].merged!=-1)continue;
                const auto index=std::int16_t(output.frustums.size());output.frustums.emplace_back();
                auto &f=output.frustums.back();
                if(source_frustum_rectangle(camera,nodes[n].rectangle,unsigned(group),f)) {
                    f.runtime_retained[0]=unsigned(entry);f.metric_retained=nodes[n].metric;
                    f.runtime_retained[1]=nodes[n].portal<0?0xffffffffu:bsp.portals[nodes[n].portal].plane_index;
                    auto &ranges=output.clusters[entry];
                    if(ranges.first_indices[group]==-1)ranges.first_indices[group]=index;
                    ++ranges.counts[group];
                }
            }
        }
    }
    output.sorted_cluster_indices.resize(output.clusters.size());
    std::iota(output.sorted_cluster_indices.begin(),output.sorted_cluster_indices.end(),0u);
    std::sort(output.sorted_cluster_indices.begin(),output.sorted_cluster_indices.end(),
        [&](auto a,auto b){return output.clusters[a].source_cluster<output.clusters[b].source_cluster;});
    return output;
}
}
