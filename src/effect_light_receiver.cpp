#include "halo2/effect_light_receiver.hpp"
#include <bit>

namespace halo2 {
SourceEffectLightReceiverView source_effect_light_receiver_view(
    const SourceEffectLightReceiverWorld& world,
    std::int16_t containing_cluster,Vec3 spatial_center,float spatial_radius,
    const EffectLightSphere& shape,std::span<const std::uint8_t> containing_fog,
    const SourceBspAdmission* auxiliary) {
    if((shape.shape[0]&0xffffu)!=0)
        throw FormatError("source effect receiver requires spherical C1D90 shape");
    if(!(spatial_radius<3.f))
        throw FormatError("source effect receiver requires unported large-bound portal intersection");
    const auto f=[&](unsigned i){return std::bit_cast<float>(shape.shape[i]);};
    const auto cameras=source_effect_light_cameras({f(11),f(12),f(13)},f(17));
    SourceEffectLightReceiverView out;out.flags=auxiliary?1u:0u;
    out.location={0xffffffffu,containing_cluster,world.selected_bsp};
    if(containing_cluster==-1)
        out.location=source_object_point_location(world.locations,world.selected_bsp,spatial_center);
    for(const auto& c:cameras)out.visibility.groups.push_back(c.camera);
    const auto clusters=source_bsp_sphere_clusters(world.spatial,out.location.cluster,
        spatial_center,spatial_radius);
    for(const auto id:clusters.clusters) {
        SourceClusterFrustumRanges range;range.source_cluster=id;
        //132D60 owns +0/+2 only. Mode1 ignores the retained other ranges.
        out.visibility.clusters.push_back(range);
    }
    out.visibility.sorted_cluster_indices=clusters.sorted_indices;
    SourceBspAdmissionInput in;
    in.cluster_capacity=source_bsp_light_cluster_capacity;
    in.instance_capacity=source_bsp_light_instance_capacity;
    in.flags=out.flags;in.context_2a78=out.pass_mode;in.mode=out.query_mode;
    in.sphere_center=spatial_center;in.sphere_radius=spatial_radius;
    in.containing_fog=containing_fog;in.auxiliary=auxiliary;
    out.bsp=source_bsp_admission(world.geometry,out.visibility,in);
    return out;
}
}
