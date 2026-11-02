#include "halo2/native_tu15_constraint_motion_compare.hpp"
#include <bit>
#include <stdexcept>
#include <xmmintrin.h>
#include <emmintrin.h>
namespace halo2 {
namespace {
std::uint32_t word(const SourcePhysicsContactMemory&m,std::uint32_t p){std::array<std::byte,4>b{};m.read(p,b);std::uint32_t out{};for(unsigned n=0;n<4;++n)out|=std::to_integer<unsigned>(b[n])<<(n*8u);return out;}
class Environment {NativeTu15PhysicsFrameFp&fp_;unsigned prior_;
public:
    Environment(NativeTu15PhysicsFrameFp&fp):fp_(fp),prior_(_mm_getcsr()){
        if((fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))throw std::runtime_error("Native TU1.5 constraint motion comparison requires its unsupported SIMD trap owner");_mm_setcsr(fp.mxcsr);
    }
    ~Environment(){fp_.mxcsr=_mm_getcsr();_mm_setcsr(prior_);}
};
__m128 scalar(std::uint32_t v){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(v)));}
__m128 mul(__m128 a,__m128 b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 add(__m128 a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
bool greater(__m128 a,__m128 b){unsigned char out;asm volatile("comiss %2,%1; seta %0":"=qm"(out):"x"(a),"x"(b):"cc","memory");return out;}
}
bool native_tu15_second_constraint_body_moves_faster(const SourcePhysicsContactMemory&m,
 std::uint32_t first,std::uint32_t second,NativeTu15PhysicsFrameFp&fp,NativeTu15ConstraintMotionCompareStats*stats){
    if(stats)++stats->calls;if(second==0xffffffffu){if(stats)++stats->missing_second;return false;}Environment environment(fp);
    const auto manager=word(m,0x52be5cu),components=word(m,manager+0x44u),objects=word(m,0x4ecbd0u),headers=word(m,objects+0x44u),
        second_object=word(m,components+(second&0xffffu)*160u+8u),first_object=word(m,components+(first&0xffffu)*160u+8u),second_data=word(m,headers+(second_object&0xffffu)*12u+8u);
    const auto second_x=scalar(word(m,second_data+0x88u)),second_y=scalar(word(m,second_data+0x8cu)),second_z=scalar(word(m,second_data+0x90u));
    const auto second_xx=mul(second_x,second_x),second_yy=mul(second_y,second_y);
    const auto first_data=word(m,headers+(first_object&0xffffu)*12u+8u);const auto first_y=scalar(word(m,first_data+0x8cu)),first_x=scalar(word(m,first_data+0x88u)),first_z=scalar(word(m,first_data+0x90u));
    const auto second_zz=mul(second_z,second_z),second_squared=add(add(second_xx,second_yy),second_zz);
    const auto first_xx=mul(first_x,first_x),first_yy=mul(first_y,first_y),first_xy=add(first_xx,first_yy),first_zz=mul(first_z,first_z),first_squared=add(first_xy,first_zz);
    if(stats)++stats->comparisons;return greater(second_squared,first_squared);
}
} // namespace halo2
