#include "halo2/native_tu15_body_deactivation_policy.hpp"
#include <bit>
#include <cstring>
#include <stdexcept>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#include <emmintrin.h>
#endif
namespace halo2 {
namespace {
using Period=NativeTu15DeactivationPeriod;using Vector=NativeTu15PhysicsVector;
bool long_period(Period period){return period==Period::long_period;}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Environment {
    alignas(16)std::array<std::byte,108>host_{};std::array<std::byte,28>native_{};
    unsigned mxcsr_{};NativeTu15PhysicsFrameFp&fp_;
public:
    Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth):fp_(fp){
        if(depth>5||(fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||
           (fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))
            throw std::runtime_error("Native TU1.5 body policy requires its unsupported x87/SSE fault owner");
        mxcsr_=_mm_getcsr();asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        std::memcpy(native_.data(),host_.data(),28);const std::uint16_t empty=0xffff;
        std::memcpy(native_.data(),&fp.control_word,2);std::memcpy(native_.data()+4,&fp.status,2);std::memcpy(native_.data()+8,&empty,2);
        asm volatile("fldenv %0"::"m"(native_):"memory");_mm_setcsr(fp.mxcsr);
    }
    ~Environment(){
        asm volatile("fnstsw %0":"=a"(fp_.status));asm volatile("fnstcw %0":"=m"(fp_.control_word));fp_.mxcsr=_mm_getcsr();
        asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(mxcsr_);
    }
};
__m128 simd(const Vector&v){return _mm_castsi128_ps(_mm_set_epi32(std::bit_cast<std::int32_t>(v[3]),std::bit_cast<std::int32_t>(v[2]),std::bit_cast<std::int32_t>(v[1]),std::bit_cast<std::int32_t>(v[0])));}
std::uint32_t low(__m128 value){return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(value));}
__m128 subtract(__m128 a,__m128 b){asm volatile("subps %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 square(__m128 a){asm volatile("mulps %0,%0":"+x"(a)::"memory");return a;}
__m128 add(__m128 a,__m128 b){asm volatile("addps %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 add_low(__m128 a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
void threshold(std::uint32_t bits){asm volatile("flds %0"::"m"(bits):"memory","st");}
void duplicate_threshold(){asm volatile("fld %%st(0)":::"memory","st");}
void square_threshold(){asm volatile("fmul %%st(1),%%st":::"memory","st");}
void distance(std::uint32_t bits){asm volatile("flds %0"::"m"(bits):"memory","st");}
void compare_distance(){asm volatile("fcompp":::"memory","st");}
void discard_threshold(){asm volatile("fstp %%st(0)":::"memory","st");}
std::uint16_t status(){std::uint16_t value;asm volatile("fnstsw %0":"=a"(value)::"memory");return value;}
void scale_angular(std::uint32_t bits){asm volatile("fmuls %0"::"m"(bits):"memory","st");}
std::uint16_t compare_angular(){
    std::uint16_t value;
    asm volatile("fld %%st(1)\n\tfmul %%st(2),%%st\n\tfcompp\n\tfnstsw %0\n\tfstp %%st(0)":"=a"(value)::"memory","st");
    return value;
}
#endif
}
bool native_tu15_body_deactivation_policy(NativeTu15BodyDeactivationPolicyStorage&s,Period period,
    NativeTu15PhysicsFrameFp&fp,unsigned depth){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    Environment environment(fp,depth);
    threshold(s.distance_threshold(period));
    const auto motion=s.motion();duplicate_threshold();const auto position=simd(s.position(motion));square_threshold();
    const auto displacement=square(subtract(simd(s.previous_position(period)),position));
    const auto xy=add_low(_mm_shuffle_ps(displacement,displacement,0x55),displacement);
    const auto distance_squared=add_low(_mm_shuffle_ps(displacement,displacement,0xaa),xy);
    const auto previous_quaternion=simd(s.previous_quaternion(period));distance(low(distance_squared));
    const auto quaternion=s.quaternion(motion);compare_distance();
    const auto orientation_delta=square(subtract(previous_quaternion,simd(quaternion)));
    discard_threshold();
    auto angular=add(_mm_shuffle_ps(orientation_delta,orientation_delta,0x4e),orientation_delta);
    angular=add(angular,_mm_shuffle_ps(angular,angular,0xb1));
    // Original TEST AH,5; JP rejects greater/equal/unordered distance. The
    // source performs quaternion SIMD work before observing that comparison.
    const auto distance_status=status();
    bool stationary=(distance_status&0x0500u)==0x0100u;
    if(stationary){
        threshold(s.angular_threshold(period));distance(low(angular));scale_angular(s.angular_scale());
        const auto angular_status=compare_angular();stationary=(angular_status&0x4100u)==0;
    }
    if(!stationary){
        s.previous_quaternion(period,quaternion);
        // This actual re-read follows the first state publication. It can
        // observe aliased or externally mutable body/motion backing.
        const auto current=s.motion();s.previous_position(period,s.position(current));
    }
    s.result(period,stationary);return stationary;
#else
    (void)s;(void)period;(void)fp;(void)depth;throw std::runtime_error("Exact native TU1.5 body policy backend unavailable");
#endif
}
NativeTu15BodyDeactivationPolicyMemory::NativeTu15BodyDeactivationPolicyMemory(SourcePhysicsContactMemory&m,std::uint32_t policy,NativeTu15PhysicsBody body,std::uint32_t output,const SourcePhysicsContactRecordObserver&o):memory_(m),policy_(policy),body_(body.identity),output_(output),observer_(o){}
std::uint32_t NativeTu15BodyDeactivationPolicyMemory::scalar(std::uint32_t p)const{std::array<std::byte,4>b{};memory_.read(p,b);std::uint32_t n{};for(unsigned i=0;i<4;++i)n|=std::to_integer<unsigned>(b[i])<<(8u*i);return n;}
Vector NativeTu15BodyDeactivationPolicyMemory::vector(std::uint32_t p)const{std::array<std::byte,16>b{};memory_.read_vector(p,b);Vector v{};for(unsigned k=0;k<4;++k)for(unsigned i=0;i<4;++i)v[k]|=std::to_integer<unsigned>(b[k*4u+i])<<(8u*i);return v;}
void NativeTu15BodyDeactivationPolicyMemory::scalar(std::uint32_t site,std::uint32_t p,std::uint32_t n){const std::array b{std::byte(n)};SourcePhysicsContactRecordStore store{site,p,{},{}};if(observer_){store.before.resize(1);memory_.read(p,store.before);store.written.assign(b.begin(),b.end());}memory_.write(p,b);if(observer_)observer_(store);}
void NativeTu15BodyDeactivationPolicyMemory::vector(std::uint32_t site,std::uint32_t p,const Vector&v){std::array<std::byte,16>b{};for(unsigned k=0;k<4;++k)for(unsigned i=0;i<4;++i)b[k*4u+i]=std::byte(v[k]>>(8u*i));SourcePhysicsContactRecordStore store{site,p,{},{}};if(observer_){store.before.resize(16);memory_.read(p,store.before);store.written.assign(b.begin(),b.end());}memory_.write(p,b);if(observer_)observer_(store);}
std::uint32_t NativeTu15BodyDeactivationPolicyMemory::distance_threshold(Period p)const{return scalar(policy_+(long_period(p)?0x58u:0x50u));}
NativeTu15PhysicsMotion NativeTu15BodyDeactivationPolicyMemory::motion()const{return{scalar(body_+0x3cu)};}
Vector NativeTu15BodyDeactivationPolicyMemory::position(NativeTu15PhysicsMotion m)const{return vector(m.identity+0xb0u);}
Vector NativeTu15BodyDeactivationPolicyMemory::previous_position(Period p)const{return vector(policy_+(long_period(p)?0x30u:0x10u));}
Vector NativeTu15BodyDeactivationPolicyMemory::previous_quaternion(Period p)const{return vector(policy_+(long_period(p)?0x40u:0x20u));}
Vector NativeTu15BodyDeactivationPolicyMemory::quaternion(NativeTu15PhysicsMotion m)const{return vector(m.identity+0x30u);}
std::uint32_t NativeTu15BodyDeactivationPolicyMemory::angular_threshold(Period p)const{return scalar(policy_+(long_period(p)?0x5cu:0x54u));}
std::uint32_t NativeTu15BodyDeactivationPolicyMemory::angular_scale()const{return scalar(policy_+8u);}
void NativeTu15BodyDeactivationPolicyMemory::previous_position(Period p,Vector v){vector(long_period(p)?0x31e746u:0x31e676u,policy_+(long_period(p)?0x30u:0x10u),v);}
void NativeTu15BodyDeactivationPolicyMemory::previous_quaternion(Period p,Vector v){vector(long_period(p)?0x31e738u:0x31e668u,policy_+(long_period(p)?0x40u:0x20u),v);}
void NativeTu15BodyDeactivationPolicyMemory::result(Period p,bool value){scalar(long_period(p)?(value?0x31e72bu:0x31e74au):(value?0x31e65bu:0x31e67au),output_,value);}
} // namespace halo2
