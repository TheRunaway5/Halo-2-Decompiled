#include "halo2/effect_light_scissor.hpp"
#include <bit>
#include <cmath>

namespace halo2 {
namespace {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
struct SourcePrecision {
    std::uint16_t previous{};
    SourcePrecision() {
        asm volatile("fnstcw %0":"=m"(previous));
        const std::uint16_t source=0x027f; // Original CRT math PC53, nearest.
        asm volatile("fldcw %0"::"m"(source):"memory");
    }
    ~SourcePrecision(){asm volatile("fldcw %0"::"m"(previous):"memory");}
};
long double projected_distance(float axis,float z_squared) {
    long double result;
    asm volatile("flds %1\n\tfld %%st(0)\n\tfmulp %%st,%%st(1)\n\tfadds %2\n\tfsqrt\n\tfstpt %0"
        :"=m"(result):"m"(axis),"m"(z_squared):"st","st(1)");
    return result;
}
float projected_arc(float radius,float distance) {
    float result;
    //1651CD division and37304C..373058. Keep the argument extended until
    // the original final float store; a float division here changes bounds.
    asm volatile("flds %1\n\tfdivs %2\n\tfld1\n\tfadd %%st(1),%%st\n\tfld1\n\tfsub %%st(2),%%st\n\tfmulp %%st,%%st(1)\n\tfsqrt\n\tfpatan\n\tfstps %0"
        :"=m"(result):"m"(radius),"m"(distance):"st","st(1)","st(2)");
    return result;
}
float projected_angle(float axis,float z) {
    float result;
    asm volatile("flds %1\n\tflds %2\n\tfchs\n\tfpatan\n\tfstps %0"
        :"=m"(result):"m"(axis),"m"(z):"st","st(1)");
    return result;
}
float projected_tangent(float angle) {
    float result;
    asm volatile("flds %1\n\tfptan\n\tfstp %%st(0)\n\tfstps %0"
        :"=m"(result):"m"(angle):"st","st(1)");
    return result;
}
#endif
void finite(float value){if(!std::isfinite(value))throw FormatError("source light projection has nonfinite input");}
float clamp(float value,float lo,float hi) {
    // Literal ordered SSE comparisons, including retained signed zero.
    if(lo>value)return lo;
    if(value>hi)return hi;
    return value;
}
}
std::optional<EffectLightSphereProjection> source_effect_light_sphere_projection(
    const Transform &view,Vec3 center,float radius) {
    finite(view.scale);finite(radius);
    if(radius<0)throw FormatError("source light projection has negative sphere radius");
    for(const auto &v:view.basis)for(float f:{v.x,v.y,v.z})finite(f);
    for(float f:{view.position.x,view.position.y,view.position.z,center.x,center.y,center.z})finite(f);
    if(view.scale!=1.f){center.x=view.scale*center.x;center.y=view.scale*center.y;center.z=view.scale*center.z;}
    const auto &a=view.basis;
    const std::array<float,3> p{
        ((a[2].x*center.z+a[1].x*center.y)+a[0].x*center.x)+view.position.x,
        ((a[2].y*center.z+a[1].y*center.y)+a[0].y*center.x)+view.position.y,
        ((a[2].z*center.z+a[1].z*center.y)+a[0].z*center.x)+view.position.z};
    if(p[2]>radius)return {};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    SourcePrecision precision;
    EffectLightSphereProjection result;
    const float z_squared=p[2]*p[2];
    constexpr float half_pi=std::bit_cast<float>(0x3fc90fdbu);
    constexpr float depth_cap=std::bit_cast<float>(0xb7800006u);
    for(unsigned axis=0;axis<2;++axis) {
        const auto distance=projected_distance(p[axis],z_squared);
        if(radius<distance) {
            const float arc=projected_arc(radius,float(distance));
            const float angle=projected_angle(p[axis],p[2]);
            const float low=angle-arc,high=angle+arc;
            result[axis*2]=low>=-half_pi?projected_tangent(low):-10000.f;
            result[axis*2+1]=high<=half_pi?projected_tangent(high):10000.f;
        } else {result[axis*2]=-10000.f;result[axis*2+1]=10000.f;}
    }
    const float low=p[2]-radius,high=p[2]+radius;
    result[4]=low<=depth_cap?low:depth_cap;
    result[5]=high<=depth_cap?high:depth_cap;
    return result;
#else
    throw FormatError("source light projection requires the original native x87 math backend");
#endif
}
EffectLightScissor source_effect_light_scissor(const EffectLightScissorInput &in) {
    for(float value:in.clipping_rectangle)finite(value);
    finite(in.near_distance);finite(in.far_distance);
    const auto &v=in.viewport;const auto &r=in.clipping_rectangle;
    EffectLightScissor out{{float(v[1]),float(v[3]),float(v[0]),float(v[2]),
        0.f-in.far_distance,0.f-in.near_distance},in.light_present};
    if(!in.light_present)return out;
    const auto projected=source_effect_light_sphere_projection(in.inverse_view,in.center,in.radius);
    if(!projected || r[1]==r[0] || r[3]==r[2])return out;
    const auto &p=*projected;
    if(r[1]<p[0] || p[1]<r[0] || r[3]<p[2] || p[3]<r[2])return out;
    const float x0=clamp(p[0],r[0],r[1]),x1=clamp(p[1],r[0],r[1]);
    const float y0=clamp(0.f-p[3],r[2],r[3]),y1=clamp(0.f-p[2],r[2],r[3]);
    const float width=r[1]-r[0],height=r[3]-r[2];
    out.bounds={float(v[3]-v[1])*(x0-r[0])/width+float(v[1]),
        float(v[3]-v[1])*(x1-r[0])/width+float(v[1]),
        float(v[2]-v[0])*(y0-r[2])/height+float(v[0]),
        float(v[2]-v[0])*(y1-r[2])/height+float(v[0]),p[4],p[5]};
    return out;
}
}
