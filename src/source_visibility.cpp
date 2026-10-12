#include "halo2/source_visibility.hpp"

namespace halo2 {
SourceSphereVisibility source_sphere_frustum_visibility(
    const SourceSphereFrustum& f,Vec3 center,float radius) {
    if(center.x-radius>f.bounds[1] || center.y-radius>f.bounds[3] ||
       center.z-radius>f.bounds[5] || f.bounds[0]>center.x+radius ||
       f.bounds[2]>radius+center.y || f.bounds[4]>radius+center.z)
        return SourceSphereVisibility::outside;
    std::array<float,6> distances;
    for(unsigned i=0;i<6;++i) {
        const auto& p=f.planes[i];
        //181882 onward: z product, y product, ADDSS, x product, ADDSS, SUBSS.
        distances[i]=(p[2]*center.z+p[1]*center.y)+p[0]*center.x-p[3];
        if(distances[i]>radius)return SourceSphereVisibility::outside;
    }
    const float negative_radius=0.f-radius;
    for(unsigned i:{0u,1u,2u,3u,5u})
        if(!(negative_radius>distances[i]))return SourceSphereVisibility::intersecting;
    return SourceSphereVisibility::contained;
}
SourceClusterFrustumResult source_cluster_frustum_visibility(
    const SourceClusterFrustumSet& set,std::size_t section_index,Vec3 center,float radius) {
    SourceClusterFrustumResult result;
    if(set.group_count<=0)return result;
    if(set.group_count>6 || section_index>=set.clusters.size())
        throw FormatError("source cluster frustum group/entry unavailable");
    const auto& ranges=set.clusters[section_index];
    for(int group=0;group<set.group_count;++group) {
        for(int i=0;i<ranges.counts[group] && !result.contained;++i) {
            const auto index=int(ranges.first_indices[group])+i;
            if(index<0 || std::size_t(index)>=set.frustums.size())
                throw FormatError("source cluster frustum range unavailable");
            const auto classification=source_sphere_frustum_visibility(set.frustums[index],center,radius);
            if(classification!=SourceSphereVisibility::outside) {
                result.visible=true;
                result.contained=classification==SourceSphereVisibility::contained;
            }
        }
    }
    return result;
}
}
