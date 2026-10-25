#include "halo2/source_objective_presentation_tu15.hpp"
#include <bit>
#include <cfenv>
#include <cmath>
#include <stdexcept>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif
namespace halo2 {
SourceObjectiveGoalPolygonTu15 source_objective_goal_polygon_tu15(Vec3 center,float radius,
    SourceKingBoundaryFloatingPoint fp) {
    for(float value:{center.x,center.y,center.z,radius})if(!std::isfinite(value))
        throw std::invalid_argument("Original goal circle requires finite authored coordinates");
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if((fp.x87_control_word&0x3f)!=0x3f || ((fp.x87_control_word>>8)&3u)==1u ||
        (fp.mxcsr&0x1f80)!=0x1f80)throw std::invalid_argument("Unsupported original goal floating-point owner");
    struct Scope {
        std::fenv_t saved;unsigned csr;
        explicit Scope(SourceKingBoundaryFloatingPoint fp):csr(_mm_getcsr()) {
            if(std::fegetenv(&saved))throw std::runtime_error("Goal floating-point environment unavailable");
            asm volatile("fldcw %0"::"m"(fp.x87_control_word):"memory");_mm_setcsr(fp.mxcsr);
        }
        ~Scope(){std::fesetenv(&saved);_mm_setcsr(csr);}
    } scope(fp);
    SourceObjectiveGoalPolygonTu15 out;constexpr float step=std::bit_cast<float>(0x3e490fdbu);
    const auto x=_mm_set_ss(center.x),y=_mm_set_ss(center.y),r=_mm_set_ss(radius);
    const auto scalar=[](__m128 value){return _mm_cvtss_f32(value);};
    for(std::int32_t i=0;i<8;++i) {
        float sine,cosine;
        asm volatile("fildl %2; fmuls %3; fld %%st(0); fsin; fstps %0; fcos; fstps %1"
            :"=m"(sine),"=m"(cosine):"m"(i),"m"(step):"st","st(1)");
        const auto a=_mm_mul_ss(_mm_set_ss(sine),r),b=_mm_mul_ss(_mm_set_ss(cosine),r);
        out.points[unsigned(i)]={scalar(_mm_add_ss(a,x)),scalar(_mm_add_ss(b,y))};
        out.points[unsigned(i)+8]={scalar(_mm_add_ss(b,x)),scalar(_mm_sub_ss(y,a))};
        out.points[unsigned(i)+16]={scalar(_mm_sub_ss(x,a)),scalar(_mm_sub_ss(y,b))};
        out.points[unsigned(i)+24]={scalar(_mm_sub_ss(x,b)),scalar(_mm_add_ss(a,y))};
    }
    constexpr float segments=32.f;
    asm volatile("flds %1; fsubs %3; flds %2; fsubs %4; fld %%st(0); fmul %%st(1),%%st; "
        "fld %%st(2); fmul %%st(3),%%st; faddp %%st,%%st(1); fsqrt; fstp %%st(2); fstp %%st(0); fmuls %5; fstps %0"
        :"=m"(out.perimeter):"m"(out.points[1][0]),"m"(out.points[1][1]),"m"(out.points[0][0]),"m"(out.points[0][1]),"m"(segments)
        :"st","st(1)","st(2)","st(3)");
    return out;
#else
    (void)fp;throw std::runtime_error("Original goal circle requires the verified x87/SSE native backend");
#endif
}
std::optional<SourceKingBoundary> source_objective_goal_boundary_tu15(
    const SourceObjectiveGoalTu15& goal,Vec3 eye,std::int32_t tick,std::int16_t rate,SourceKingBoundaryFloatingPoint fp) {
    const auto polygon=source_objective_goal_polygon_tu15(goal.center,goal.radius,fp);
    return source_polygon_boundary(polygon.points,{goal.center.x,goal.center.y,goal.center.z},goal.radius,
        polygon.perimeter,std::bit_cast<float>(0x3ecccccdu),goal.distance_scale,
        {eye.x,eye.y,eye.z},{goal.color.x,goal.color.y,goal.color.z},tick,rate,fp);
}
} // namespace halo2
