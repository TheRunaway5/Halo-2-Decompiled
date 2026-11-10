#include "halo2/source_visibility.hpp"
#include "halo2/native_tu15_cluster_frustum_value.hpp"
#include "halo2/native_tu15_sphere_frustum_value.hpp"

namespace halo2 {
SourceSphereVisibility source_sphere_frustum_visibility(
    const SourceSphereFrustum& f,Vec3 center,float radius) {
    return native_tu15_sphere_frustum_value(f,center,radius);
}
SourceClusterFrustumResult source_cluster_frustum_visibility(
    const SourceClusterFrustumSet& set,std::size_t section_index,Vec3 center,float radius) {
    return native_cluster_frustum_value_detail::visible(set,section_index,center,radius);
}
}
