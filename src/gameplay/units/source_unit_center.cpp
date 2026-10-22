#include "halo2/source_unit_center.hpp"
#include "halo2/kernels.hpp"
#include <bit>
#include <cfenv>
#include <cmath>
#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
#include <xmmintrin.h>
#endif

namespace halo2 {
namespace {
void arithmetic() {
    if(std::fegetround()!=FE_TONEAREST)
        throw FormatError("source unit center requires nearest arithmetic");
#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
    if(_mm_getcsr()&0xe040u)
        throw FormatError("source unit center requires nearest non-FTZ/non-DAZ SSE arithmetic");
#endif
}
void finite(float f) { if(!std::isfinite(f))throw FormatError("nonfinite source unit center operand"); }
void finite(Vec3 v) {finite(v.x);finite(v.y);finite(v.z);}
template<class F,class... A> decltype(auto) service(const F &f,A... a) {
    if(!f)throw FormatError("required original unit center service unavailable");
    return f(a...);
}
const SourceUnitEyeSeat &seat(const SourceUnitEyeDefinition &d,std::int16_t index) {
    if(index<0 || std::size_t(index)>=d.seats.size())throw FormatError("invalid source unit eye seat");
    return d.seats[index];
}
bool normalize(Vec2 &v) {
    const double x=v.x,y=v.y;
    const double magnitude=std::sqrt(x*x+y*y);
    if(std::abs(magnitude)<double(.0001f))return false;
    const float rounded=float(magnitude),inverse=1.f/rounded;
    v.x*=inverse;v.y*=inverse;
    return rounded!=0.f;
}
float pitch(float z) {
    constexpr float half_pi=std::bit_cast<float>(0x3fc90fdbu);
    if(-1.f>=z)return half_pi;
    if(!(0.f>z))return 0.f;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    std::uint16_t previous{};
    asm volatile("fnstcw %0":"=m"(previous));
    const std::uint16_t source=0x027f;
    float result;
    //322BD8/322BF5 keep ACOS in x87 through DF41D's subtraction. Rounding
    // ACOS to float first changes the corrected position near curve steps.
    asm volatile("fldcw %1\n\tflds %2\n\tfchs\n\tfld1\n\tfadd %%st(1)\n\t"
        "fld1\n\tfsub %%st(2)\n\tfmulp %%st,%%st(1)\n\tfsqrt\n\t"
        "fxch %%st(1)\n\tfpatan\n\tfsubrs %3\n\tfstps %0\n\tfldcw %4"
        :"=m"(result):"m"(source),"m"(z),"m"(half_pi),"m"(previous)
        :"st","st(1)","st(2)","memory");
    return result;
#else
    throw FormatError("original x87 unit camera pitch kernel unavailable");
#endif
}
float toward(float current,float target) {
    float delta=target-current;
    if(-.01f>delta)delta=-.01f;
    else if(delta>.01f)delta=.01f;
    return current+delta;
}
float dot(Vec3 a,Vec3 b) { return (a.z*b.z+a.y*b.y)+a.x*b.x; }
}
SourceUnitHorizontalBasis source_unit_horizontal_basis(Vec3 forward,Vec3 up) {
    arithmetic();finite(forward);finite(up);
    Vec2 f{forward.x,forward.y};
    if(!normalize(f)) {
        f={up.x,up.y};
        if(forward.z>0.f)f={0.f-f.x,0.f-f.y};
        if(!normalize(f))f={1.f,0.f}; // Actual *468778 ->440B30.
    }
    return {f,{0.f-f.y,f.x}};
}
Vec3 source_unit_perpendicular_up(Vec3 forward) {
    arithmetic();finite(forward);
    const double x=forward.x,y=forward.y;
    const double magnitude=std::sqrt(x*x+y*y);
    Vec3 side{1.f,0.f,0.f}; // Actual *4687A8 ->440B30.
    if(!(std::abs(magnitude)<double(.0001f))) {
        const float rounded=float(magnitude),inverse=1.f/rounded;
        side={inverse*forward.y,inverse*(-forward.x),inverse*0.f};
        if(rounded==0.f)side={1.f,0.f,0.f};
    }
    return {forward.z*side.y-side.z*forward.y,
        forward.x*side.z-forward.z*side.x,
        side.x*forward.y-forward.x*side.y};
}
void source_update_unit_head_offset(SourceUnitEyeState &u,const SourceUnitEyeServices &s) {
    if(u.player==0xffffffffu)return;
    if(source_unit_head_offset_suppressed(u.offset_channels))return;
    arithmetic();finite(u.forward);finite(u.up);finite(u.node_zero_position);finite(u.head.local);
    auto marker=service(s.marker,u.object,0x0b0000ddu);
    if(!marker.count)marker=service(s.marker,u.object,0x04000095u);
    finite(marker.position);
    const Vec3 offset{marker.position.x-u.node_zero_position.x,
        marker.position.y-u.node_zero_position.y,marker.position.z-u.node_zero_position.z};
    const auto aim=service(s.clamp_aim,u.object,u.forward);
    const auto basis=source_unit_horizontal_basis(aim,u.up);
    const Vec3 local{basis.forward.y*offset.y+basis.forward.x*offset.x,
        basis.left.y*offset.y+basis.left.x*offset.x,offset.z};
    if(u.head.initialized)u.head.local={toward(u.head.local.x,local.x),
        toward(u.head.local.y,local.y),toward(u.head.local.z,local.z)};
    else {u.head.local=local;u.head.initialized=1;}
}
bool source_unit_head_offset_suppressed(std::span<const SourceUnitHeadOffsetChannel,2> channels) {
    for(const auto &c:channels)
        if(c.datum!=0xffffffffu && c.index!=-1 && (c.flags&1u) && !(c.state&9u))return true;
    return false;
}
Vec3 source_unit_observer_position(const SourceUnitEyeState &u,Vec3 position,
    Vec3 forward,Vec3 up,std::span<const std::uint8_t,1024> curve) {
    arithmetic();finite(position);finite(forward);finite(up);
    const auto &d=u.definition;finite(d.pitch_minimum);finite(d.pitch_maximum);
    if(!(d.pitch_maximum>d.pitch_minimum))return position;
    const float range=d.pitch_maximum-d.pitch_minimum;
    float fraction=(pitch(forward.z)-d.pitch_minimum)/range;
    if(0.f>fraction)fraction=0.f;
    else if(fraction>1.f)fraction=1.f;
    fraction=fade_curve_fraction(fraction,curve);
    if(!(fraction>0.f) || !u.head.initialized)return position;
    finite(d.correction_scale);finite(d.minimum_forward);finite(u.head.local);finite(u.node_zero_position);
    const auto b=source_unit_horizontal_basis(forward,up);
    const Vec3 offset{position.x-u.node_zero_position.x,position.y-u.node_zero_position.y,
        position.z-u.node_zero_position.z};
    const float local_x=b.forward.y*offset.y+b.forward.x*offset.x;
    const float local_y=b.left.y*offset.y+b.left.x*offset.x;
    const float target_x=u.head.local.x>d.minimum_forward?u.head.local.x:d.minimum_forward;
    const float x=(d.correction_scale.x*(target_x-local_x))*fraction;
    const float y=(d.correction_scale.y*(u.head.local.y-local_y))*fraction;
    const float z=(d.correction_scale.z*(u.head.local.z-offset.z))*fraction;
    return {(b.left.x*y+b.forward.x*x)+position.x,
        (b.left.y*y+b.forward.y*x)+position.y,position.z+z};
}
Vec3 source_unit_center(const SourceUnitEyeState &u,const SourceUnitEyeServices &s,
    std::span<const std::uint8_t,1024> curve) {
    if(u.parent!=0xffffffffu) {
        auto position=service(s.attachment_position,u.parent);
        if(u.seat!=-1) {
            const auto &parent=service(s.object,u.parent);
            const auto marker=seat(parent.definition,u.seat).camera_marker;
            if(parent.type!=1 || marker)position=service(s.marker,u.parent,marker).position;
        }
        finite(position);return position;
    }
    if(u.type || (u.flags10a&4u)) {
        std::uint32_t name=0x04000095u;
        if(u.related!=0xffffffffu) {
            const auto &related=service(s.object,u.related);
            if(related.seat!=-1)name=seat(u.definition,related.seat).marker;
        }
        const auto position=service(s.marker,u.object,name).position;
        finite(position);return position;
    }
    arithmetic();finite(u.crouch);finite(u.scale);finite(u.definition.standing_height);
    finite(u.definition.crouching_height);
    const auto root=service(s.root,u.object);finite(root);
    const float crouch=u.state3dc==6 && u.flag3f8 && (u.flags348&0x2000u)?0.f:u.crouch;
    const auto base=ordinary_unit_eye(root,u.definition.standing_height,u.definition.crouching_height,crouch,u.scale);
    const auto origin=source_unit_observer_position(u,base,u.forward,source_unit_perpendicular_up(u.forward),curve);
    Vec3 offset{origin.x-base.x,origin.y-base.y,origin.z-base.z};
    if(!(original_vector_normalize(offset)>0.f))return base;
    const Vec3 reverse{offset.x*-.25f,offset.y*-.25f,offset.z*-.25f};
    const auto first=service(s.contact,base,reverse);finite(first.point);finite(first.fraction);
    Vec3 point=first.point;
    if(first.hit) {
        const float t=first.fraction*.9f;
        point={reverse.x*t+base.x,reverse.y*t+base.y,reverse.z*t+base.z};
    }
    const auto second=service(s.contact,point,offset);finite(second.point);finite(second.fraction);
    if(second.hit) {
        const Vec3 to_origin{origin.x-point.x,origin.y-point.y,origin.z-point.z};
        const Vec3 to_contact{second.point.x-point.x,second.point.y-point.y,second.point.z-point.z};
        if(dot(to_origin,offset)+.05f>dot(to_contact,offset))
            return {second.point.x-offset.x*.05f,second.point.y-offset.y*.05f,second.point.z-offset.z*.05f};
    }
    return origin;
}
}
