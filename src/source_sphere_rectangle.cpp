#include "halo2/source_sphere_rectangle.hpp"
#include <cmath>
#include <limits>

namespace halo2 {
namespace {
float mul(float a,float b){const volatile float v=a*b;return v;}
float add(float a,float b){const volatile float v=a+b;return v;}
float sub(float a,float b){const volatile float v=a-b;return v;}
float div(float a,float b){const volatile float v=a/b;return v;}
float pixel(float coordinate,float depth,float scale,float offset,float first,float last) {
    const float projected=add(mul(div(coordinate,depth),scale),offset);
    return add(mul(sub(1.f,projected),first),mul(add(projected,1.f),last));
}
std::array<float,2> axis(float x,float z,float radius,float z_squared,
    float radius_squared,float scale,float offset,float first,float last) {
    constexpr auto maximum=std::numeric_limits<float>::max();
    const float squared=add(mul(x,x),z_squared),delta=sub(squared,radius_squared);
    if(!(delta>1.e-4f))return {-maximum,maximum};
    const float root=std::sqrt(delta),ratio=div(radius,squared);
    const float zr=mul(z,radius),rx=mul(root,x),xr=mul(x,radius),rz=mul(root,z);
    const float low_depth=sub(z,mul(add(rx,zr),ratio));
    const float high_depth=sub(z,mul(sub(zr,rx),ratio));
    const float low=low_depth>.01f?pixel(sub(x,mul(add(xr,rz),ratio)),low_depth,scale,offset,first,last):-maximum;
    const float high=high_depth>.01f?pixel(sub(x,mul(sub(xr,rz),ratio)),high_depth,scale,offset,first,last):maximum;
    return {low,high};
}
}
std::array<float,4> source_sphere_rectangle(const SourceViewProjectionBank& bank,
    Vec3 eye,std::array<std::int16_t,4> viewport,Vec3 center,float radius) {
    const auto& view=bank.inverse_view;const auto& b=view.basis;
    auto x=sub(center.x,eye.x),y=sub(center.y,eye.y),z=sub(center.z,eye.z);
    if(view.scale!=1.f){x=mul(view.scale,x);y=mul(view.scale,y);z=mul(view.scale,z);}
    const float horizontal=add(add(mul(b[2].x,z),mul(b[1].x,y)),mul(b[0].x,x));
    const float vertical=add(add(mul(b[2].y,z),mul(b[1].y,y)),mul(b[0].y,x));
    const float depth=mul(add(add(mul(b[2].z,z),mul(b[1].z,y)),mul(b[0].z,x)),-1.f);
    constexpr auto maximum=std::numeric_limits<float>::max();
    if(!(add(depth,radius)>.01f))return {maximum,-maximum,maximum,-maximum};
    const float z_squared=mul(depth,depth),radius_squared=mul(radius,radius);
    const auto h=axis(horizontal,depth,radius,z_squared,radius_squared,
        bank.projection[0],bank.projection[8],mul(float(viewport[1]),.5f),mul(float(viewport[3]),.5f));
    const auto v=axis(vertical,depth,radius,z_squared,radius_squared,
        bank.projection[5],bank.projection[9],mul(float(viewport[0]),.5f),mul(float(viewport[2]),.5f));
    return {h[0],h[1],v[0],v[1]};
}
}
