#pragma once
#include <cstdint>

namespace halo2::native_geometry_visibility_detail {
enum class Read {center_z,volume_z,center_x,volume_x,center_y,volume_y,radius,volume_radius};
template<class Scalar>struct Progress {
    Scalar z{},x{},y{},radius{},combined_radius{},square_z{},square_x{},square_y{},distance_squared{},radius_squared{};
};
// Shared135970 geometry policy. The view owns actual lazy input acquisition,
// saved caller storage and separately owned frustum/auxiliary services. Its
// extended norm preserves the source's active x87 precision/rounding policy.
template<class View>bool overlaps(View&view){
    Progress<typename View::Scalar>d;
    d.z=view.scalar(Read::center_z,d);d.z=view.subtract(d.z,view.scalar(Read::volume_z,d));
    d.x=view.scalar(Read::center_x,d);d.x=view.subtract(d.x,view.scalar(Read::volume_x,d));
    d.y=view.scalar(Read::center_y,d);d.y=view.subtract(d.y,view.scalar(Read::volume_y,d));
    d.radius=view.scalar(Read::radius,d);d.combined_radius=view.add(d.radius,view.scalar(Read::volume_radius,d));
    d.square_z=view.multiply(d.z,d.z);d.square_x=view.multiply(d.x,d.x);d.square_y=view.multiply(d.y,d.y);
    d.distance_squared=view.add(d.square_z,d.square_x);d.distance_squared=view.add(d.distance_squared,d.square_y);
    d.radius_squared=view.multiply(d.combined_radius,d.combined_radius);
    return view.overlap_result(d);
}
template<class View>auto admit(View&view){
    view.begin();
    const auto mode=view.mode();
    if(mode==0){
        bool promote=true;
        if(view.skip())view.accept_skip();
        else{
            if(!view.frustum())return view.finish();
            promote=!view.contained();
        }
        if(promote)view.promote_small();
        if(!view.visible())return view.finish();
    }else if(mode==1){
        if(!overlaps(view))return view.outside();
        view.sphere_containment();
    }
    // Other raw context modes intentionally retain metadata-only admission.
    // Public policy rejects them before constructing its validated value view.
    view.auxiliary();
    return view.finish();
}
} // namespace halo2::native_geometry_visibility_detail
