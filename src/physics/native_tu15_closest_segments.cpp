#include "halo2/native_tu15_closest_segments.hpp"
#include <cstring>
#include <stdexcept>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif
namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory&memory_;const SourcePhysicsContactRecordObserver&observe_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):memory_(m),observe_(o){}
    std::uint32_t word(std::uint32_t p)const{std::array<std::byte,4>b{};memory_.read(p,b);std::uint32_t n{};for(unsigned k=0;k<4;++k)n|=std::to_integer<unsigned>(b[k])<<(k*8u);return n;}
    void bytes(std::uint32_t pc,std::uint32_t p,std::span<const std::byte>b){SourcePhysicsContactRecordStore event{pc,p,{},{}};if(observe_){event.before.resize(b.size());memory_.read(p,event.before);event.written.assign(b.begin(),b.end());}memory_.write(p,b);if(observe_)observe_(event);}
    void word(std::uint32_t pc,std::uint32_t p,std::uint32_t n){std::array<std::byte,4>b{};for(unsigned k=0;k<4;++k)b[k]=std::byte(n>>(k*8u));bytes(pc,p,b);}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    __m128 vector(std::uint32_t p)const{if(p&15u)throw std::runtime_error("Native TU1.5 closest segments require aligned input DATA");std::array<std::byte,16>b{};memory_.read_vector(p,b);__m128 value;std::memcpy(&value,b.data(),16);return value;}
    void vector(std::uint32_t pc,std::uint32_t p,__m128 value){if(p&15u)throw std::runtime_error("Native TU1.5 closest segments require aligned output");std::array<std::byte,16>b{};std::memcpy(b.data(),&value,16);bytes(pc,p,b);}
    void scalar(std::uint32_t pc,std::uint32_t p,bool pop){std::uint32_t value;if(pop)asm volatile("fstps %0":"=m"(value)::"memory","st");else asm volatile("fsts %0":"=m"(value)::"memory");word(pc,p,value);}
#endif
};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Environment {
    alignas(16)std::array<std::byte,108>host_{};std::array<std::byte,28>native_{};NativeTu15PhysicsFrameFp&fp_;unsigned previous_;
public:
    Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth):fp_(fp),previous_(_mm_getcsr()){
        if(depth>6||(fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||(fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))throw std::runtime_error("Native TU1.5 closest segments require their unsupported FP owner");asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");std::memcpy(native_.data(),host_.data(),28);const std::uint16_t empty=0xffff;std::memcpy(native_.data(),&fp.control_word,2);std::memcpy(native_.data()+4,&fp.status,2);std::memcpy(native_.data()+8,&empty,2);asm volatile("fldenv %0"::"m"(native_):"memory");_mm_setcsr(fp.mxcsr);
    }
    ~Environment(){asm volatile("fnstcw %0":"=m"(fp_.control_word));asm volatile("fnstsw %0":"=a"(fp_.status));fp_.mxcsr=_mm_getcsr();asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(previous_);}
};
__m128 multiply(__m128 left,__m128 right){asm volatile("mulps %1,%0":"+x"(left):"x"(right):"memory");return left;}
__m128 add(__m128 left,__m128 right){asm volatile("addps %1,%0":"+x"(left):"x"(right):"memory");return left;}
__m128 subtract(__m128 left,__m128 right){asm volatile("subps %1,%0":"+x"(left):"x"(right):"memory");return left;}
__m128 add_scalar(__m128 left,__m128 right){asm volatile("addss %1,%0":"+x"(left):"x"(right):"memory");return left;}
std::uint32_t low(__m128 value){std::uint32_t n;std::memcpy(&n,&value,4);return n;}
__m128 broadcast(std::uint32_t n){__m128 value{};std::memcpy(&value,&n,4);return _mm_shuffle_ps(value,value,0);}
std::uint32_t dot(__m128 left,__m128 right){const auto product=multiply(left,right),xy=add_scalar(_mm_shuffle_ps(product,product,0x55),product),sum=add_scalar(_mm_shuffle_ps(product,product,0xaa),xy);return low(sum);}
void push(std::uint32_t value){asm volatile("flds %0"::"m"(value):"memory","st");}
void mul(std::uint32_t value){asm volatile("fmuls %0"::"m"(value):"memory","st");}
void plus(std::uint32_t value){asm volatile("fadds %0"::"m"(value):"memory","st");}
void minus(std::uint32_t value){asm volatile("fsubs %0"::"m"(value):"memory","st");}
void divide(std::uint32_t value){asm volatile("fdivs %0"::"m"(value):"memory","st");}
std::uint16_t compare_pop(std::uint32_t value){std::uint16_t status;asm volatile("fcomps %1\n\tfnstsw %0":"=a"(status):"m"(value):"memory","st");return status;}
std::uint16_t compare_two_pop(){std::uint16_t status;asm volatile("fcompp\n\tfnstsw %0":"=a"(status)::"memory","st","st(1)");return status;}
bool parity41(std::uint16_t status){const auto bits=status&0x4100u;return bits==0||bits==0x4100u;}
bool parity5(std::uint16_t status){return(status&0x0500u)!=0x0100u;}
#endif
}
std::uint32_t native_tu15_closest_segment_pair(SourcePhysicsContactMemory&memory,
    std::uint32_t first_point,std::uint32_t first_direction,
    std::uint32_t second_point,std::uint32_t second_direction,std::uint32_t output,
    NativeTu15ClosestSegmentsScratch scratch,NativeTu15PhysicsFrameFp&fp,
    unsigned depth,const SourcePhysicsContactRecordObserver&observe){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    Memory m(memory,observe);Environment environment(fp,depth);const auto p=scratch.identity;
    const auto da=m.vector(first_direction),a=m.vector(first_point),b=m.vector(second_point),delta=subtract(b,a),db=m.vector(second_direction);
    const auto cross=dot(da,db);m.word(0x312666u,p+0x14u,cross);const auto along_a=dot(da,delta),along_b=dot(db,delta);m.word(0x3126a6u,p+0x28u,along_b);
    const auto length_a=dot(da,da),length_b=dot(db,db);m.word(0x3126dau,p+0x2cu,along_a);m.word(0x3126eeu,p+0x18u,length_b);
    push(m.word(p+0x18u));m.word(0x3126fau,p+0x1cu,length_a);mul(m.word(p+0x1cu));m.scalar(0x312702u,p+0x24u,true);
    push(m.word(p+0x14u));mul(m.word(p+0x14u));m.scalar(0x31270eu,p+0x20u,true);
    push(m.word(p+0x24u));minus(m.word(p+0x20u));asm volatile("fabs":::"memory","st");m.scalar(0x31271cu,p+0xcu,true);
    push(m.word(p+0x18u));mul(m.word(p+0x2cu));push(m.word(p+0x28u));mul(m.word(p+0x14u));
    // GNU AT&T FSUBRP spelling emits Intel FSUBP ST(1),ST(0).
    asm volatile("fsubrp %%st,%%st(1)":::"memory","st","st(1)");m.scalar(0x312732u,p+0x10u,false);mul(m.word(p+0xcu));push(m.word(p+0xcu));mul(m.word(p+0xcu));std::uint32_t flags{};
    if(!parity41(compare_two_pop())){push(m.word(0x46a0d0u));flags=1;}
    else{
        push(m.word(p+0x10u));if(!parity41(compare_pop(m.word(0x46a0e8u)))){push(m.word(0x46a0e8u));flags=2;}
        else{push(m.word(p+0x20u));plus(m.word(p+0x24u));mul(m.word(0x46a25cu));if(parity5(compare_pop(m.word(p+0xcu)))){push(m.word(0x46a0d0u));flags=1;}else{push(m.word(p+0x10u));divide(m.word(p+0xcu));}}
    }
    push(m.word(p+0x14u));asm volatile("fmul %%st(1),%%st(0)":::"memory","st","st(1)");minus(m.word(p+0x28u));m.scalar(0x3127a3u,p+0xcu,false);const auto b_upper=compare_pop(m.word(p+0x18u));bool recompute{};
    if(!(b_upper&0x0100u)){m.word(0x3127b2u,p+0xcu,0x3f800000u);flags=4;recompute=true;}
    else{push(m.word(p+0xcu));if(!parity41(compare_pop(m.word(0x46a0e8u)))){m.word(0x3127d2u,p+0xcu,0);flags=8;recompute=true;}}
    if(recompute){
        asm volatile("fstp %%st(0)":::"memory","st");push(m.word(p+0x14u));mul(m.word(p+0xcu));plus(m.word(p+0x2cu));m.scalar(0x3127edu,p+0x10u,false);
        if(!parity41(compare_pop(m.word(0x46a0e8u)))){push(m.word(0x46a0e8u));flags|=2;}
        else{push(m.word(p+0x10u));if(!(compare_pop(m.word(p+0x1cu))&0x0100u)){push(m.word(0x46a0d0u));flags|=1;}else{push(m.word(p+0x10u));divide(m.word(p+0x1cu));}}
    }else{push(m.word(p+0xcu));divide(m.word(p+0x18u));m.scalar(0x312835u,p+0xcu,true);}
    m.scalar(0x31283cu,p+0x2cu,true);const auto fraction_a=m.word(p+0x2cu),fraction_b=m.word(p+0xcu);const auto offset_a=multiply(broadcast(fraction_a),da),point_a=add(a,offset_a);
    m.word(0x312857u,p+0x2cu,fraction_b);const auto factor_b=m.word(p+0x2cu);m.vector(0x312861u,output,point_a);
    const auto db_now=m.vector(second_direction),b_now=m.vector(second_point),offset_b=multiply(broadcast(factor_b),db_now),point_b=add(b_now,offset_b),difference=subtract(point_a,point_b);m.vector(0x31287au,output+0x10u,difference);
    const auto final_delta=m.vector(output+0x10u);m.word(0x31289fu,p+0x2cu,dot(final_delta,final_delta));m.word(0x3128a8u,output+0x20u,m.word(p+0x2cu));return flags;
#else
    (void)memory;(void)first_point;(void)first_direction;(void)second_point;(void)second_direction;(void)output;(void)scratch;(void)fp;(void)depth;(void)observe;throw std::runtime_error("Exact native TU1.5 closest segments backend unavailable");
#endif
}
} // namespace halo2
