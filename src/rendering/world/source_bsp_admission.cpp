#include "halo2/native_tu15_geometry_visibility_value.hpp"
#include "halo2/native_tu15_visibility_association_value.hpp"
#include "halo2/native_tu15_id_search_kernel.hpp"
#include "halo2/source_bsp_admission.hpp"
#include "halo2/bsp_atmosphere.hpp"
#include <algorithm>
#include <cmath>

namespace halo2 {
namespace {
void validate(const SourceBspAdmissionBsp &bsp) {
    if(bsp.clusters.size()>512 || bsp.instances.size()>32768)
        throw FormatError("source BSP admission exceeds original cluster/instance index banks");
    for(const auto &c:bsp.clusters)for(const auto id:c.instances)
        if(id>=bsp.instances.size())throw FormatError("source BSP admission instance ID outside authored bank");
    for(const auto &i:bsp.instances)
        if(i.definition<0 || std::size_t(i.definition)>=bsp.definitions.size())
            throw FormatError("source BSP admission definition outside authored bank");
}
struct Sides {bool negative=true,positive=true;};
Sides sides(Vec3 center,double radius,const SourceBspAdmissionInput &in) {
    if(!(in.flags&0x200))return {};
    const auto &p=in.clipping_plane;
    const float distance=(p[2]*center.z+p[1]*center.y)+center.x*p[0]-p[3];
    if(double(std::abs(distance))<=radius)return {};
    return {distance<0.f,distance>0.f};
}
Sides cluster_sides(const SourceBspAdmissionCluster &c,const SourceBspAdmissionInput &in) {
    const Vec3 midpoint{(c.bounds[0]+c.bounds[1])*.5f,
        (c.bounds[2]+c.bounds[3])*.5f,(c.bounds[4]+c.bounds[5])*.5f};
    const Vec3 extent{c.bounds[1]-midpoint.x,c.bounds[3]-midpoint.y,c.bounds[5]-midpoint.z};
    //13448A..1344CC uses x87 PC53, retaining double z²+y²+x² and root.
    const double radius=std::sqrt((double(extent.z)*extent.z+double(extent.y)*extent.y)+double(extent.x)*extent.x);
    return sides(midpoint,radius,in);
}
std::uint16_t side_flags(Sides s) {return std::uint16_t((s.negative?0x8000:0)|(s.positive?0x4000:0));}
void bit(std::array<std::uint32_t,4> &mask,std::size_t ordinal) {mask[ordinal/32]|=1u<<(ordinal%32);}
}
SourceBspSphereClusters source_bsp_sphere_clusters(const ObjectLightSpatialBsp& spatial,
    std::int16_t containing_cluster,Vec3 position,float radius){
    if(spatial.clusters.size()>512)throw FormatError("source sphere visibility exceeds512 clusters");
    SourceBspSphereClusters out;
    out.clusters=source_object_light_spatial_membership(spatial,containing_cluster,position,radius,{},512).clusters;
    out.sorted_indices.resize(out.clusters.size());
    for(unsigned i=0;i<out.sorted_indices.size();++i)out.sorted_indices[i]=i;
    std::sort(out.sorted_indices.begin(),out.sorted_indices.end(),[&](unsigned a,unsigned b){return out.clusters[a]<out.clusters[b];});
    return out;
}
SourceBspAdmissionBsp Cache::load_source_bsp_admission(std::size_t index) const {
    if(index>=bsps_.size())throw FormatError("source BSP admission index outside scenario");
    const auto &bsp=bsps_[index];const Bytes data(cache_bytes());
    const Bytes metadata(data.slice(bsp.file_offset,bsp.size));
    
    SourceBspAdmissionBsp out;
    const auto [nc,clusters]=TagAddressSpace(metadata, bsp.memory_address).block(16+0x9c, 0xb0);
    for(std::size_t i=0;i<nc;++i){const auto p=clusters+i*0xb0;SourceBspAdmissionCluster c;
        c.geometry_count=metadata.u16(p+0x24);for(unsigned j=0;j<6;++j)c.bounds[j]=metadata.f32(p+0x54+j*4);
        const auto [n,ids]=TagAddressSpace(metadata, bsp.memory_address).block(p+0x98, 2);for(std::size_t j=0;j<n;++j)c.instances.push_back(metadata.u16(ids+j*2));
        out.clusters.push_back(std::move(c));}
    const auto [ni,instances]=TagAddressSpace(metadata, bsp.memory_address).block(16+0x140, 0x58);
    for(std::size_t i=0;i<ni;++i){const auto p=instances+i*0x58;
        out.instances.push_back({std::int16_t(metadata.u16(p+0x34)),
            {metadata.f32(p+0x3c),metadata.f32(p+0x40),metadata.f32(p+0x44)},metadata.f32(p+0x48)});}
    const auto [nd,definitions]=TagAddressSpace(metadata, bsp.memory_address).block(16+0x138, 0xc8);
    for(std::size_t i=0;i<nd;++i)out.definitions.push_back({metadata.u16(definitions+i*0xc8+0x24)});
    validate(out);return out;
}
bool source_bsp_cluster_contains_fog(const BspAtmosphere &atmosphere,int cluster,
    const SourceBspFogAdmissionInput &in) {
    if(!in.scenario_loaded || !in.bsp_loaded || cluster==-1)return false;
    if(in.flags_4e64c8&0x10)return true;
    if(cluster<0 || std::size_t(cluster)>=atmosphere.clusters.size())
        throw FormatError("source16E210 cluster outside active BSP");
    return atmosphere.contains_planar(cluster,in.datum);
}
std::vector<std::uint8_t> source_bsp_fog_admission(const BspAtmosphere &atmosphere,
    const SourceBspFogAdmissionInput &in) {
    if(atmosphere.clusters.size()>512)throw FormatError("source16E210 cluster bank exceeds512 entries");
    std::vector<std::uint8_t> out(atmosphere.clusters.size());
    for(std::size_t i=0;i<out.size();++i)out[i]=source_bsp_cluster_contains_fog(atmosphere,int(i),in);
    return out;
}
const SourceBspAdmissionRecord *SourceBspAdmission::instance(std::size_t id) const {
    return native_id_search_detail::record(instances,id);
}
bool SourceBspAdmission::geometry_visible(int encoding) const {
    if(encoding==-1)return true;
    if(encoding<=-2)return instance(std::size_t(-std::int64_t(encoding)-2))!=nullptr;
    return std::any_of(clusters.begin(),clusters.end(),[&](const auto &r){return r.id==unsigned(encoding);});
}
bool SourceBspAdmission::geometry_contains_fog(int encoding) const {
    if(encoding==-1)return false;
    if(encoding<=-2){const auto *r=instance(std::size_t(-std::int64_t(encoding)-2));return r&&(r->flags&0x400);}
    const auto r=std::find_if(clusters.begin(),clusters.end(),[&](const auto &r){return r.id==unsigned(encoding);});
    return r!=clusters.end()&&(r->flags&0x400);
}
SourceBspAdmission source_bsp_admission(const SourceBspAdmissionBsp &bsp,
    const SourcePortalVisibilitySet &visible,const SourceBspAdmissionInput &in) {
    validate(bsp);
    if(visible.clusters.size()>128 || visible.groups.size()>6 || visible.frustums.size()>512 ||
        in.cluster_capacity<1 || in.instance_capacity<1 || in.cluster_capacity>32768 ||
        in.instance_capacity>32768 || in.containing_fog.size()!=bsp.clusters.size() ||
        in.mode>1 || (in.flags&~0x6a1u))throw FormatError("unsupported source BSP admission context");
    SourceBspAdmission out;out.cluster_entries.fill(0xff);
    if(in.flags&0x20)return out;
    const auto spheres=visible.sphere_bank();
    std::vector<int> admitted(bsp.instances.size(),-1);
    for(std::size_t ordinal=0;ordinal<visible.clusters.size();++ordinal) {
        const auto authored=visible.clusters[ordinal].source_cluster;
        if(authored<0 || std::size_t(authored)>=bsp.clusters.size())throw FormatError("source BSP admission constructed cluster outside authored bank");
        const auto &cluster=bsp.clusters[authored];const bool fog=in.containing_fog[authored]!=0;
        out.marked_clusters[authored/32]|=1u<<(authored%32);out.cluster_entries[authored]=std::uint8_t(ordinal);
        const bool main_cluster=!in.auxiliary || !(in.flags&1) || in.auxiliary->geometry_visible(authored);
        if(cluster.geometry_count && main_cluster && out.clusters.size()<in.cluster_capacity-1) {
            out.clusters.push_back({std::uint32_t(authored),std::uint16_t(0x3000|side_flags(cluster_sides(cluster,in))|(fog?0x400:0)),0xffff,0});
        }
        //1345DB's missing auxiliary cluster skips this cluster's instance
        //walk as well. Clusters with no geometry take1345F4 directly.
        if(cluster.geometry_count && !main_cluster)continue;
        for(const auto id:cluster.instances) {
            if(admitted[id]!=-1) {
                auto &record=out.instances[admitted[id]];
                if(!(record.flags&0x800))bit(out.associations[record.association],ordinal);
                if(fog)record.flags|=0x400;
                continue;
            }
            const auto &i=bsp.instances[id];if(!bsp.definitions[i.definition].geometry_count)continue;
            const auto result=native_geometry_visibility_detail::geometry(spheres,ordinal,i.center,i.radius,in.mode,false,in.context_2a78!=0,in.sphere_center,in.sphere_radius);
            if(!result.visible && !(in.flags&0x80))continue;
            std::uint16_t association=0xffff;
            if(!result.contained) {
                association=native_visibility_association_detail::reserve_mask(out.associations,out.association_capacity_reached,ordinal);
            }
            if(out.instances.size()<in.instance_capacity-1) {
                const bool main_instance=!result.visible || !in.auxiliary || !(in.flags&1) || in.auxiliary->instance(id)!=nullptr;
                const auto flags=std::uint16_t((main_instance?0x3000:0)|side_flags(sides(i.center,double(i.radius),in))|
                    (fog?0x400:0)|(result.contained?0x800:0));
                admitted[id]=int(out.instances.size());out.instances.push_back({id,flags,association,0});
            }
        }
    }
    return out;
}
}
