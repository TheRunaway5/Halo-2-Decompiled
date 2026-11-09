#pragma once
#include "halo2/source_visibility.hpp"
#include "halo2/native_tu15_sphere_frustum_kernel.hpp"

namespace halo2::native_sphere_frustum_value_detail {
// Value-policy view of the same algorithm as the retained TU1.5 adapter.
// Keep the public C++ comparison contract; complete COMISS/escaped-register
// behavior belongs to the separately qualified retained ABI view.
struct Frame {
    using Scalar=float;
    const SourceSphereFrustum&frustum;Vec3 center;float extent;
    float radius()const{return extent;}
    float zero()const{return 0.f;}
    float add(float a,float b)const{return a+b;}
    float subtract(float a,float b)const{return a-b;}
    float multiply(float a,float b)const{return a*b;}
    float coordinate(unsigned axis)const{return axis==0?center.x:axis==1?center.y:center.z;}
    float read(native_sphere_frustum_detail::Read kind,const native_sphere_frustum_detail::Work<float>&d)const{
        using R=native_sphere_frustum_detail::Read;
        switch(kind){
        case R::lower_center:case R::upper_center:return coordinate(d.axis);
        case R::upper_bound:return frustum.bounds[d.axis*2u+1u];
        case R::lower_bound:return frustum.bounds[d.axis*2u];
        case R::normal_z:return frustum.planes[d.plane][2];case R::normal_y:return frustum.planes[d.plane][1];case R::normal_x:return frustum.planes[d.plane][0];
        case R::center_z:return center.z;case R::center_y:return center.y;case R::center_x:return center.x;
        case R::plane_distance:return frustum.planes[d.plane][3];
        }
        throw FormatError("Invalid sphere/frustum value field");
    }
    bool above(float a,float b,native_sphere_frustum_detail::Comparison,const native_sphere_frustum_detail::Work<float>&)const{return a>b;}
    native_sphere_frustum_detail::Visibility finish(native_sphere_frustum_detail::Visibility value,const native_sphere_frustum_detail::Work<float>&)const{return value;}
};
}
namespace halo2 {
inline SourceSphereVisibility native_tu15_sphere_frustum_value(const SourceSphereFrustum&frustum,Vec3 center,float radius){
    native_sphere_frustum_value_detail::Frame frame{frustum,center,radius};
    return SourceSphereVisibility(native_sphere_frustum_detail::classify(frame));
}
}
