#include "halo2/native_tu15_bsp_admission_value.hpp"
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
    return native_bsp_admission_detail::values(bsp,visible,in,cluster_sides,sides);
}
}
