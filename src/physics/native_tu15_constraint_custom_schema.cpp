#include "halo2/native_tu15_constraint_custom_schema.hpp"
#include <bit>
#include <cstring>
#include <stdexcept>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#include <emmintrin.h>
#endif
namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory&memory_;const SourcePhysicsContactRecordObserver&observer_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):memory_(m),observer_(o){}
    std::uint32_t word(std::uint32_t p)const{std::array<std::byte,4>b{};memory_.read(p,b);std::uint32_t value{};for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(b[n])<<(n*8u);return value;}
    void word(std::uint32_t site,std::uint32_t p,std::uint32_t value){std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(value>>(n*8u));SourcePhysicsContactRecordStore store{site,p,{},{}};if(observer_){store.before.resize(4);memory_.read(p,store.before);store.written.assign(b.begin(),b.end());}memory_.write(p,b);if(observer_)observer_(store);}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    void pop_float(std::uint32_t site,std::uint32_t p){
        SourcePhysicsContactRecordStore store{site,p,{},{}};if(observer_){store.before.resize(4);memory_.read(p,store.before);}
        // A rejected destination must leave the actual FSTP uncommitted.
        // Restore its real pre-instruction x87 state on write rejection;
        // observer delivery occurs after the successful store/pop commit.
        alignas(16)std::array<std::byte,108>prior{};std::uint32_t value;
        asm volatile("fnsave %0\n\tfrstor %0\n\tfstps %1":"=m"(prior),"=m"(value)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(value>>(n*8u));
        try{memory_.write(p,b);}catch(...){asm volatile("frstor %0"::"m"(prior):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");throw;}
        if(observer_){store.written.assign(b.begin(),b.end());observer_(store);}
    }
#endif
};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Environment {
    alignas(16)std::array<std::byte,108>host_{};std::array<std::byte,28>native_{};unsigned host_mxcsr_{};NativeTu15PhysicsFrameFp&fp_;
public:
    Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth):fp_(fp){
        if(depth>7||(fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||(fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))throw std::runtime_error("Native TU1.5 custom constraint schema requires its unsupported FP owner");
        host_mxcsr_=_mm_getcsr();asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");std::memcpy(native_.data(),host_.data(),28);const std::uint16_t empty=0xffff;std::memcpy(native_.data(),&fp.control_word,2);std::memcpy(native_.data()+4,&fp.status,2);std::memcpy(native_.data()+8,&empty,2);asm volatile("fldenv %0"::"m"(native_):"memory");_mm_setcsr(fp.mxcsr);
    }
    ~Environment(){asm volatile("fnstsw %0":"=a"(fp_.status));asm volatile("fnstcw %0":"=m"(fp_.control_word));fp_.mxcsr=_mm_getcsr();asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(host_mxcsr_);}
};
void push(std::uint32_t value){asm volatile("flds %0"::"m"(value):"memory","st");}
void multiply(std::uint32_t value){asm volatile("fmuls %0"::"m"(value):"memory","st");}
void add(std::uint32_t value){asm volatile("fadds %0"::"m"(value):"memory","st");}
using Vector=std::array<std::uint32_t,4>;
__m128 simd(const Vector&v){return _mm_castsi128_ps(_mm_set_epi32(std::bit_cast<std::int32_t>(v[3]),std::bit_cast<std::int32_t>(v[2]),std::bit_cast<std::int32_t>(v[1]),std::bit_cast<std::int32_t>(v[0])));}
__m128 multiply(__m128 a,__m128 b){asm volatile("mulps %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 add_low(__m128 a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
std::uint32_t low(__m128 value){return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(value));}
void scale(Memory&m,std::uint32_t payload){
    constexpr std::array<std::uint32_t,8>sites{0x31f3adu,0x31f3b9u,0x31f3c5u,0x31f3d1u,0x31f3ddu,0x31f3e9u,0x31f3f5u,0x31f401u};
    for(unsigned n=0;n<8;++n){const auto body=m.word(payload+(n<4?0x28u:0x2cu));const auto p=body+0x30u+(n%4u)*4u;const auto factor=payload+(n<4?0x20u:0x24u);if(n==2){push(m.word(p));multiply(m.word(factor));}else{push(m.word(factor));multiply(m.word(p));}m.pop_float(sites[n],p);}
}
void restore(Memory&m,std::uint32_t payload){
    constexpr std::array<std::uint32_t,8>sites{0x31f419u,0x31f422u,0x31f42bu,0x31f434u,0x31f43du,0x31f446u,0x31f44fu,0x31f458u};
    for(unsigned n=0;n<8;++n){const auto body=m.word(payload+(n<4?0x28u:0x2cu));const auto value=m.word(payload+n*4u);m.word(sites[n],body+0x30u+(n%4u)*4u,value);}
}
void add_bias(Memory&m,std::uint32_t payload){
    Vector input{},first{},second{};
    input[0]=m.word(payload);const auto row=m.word(payload+0x10u);input[1]=m.word(payload+4u);input[2]=m.word(payload+8u);input[3]=m.word(payload+0xcu);
    first[0]=m.word(row);first[1]=m.word(row+4u);first[2]=m.word(row+8u);first[3]=m.word(row+0xcu);second[0]=m.word(row+0x30u);second[1]=m.word(row+0x34u);
    const auto packed=simd(input);const auto first_product=multiply(simd(first),packed);auto first_xy=add_low(_mm_shuffle_ps(first_product,first_product,0x55),first_product);
    second[2]=m.word(row+0x38u);const auto first_xyz=add_low(_mm_shuffle_ps(first_product,first_product,0xaa),first_xy);second[3]=m.word(row+0x3cu);
    const auto second_product=multiply(simd(second),packed);push(low(first_xyz));add(m.word(row+0xcu));m.pop_float(0x31f605u,row+0xcu);
    const auto second_xy=add_low(_mm_shuffle_ps(second_product,second_product,0x55),second_product);const auto second_xyz=add_low(_mm_shuffle_ps(second_product,second_product,0xaa),second_xy);
    push(low(second_xyz));add(m.word(row+0x3cu));m.pop_float(0x31f623u,row+0x3cu);
}
#endif
}
bool native_tu15_owns_constraint_custom_schema(NativeTu15PhysicsVirtualOwner owner){return owner.identity==0x31f3a0u||owner.identity==0x31f410u||owner.identity==0x31f560u;}
void native_tu15_apply_constraint_custom_schema(SourcePhysicsContactMemory&memory,NativeTu15PhysicsVirtualOwner owner,std::uint32_t row,std::uint32_t payload,NativeTu15PhysicsFrameFp&fp,unsigned depth,const SourcePhysicsContactRecordObserver&observer,NativeTu15ConstraintCustomSchemaStats*stats){
    (void)row;if(!native_tu15_owns_constraint_custom_schema(owner))throw std::runtime_error("Native TU1.5 constraint custom schema owner is not authenticated");
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    Environment environment(fp,depth);Memory m(memory,observer);const auto kind=owner.identity==0x31f3a0u?0u:owner.identity==0x31f410u?1u:2u;if(stats){++stats->calls;++stats->owners[kind];}if(kind==0)scale(m,payload);else if(kind==1)restore(m,payload);else add_bias(m,payload);
#else
    (void)memory;(void)payload;(void)fp;(void)depth;(void)observer;(void)stats;throw std::runtime_error("Exact native TU1.5 constraint custom schema backend unavailable");
#endif
}
} // namespace halo2
