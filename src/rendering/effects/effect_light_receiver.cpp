#include "halo2/effect_light_receiver.hpp"
#include "halo2/source_object_admission.hpp"
#include <bit>
#include <cstdlib>

namespace halo2 {
bool source_effect_light_append_parent_biped(SourceObjectAdmission& out,
    const SourceEffectLightParentBipedInput& in,const std::function<std::uint32_t(std::uint32_t)>& parent) {
    if(!(in.definition_flags&0x400000u))return false;
    if(in.shape || !(in.query_flags&0x400u) || !in.capacity)
        throw FormatError("Parent-biped light lost its original sphere query policy");
    if(in.attached_node==-1 || in.owner==0xffffffffu)return false;
    if(!parent)throw FormatError("Parent-biped light has no native owner walk");
    const auto object=parent(in.owner);
    if(out.objects.size()>=in.capacity-1u)return false;
    out.objects.push_back({object,0x1000u,0xffffu,0});return true;
}
SourceEffectLightReceiverView source_effect_light_receiver_view(
    const SourceEffectLightReceiverWorld& world,
    std::int16_t containing_cluster,Vec3 spatial_center,float spatial_radius,
    const EffectLightSphere& shape,std::span<const std::uint8_t> containing_fog,
    const SourceBspAdmission* auxiliary,std::uint32_t definition_flags) {
    if((shape.shape[0]&0xffffu)!=0)
        throw FormatError("source effect receiver requires spherical C1D90 shape");
    if(!(spatial_radius<3.f))
        throw FormatError("source effect receiver requires unported large-bound portal intersection");
    const auto f=[&](unsigned i){return std::bit_cast<float>(shape.shape[i]);};
    const auto cameras=source_effect_light_cameras({f(11),f(12),f(13)},f(17));
    SourceEffectLightReceiverView out;out.flags=(auxiliary?1u:0u)|((definition_flags>>12)&0x400u);
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
    static const bool reference=[]{const auto* p=std::getenv("HALO2_REFERENCE_LIGHT_FLAGS_31650");return p&&*p&&*p!='0';}();
    if(reference&&(definition_flags&0x10000u))out.bsp_before_flags=out.bsp;
    source_effect_light_bsp_flags_31650(out.bsp,definition_flags);
    return out;
}
}
