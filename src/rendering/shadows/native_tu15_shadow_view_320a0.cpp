#include "halo2/native_tu15_shadow_view_320a0.hpp"
#include "halo2/detail/native_tu15_arithmetic_environment.hpp"
#include <bit>
#include <stdexcept>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <emmintrin.h>
#endif
namespace halo2 {
namespace {
std::uint32_t word(const SourcePhysicsContactMemory& memory,std::uint32_t at) {
    std::array<std::byte,4> b{};memory.read(at,b);std::uint32_t v{};
    for(unsigned n=0;n<4;++n)v|=std::to_integer<unsigned>(b[n])<<(8*n);return v;
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
struct Scalar {
    __m128 v;
    static Scalar raw(std::uint32_t v){return {_mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(v)))};}
    static Scalar value(float v){return raw(std::bit_cast<std::uint32_t>(v));}
    float value()const{return std::bit_cast<float>(std::bit_cast<std::uint32_t>(_mm_cvtsi128_si32(_mm_castps_si128(v))));}
};
Scalar operator+(Scalar a,Scalar b){asm volatile("addss %1,%0":"+x"(a.v):"x"(b.v):"memory");return a;}
Scalar operator-(Scalar a,Scalar b){asm volatile("subss %1,%0":"+x"(a.v):"x"(b.v):"memory");return a;}
Scalar operator*(Scalar a,Scalar b){asm volatile("mulss %1,%0":"+x"(a.v):"x"(b.v):"memory");return a;}
Scalar operator/(Scalar a,Scalar b){asm volatile("divss %1,%0":"+x"(a.v):"x"(b.v):"memory");return a;}
bool above(Scalar a,Scalar b){unsigned char v;asm volatile("comiss %2,%1; seta %0":"=qm"(v):"x"(a.v),"x"(b.v):"cc","memory");return v;}
bool below(Scalar a,Scalar b){unsigned char v;asm volatile("comiss %2,%1; setb %0":"=qm"(v):"x"(a.v),"x"(b.v):"cc","memory");return v;}
bool unequal(Scalar a,Scalar b){unsigned char v;asm volatile("ucomiss %2,%1; lahf; test $0x44,%%ah; setp %0":"=qm"(v):"x"(a.v),"x"(b.v):"ax","cc","memory");return v;}
using Vector=std::array<Scalar,3>;
Vector vector(Vec3 p){return {Scalar::value(p.x),Scalar::value(p.y),Scalar::value(p.z)};}
Vec3 value(const Vector& p){return {p[0].value(),p[1].value(),p[2].value()};}
Vector box_point(const Transform& t,Vector p,Scalar one) {
    const auto scale=Scalar::value(t.scale);
    if(unequal(scale,one))for(auto& v:p)v=scale*v;
    Vector out;
    for(unsigned axis=0;axis<3;++axis) {
        const auto x=vector(t.basis[0])[axis],y=vector(t.basis[1])[axis],z=vector(t.basis[2])[axis];
        out[axis]=((x*p[0]+z*p[2])+y*p[1])+vector(t.position)[axis];
    }
    return out;
}
#endif
}
SourceShadowViewVolume native_tu15_shadow_view_volume_320a0(const SourcePhysicsContactMemory& memory,
    const SourceFrustumCamera& camera,const SourceShadowViewBox& box,NativeTu15PhysicsFrameFp& fp) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    const auto one=Scalar::raw(word(memory,0x46a0d0)),negative_one=Scalar::raw(word(memory,0x46a0dc));
    const auto floor=Scalar::raw(word(memory,0x46a5a0)),epsilon=Scalar::raw(word(memory,0x46a30c));
    const auto projection_floor=Scalar::raw(word(memory,0x46a340));const auto empty=word(memory,0x474d40);
    std::array<Scalar,6> bounds;
    for(unsigned n=0;n<6;++n)bounds[n]=Scalar::raw(word(memory,empty+4*n));
    detail::NativeTu15ArithmeticEnvironment environment(fp);const auto zero=Scalar::raw(0);
    const auto h=vector(box.half_extent);const Vector negative{zero-h[0],zero-h[1],zero-h[2]};
    const std::array<Vector,8> corners{{{negative[0],negative[1],negative[2]},{h[0],negative[1],negative[2]},
        {negative[0],h[1],negative[2]},{h[0],h[1],negative[2]},{negative[0],negative[1],h[2]},
        {h[0],negative[1],h[2]},{negative[0],h[1],h[2]},{h[0],h[1],h[2]}}};
    constexpr std::array<unsigned,24> edges{0,2,2,3,3,1,1,0,0,4,1,5,2,6,3,7,4,5,5,7,7,6,6,4};
    SourceShadowViewVolume result;std::array<Vector,24> points;
    for(unsigned n=0;n<24;++n){points[n]=box_point(box.world_from_box,corners[edges[n]],one);result.world_endpoints[n]=value(points[n]);}
    const auto backward=vector(camera.camera_to_world.basis[2]),eye=vector(camera.full_frustum.eye);
    auto bias=Scalar::value(camera.full_frustum.metric_retained)+Scalar::value(camera.near_distance);
    if(above(floor,bias))bias=floor;
    const Vector normal{zero-backward[0],zero-backward[1],zero-backward[2]};
    const auto d=(zero-((eye[1]*backward[1]+eye[2]*backward[2])+eye[0]*backward[0]))+bias;
    std::array<bool,24> front;
    for(unsigned n=0;n<24;++n){const auto& p=points[n];
        const auto distance=n%4==0?((p[0]*normal[0]+p[2]*normal[2])+normal[1]*p[1])-d:
            ((p[1]*normal[1]+p[0]*normal[0])+p[2]*normal[2])-d;
        front[n]=above(distance,epsilon);
    }
    bool survived=false;
    for(unsigned n=0;n<24;n+=2) {
        if(!front[n]&&!front[n+1])continue;
        if(front[n]!=front[n+1]) {
            const auto p=points[n];const Vector delta{points[n+1][0]-p[0],points[n+1][1]-p[1],points[n+1][2]-p[2]};
            const auto denominator=(delta[0]*normal[0]+delta[2]*normal[2])+normal[1]*delta[1];
            const auto numerator=((normal[1]*p[1]+p[2]*normal[2])+normal[0]*p[0])-d;
            const auto t=unequal(denominator,zero)?numerator*(negative_one/denominator):zero;
            points[front[n]?n+1:n]={t*delta[0]+p[0],delta[1]*t+p[1],delta[2]*t+p[2]};
        }
        survived=true;
        for(unsigned j=n;j<n+2;++j) {
            auto p=points[j];const auto& t=camera.inverse_view;const auto scale=Scalar::value(t.scale);
            if(unequal(scale,one))for(auto& v:p)v=scale*v;
            Vector q;
            for(unsigned axis=0;axis<3;++axis)q[axis]=((vector(t.basis[2])[axis]*p[2]+
                vector(t.basis[1])[axis]*p[1])+vector(t.basis[0])[axis]*p[0])+vector(t.position)[axis];
            const auto denominator=above(q[2],projection_floor)?projection_floor:q[2];
            const auto inverse=negative_one/denominator;q[0]=q[0]*inverse;q[1]=q[1]*inverse;
            for(unsigned k=0;k<3;++k){if(above(bounds[k*2],q[k]))bounds[k*2]=q[k];if(above(q[k],bounds[k*2+1]))bounds[k*2+1]=q[k];}
        }
    }
    for(unsigned n=0;n<6;++n)result.projected_bounds[n]=bounds[n].value();
    const auto& rectangle=camera.full_frustum.rectangle;
    // The original compares the rectangle even when no edge survived. JB
    // rejects unordered; COMISS still publishes its real invalid exception.
    result.admitted=!(below(bounds[1],Scalar::value(rectangle[0]))||below(Scalar::value(rectangle[1]),bounds[0])||
        below(bounds[3],Scalar::value(rectangle[2]))||below(Scalar::value(rectangle[3]),bounds[2]))&&survived;
    return result;
#else
    (void)memory;(void)camera;(void)box;(void)fp;
    throw std::runtime_error("Exact native TU1.5 shadow view backend unavailable");
#endif
}
}
