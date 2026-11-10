#pragma once
#include "halo2/native_tu15_geometry_visibility_kernel.hpp"
#include "halo2/source_visibility.hpp"
#include <cmath>

namespace halo2::native_geometry_visibility_detail {
class HostView {
    const SourceClusterFrustumSet&frustums_;std::size_t ordinal_;Vec3 center_,volume_center_;float radius_,volume_radius_;
    unsigned mode_;bool skip_,special_;SourceClusterFrustumResult result_;
public:
    using Scalar=float;
    HostView(const SourceClusterFrustumSet&frustums,std::size_t ordinal,Vec3 center,float radius,
        unsigned mode,bool skip,bool special,Vec3 volume_center,float volume_radius):
        frustums_(frustums),ordinal_(ordinal),center_(center),volume_center_(volume_center),radius_(radius),volume_radius_(volume_radius),mode_(mode),skip_(skip),special_(special){}
    void begin(){result_={true,false};}unsigned mode()const{return mode_;}bool skip()const{return skip_;}
    void accept_skip(){result_={true,false};}
    bool frustum(){result_=source_cluster_frustum_visibility(frustums_,ordinal_,center_,radius_);return result_.visible;}
    bool contained()const{return result_.contained;}
    void promote_small(){if(!special_&&radius_<=2.5f)result_.contained=true;}
    bool visible()const{return result_.visible;}
    static float add(float a,float b){return a+b;}
    static float subtract(float a,float b){return a-b;}
    static float multiply(float a,float b){return a*b;}
    float scalar(Read field,const Progress<float>&)const{
        switch(field){
        case Read::center_z:return center_.z;case Read::volume_z:return volume_center_.z;
        case Read::center_x:return center_.x;case Read::volume_x:return volume_center_.x;
        case Read::center_y:return center_.y;case Read::volume_y:return volume_center_.y;
        case Read::radius:return radius_;case Read::volume_radius:return volume_radius_;
        }
        return 0;
    }
    // Keep the public predecessor's unordered C++ comparison policy. The raw
    // view separately owns the unchanged CPU's COMISS condition semantics.
    bool overlap_result(const Progress<float>&d){result_.visible=!(d.radius_squared<d.distance_squared);return result_.visible;}
    void sphere_containment(){
        const double x=double(volume_center_.x)-center_.x,y=double(volume_center_.y)-center_.y,z=double(volume_center_.z)-center_.z;
        result_.contained=!(double(volume_radius_)<std::sqrt((z*z+y*y)+x*x)+radius_);
    }
    void auxiliary(){} // Actual host auxiliary membership stays at its owner.
    SourceClusterFrustumResult finish()const{return result_;}
    SourceClusterFrustumResult outside(){result_.visible=false;return result_;}
};
inline SourceClusterFrustumResult geometry(const SourceClusterFrustumSet&frustums,std::size_t ordinal,Vec3 center,float radius,
    unsigned mode,bool skip,bool special,Vec3 volume_center,float volume_radius){
    HostView view{frustums,ordinal,center,radius,mode,skip,special,volume_center,volume_radius};return admit(view);
}
} // namespace halo2::native_geometry_visibility_detail
