#include "halo2/source_shadow_view.hpp"
#include "halo2/math.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>

namespace halo2 {
namespace {
void finite(float v) {
    if(!std::isfinite(v))throw FormatError("source shadow view has unresolved nonfinite input");
}
void finite(Vec3 v){finite(v.x);finite(v.y);finite(v.z);}
void finite(const Transform& t){finite(t.scale);for(auto v:t.basis)finite(v);finite(t.position);}
Vec3 box_point(const Transform& t,Vec3 v){
    if(t.scale!=1.f){v.x*=t.scale;v.y*=t.scale;v.z*=t.scale;}
    //320B0 differs from1429D0: (X+Z)+Y, then translation.
    return {((t.basis[0].x*v.x+t.basis[2].x*v.z)+t.basis[1].x*v.y)+t.position.x,
            ((t.basis[0].y*v.x+t.basis[2].y*v.z)+t.basis[1].y*v.y)+t.position.y,
            ((t.basis[0].z*v.x+t.basis[2].z*v.z)+t.basis[1].z*v.y)+t.position.z};
}
}
SourceShadowViewVolume source_shadow_view_volume(const SourceFrustumCamera& camera,
    const SourceShadowViewBox& box) {
    finite(box.world_from_box);finite(box.half_extent);
    if(box.half_extent.x<0 || box.half_extent.y<0 || box.half_extent.z<0)
        throw FormatError("source shadow view box has negative half extent");
    finite(camera.inverse_view);finite(camera.camera_to_world.basis[2]);
    finite(camera.full_frustum.eye);finite(camera.full_frustum.metric_retained);
    finite(camera.near_distance);
    for(float v:camera.full_frustum.rectangle)finite(v);
    const auto h=box.half_extent;
    const std::array<Vec3,8> corners{{{0.f-h.x,0.f-h.y,0.f-h.z},{h.x,0.f-h.y,0.f-h.z},
        {0.f-h.x,h.y,0.f-h.z},{h.x,h.y,0.f-h.z},{0.f-h.x,0.f-h.y,h.z},
        {h.x,0.f-h.y,h.z},{0.f-h.x,h.y,h.z},{h.x,h.y,h.z}}};
    constexpr std::array<unsigned,24> edge{{0,2,2,3,3,1,1,0,0,4,1,5,2,6,3,7,4,5,5,7,7,6,6,4}};
    SourceShadowViewVolume result;
    for(unsigned i=0;i<edge.size();++i)result.world_endpoints[i]=box_point(box.world_from_box,corners[edge[i]]);
    auto clipped=result.world_endpoints;
    const auto backward=camera.camera_to_world.basis[2],eye=camera.full_frustum.eye;
    float bias=camera.full_frustum.metric_retained+camera.near_distance;
    const auto minimum=std::bit_cast<float>(0x37800006u);
    if(minimum>bias)bias=minimum;
    const Vec3 n{0.f-backward.x,0.f-backward.y,0.f-backward.z};
    const float d=(0.f-((eye.y*backward.y+eye.z*backward.z)+eye.x*backward.x))+bias;
    const auto epsilon=std::bit_cast<float>(0xb8d1b717u);
    std::array<bool,24> front{};
    for(unsigned i=0;i<clipped.size();++i){
        const auto p=clipped[i];
        //165550's four-endpoint unroll has a distinct first dot order.
        const float distance=i%4==0?((p.x*n.x+p.z*n.z)+n.y*p.y)-d:
            ((p.y*n.y+p.x*n.x)+p.z*n.z)-d;
        front[i]=distance>epsilon;
    }
    const auto largest=std::numeric_limits<float>::max();
    result.projected_bounds={largest,0.f-largest,largest,0.f-largest,largest,0.f-largest};
    bool survived=false;
    for(unsigned i=0;i<clipped.size();i+=2){
        if(!front[i]&&!front[i+1])continue;
        if(front[i]!=front[i+1]){
            const auto p=clipped[i];const Vec3 delta=clipped[i+1]-p;
            const float denominator=(delta.x*n.x+delta.z*n.z)+n.y*delta.y;
            const float numerator=((n.y*p.y+p.z*n.z)+n.x*p.x)-d;
            const float t=denominator==0.f?0.f:numerator*(-1.f/denominator);
            const Vec3 hit{t*delta.x+p.x,delta.y*t+p.y,delta.z*t+p.z};
            clipped[front[i]?i+1:i]=hit;
        }
        survived=true;
        for(unsigned j=i;j<i+2;++j){
            auto p=clipped[j];const auto& t=camera.inverse_view;
            if(t.scale!=1.f){p.x*=t.scale;p.y*=t.scale;p.z*=t.scale;}
            const float x=((t.basis[2].x*p.z+t.basis[1].x*p.y)+t.basis[0].x*p.x)+t.position.x;
            const float y=((t.basis[2].y*p.z+t.basis[1].y*p.y)+t.basis[0].y*p.x)+t.position.y;
            const float z=((t.basis[2].z*p.z+t.basis[1].z*p.y)+t.basis[0].z*p.x)+t.position.z;
            const auto negative_minimum=std::bit_cast<float>(0xb7800006u);
            const float denominator=z>negative_minimum?negative_minimum:z;
            const float inverse=-1.f/denominator;
            const std::array<float,3> q{x*inverse,y*inverse,z};
            for(unsigned k=0;k<3;++k){
                if(result.projected_bounds[k*2]>q[k])result.projected_bounds[k*2]=q[k];
                if(q[k]>result.projected_bounds[k*2+1])result.projected_bounds[k*2+1]=q[k];
            }
        }
    }
    const auto& r=camera.full_frustum.rectangle;const auto& b=result.projected_bounds;
    result.admitted=survived && b[1]>=r[0] && r[1]>=b[0] && b[3]>=r[2] && r[3]>=b[2];
    return result;
}
SourceShadowSecondaryCamera source_shadow_secondary_camera(
    const SourceShadowSecondaryProjection& projection,Vec3 direction,Vec3 up) {
    const auto& w=projection.words;
    if(w[0]!=1 && w[0]!=2)
        throw FormatError("source secondary shadow camera requires proved single-camera mode1/2");
    const auto f=[&](unsigned i){const float v=std::bit_cast<float>(w[i]);finite(v);return v;};
    finite(direction);finite(up);
    const Vec3 left{f(7),f(8),f(9)},eye{f(11),f(12),f(13)},ray{f(15),f(16),f(17)};
    const float near=f(10),width=f(22),height=f(23),far_offset=f(25);
    //Unlike441B0, C1D90 copies these bases without normalization or cross.
    SourceShadowSecondaryCamera result{};auto& c=result.camera;
    c.camera_to_world={1.f,{left,up,{0.f-direction.x,0.f-direction.y,0.f-direction.z}},eye};
    c.orthographic=w[0]==1;
    c.near_plane_enabled=near>0.f;c.near_distance=near;
    const Vec3 near_point{direction.x*near+eye.x,direction.y*near+eye.y,direction.z*near+eye.z};
    c.near_plane={direction.x,direction.y,direction.z,
        (direction.z*near_point.z+direction.y*near_point.y)+direction.x*near_point.x};
    c.far_distance=far_offset+near;
    const float local_x=(left.z*ray.z+left.y*ray.y)+left.x*ray.x;
    const float local_y=(up.z*ray.z+up.y*ray.y)+up.x*ray.x;
    const auto backward=c.camera_to_world.basis[2];
    const float local_z=(backward.z*ray.z+backward.y*ray.y)+backward.x*ray.x;
    //C1A80 mode1 supplies near0. C1D90 performs IEEE division without a
    //zero-denominator guard; preserve the completed camera/empty-frustum path.
    const float inverse=-1.f/local_z;
    const float x=local_x*inverse,y=local_y*inverse,near_inverse=1.f/near;
    const float half_width=(width*near_inverse)*.5f,half_height=(height*near_inverse)*.5f;
    const ShaderVector rectangle{x-half_width,x+half_width,y-half_height,y+half_height};
    c.polygon_count=4;
    c.polygon={Vec2{rectangle[0],rectangle[2]},Vec2{rectangle[1],rectangle[2]},
        Vec2{rectangle[1],rectangle[3]},Vec2{rectangle[0],rectangle[3]}};
    c.full_frustum_enabled=1;c.far_plane_enabled=1;
    c.inverse_view=inverse_transform(c.camera_to_world);
    source_frustum_rectangle(c,rectangle,0,c.full_frustum,&result.plane_inconsistent);
    return result;
}
std::array<SourceShadowSecondaryCamera,6> source_effect_light_cameras(
    Vec3 position,float radius) {
    finite(position);finite(radius);
    if(!(radius>0.f))throw FormatError("source spherical light camera requires positive radius");
    const float negative=0.f-radius;
    // C1D90's literal eight corners and four vertex indices per face.
    const std::array<Vec3,8> corners{{{radius,radius,negative},{negative,radius,negative},
        {negative,negative,negative},{radius,negative,negative},{radius,radius,radius},
        {negative,radius,radius},{negative,negative,radius},{radius,negative,radius}}};
    constexpr std::array<unsigned,24> indices{{0,1,2,3,0,3,7,4,0,4,5,1,
        1,5,6,2,2,6,7,3,4,7,6,5}};
    constexpr std::array<Vec3,6> normals{{{0,0,-1},{1,0,0},{0,1,0},
        {-1,0,0},{0,-1,0},{0,0,1}}};
    std::array<SourceShadowSecondaryCamera,6> result{};
    for(unsigned face=0;face<6;++face){
        auto& c=result[face].camera;
        const auto n=normals[face];
        const Vec3 backward{0.f-n.x,0.f-n.y,0.f-n.z};
        const Vec3 reference=(face==1||face==3)?Vec3{0,1,0}:Vec3{1,0,0};
        Vec3 up{reference.z*backward.y-backward.z*reference.y,
            backward.z*reference.x-backward.x*reference.z,
            backward.x*reference.y-backward.y*reference.x};
        const float length=float(std::sqrt((double(up.y)*up.y+double(up.z)*up.z)+double(up.x)*up.x));
        if(std::bit_cast<float>(0x38d1b717u)<=std::fabs(length)){
            const float scale=1.f/length;up={scale*up.x,up.y*scale,up.z*scale};
        }
        const Vec3 left{up.y*backward.z-up.z*backward.y,
            up.z*backward.x-backward.z*up.x,backward.y*up.x-up.y*backward.x};
        c.camera_to_world={1.f,{left,up,backward},position};
        c.far_distance=radius;c.far_plane_enabled=1;c.full_frustum_enabled=1;
        c.polygon_count=4;
        for(unsigned vertex=0;vertex<4;++vertex){
            const auto p=corners[indices[face*4+vertex]];
            const float inverse=-1.f/((backward.z*p.z+backward.x*p.x)+backward.y*p.y);
            c.polygon[vertex]={inverse*((left.y*p.y+left.x*p.x)+left.z*p.z),
                inverse*((p.x*up.x+up.z*p.z)+up.y*p.y)};
        }
        ShaderVector rectangle{std::numeric_limits<float>::max(),-std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max(),-std::numeric_limits<float>::max()};
        for(auto p:c.polygon){
            if(p.x<rectangle[0])rectangle[0]=p.x;if(rectangle[1]<p.x)rectangle[1]=p.x;
            if(p.y<rectangle[2])rectangle[2]=p.y;if(rectangle[3]<p.y)rectangle[3]=p.y;
        }
        c.inverse_view=inverse_transform(c.camera_to_world);
        source_frustum_rectangle(c,rectangle,0,c.full_frustum,&result[face].plane_inconsistent);
    }
    return result;
}
}
