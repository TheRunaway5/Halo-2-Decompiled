#include "halo2/native_tu15_contact_shape.hpp"
#include <bit>
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
    std::uint32_t word(std::uint32_t p,unsigned size=4)const{std::array<std::byte,4>b{};memory_.read(p,std::span(b).first(size));std::uint32_t n{};for(unsigned k=0;k<size;++k)n|=std::to_integer<unsigned>(b[k])<<(k*8u);return n;}
    void bytes(std::uint32_t pc,std::uint32_t p,std::span<const std::byte>b){SourcePhysicsContactRecordStore event{pc,p,{},{}};if(observe_){event.before.resize(b.size());memory_.read(p,event.before);event.written.assign(b.begin(),b.end());}memory_.write(p,b);if(observe_)observe_(event);}
    void word(std::uint32_t pc,std::uint32_t p,std::uint32_t n){std::array<std::byte,4>b{};for(unsigned k=0;k<4;++k)b[k]=std::byte(n>>(k*8u));bytes(pc,p,b);}
    NativeTu15ContactVirtualOwner owner(std::uint32_t object,unsigned slot)const{return{word(word(object)+slot)};}
    std::array<std::byte,16>vector_bytes(std::uint32_t p)const{if(p&15u)throw std::runtime_error("Native TU1.5 support requires aligned vector DATA");std::array<std::byte,16>b{};memory_.read_vector(p,b);return b;}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    void pop_float(std::uint32_t pc,std::uint32_t p){
        SourcePhysicsContactRecordStore event{pc,p,{},{}};
        if(observe_){event.before.resize(4);memory_.read(p,event.before);}
        alignas(16)std::array<std::byte,108>before{};std::uint32_t bits;
        asm volatile("fnsave %0;frstor %0":"=m"(before)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        asm volatile("fstps %0":"=m"(bits)::"memory","st");
        std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(bits>>(8u*n));
        try{memory_.write(p,b);}catch(...){asm volatile("frstor %0"::"m"(before):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");throw;}
        if(observe_){event.written.assign(b.begin(),b.end());observe_(event);}
    }
    __m128 vector(std::uint32_t p)const{const auto b=vector_bytes(p);__m128 value;std::memcpy(&value,b.data(),16);return value;}
    void vector(std::uint32_t pc,std::uint32_t p,__m128 value){if(p&15u)throw std::runtime_error("Native TU1.5 support requires aligned vector DATA");std::array<std::byte,16>b{};std::memcpy(b.data(),&value,16);bytes(pc,p,b);}
#endif
};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Environment {
    alignas(16)std::array<std::byte,108>host_{};std::array<std::byte,28>native_{};
    NativeTu15PhysicsFrameFp&fp_;unsigned previous_;bool active_{};
    void validate()const{if((fp_.control_word&0x3fu)!=0x3fu||(fp_.status&0x8080u)||(fp_.mxcsr&0x1f80u)!=0x1f80u||(fp_.mxcsr&0xffff0000u))throw std::runtime_error("Native TU1.5 support requires its unsupported FP exception owner");}
public:
    Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth):fp_(fp),previous_(_mm_getcsr()){
        if(depth>7)throw std::runtime_error("Native TU1.5 support requires its overfull x87 owner");validate();asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");std::memcpy(native_.data(),host_.data(),28);resume();
    }
    ~Environment(){if(active_)publish();asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(previous_);}
    void publish(){asm volatile("fnstcw %0":"=m"(fp_.control_word));asm volatile("fnstsw %0":"=a"(fp_.status));fp_.mxcsr=_mm_getcsr();active_=false;}
    void resume(){validate();const std::uint16_t empty=0xffff;std::memcpy(native_.data(),&fp_.control_word,2);std::memcpy(native_.data()+4,&fp_.status,2);std::memcpy(native_.data()+8,&empty,2);asm volatile("fldenv %0"::"m"(native_):"memory");_mm_setcsr(fp_.mxcsr);active_=true;}
};
__m128 scalar(std::uint32_t n){__m128 value{};std::memcpy(&value,&n,4);return value;}
std::uint32_t low(__m128 value){std::uint32_t n;std::memcpy(&n,&value,4);return n;}
__m128 multiply(__m128 left,__m128 right){asm volatile("mulps %1,%0":"+x"(left):"x"(right):"memory");return left;}
__m128 add(__m128 left,__m128 right){asm volatile("addps %1,%0":"+x"(left):"x"(right):"memory");return left;}
__m128 subtract(__m128 left,__m128 right){asm volatile("subps %1,%0":"+x"(left):"x"(right):"memory");return left;}
__m128 add_scalar(__m128 left,__m128 right){asm volatile("addss %1,%0":"+x"(left):"x"(right):"memory");return left;}
unsigned improvements(__m128 best,__m128 dots){asm volatile("cmpltps %1,%0":"+x"(best):"x"(dots):"memory");return unsigned(_mm_movemask_ps(best));}
bool simd_greater(__m128 left,__m128 right){unsigned char value;asm volatile("comiss %2,%1\n\tseta %0":"=qm"(value):"x"(left),"x"(right):"cc","memory");return value;}
std::uint16_t compare(std::uint32_t left,std::uint32_t right){std::uint16_t status;asm volatile("flds %1\n\tfcomps %2\n\tfnstsw %0":"=a"(status):"m"(left),"m"(right):"memory","st");return status;}
std::uint32_t round_scalar(std::uint32_t value){std::uint32_t result;asm volatile("flds %1\n\tfstps %0":"=m"(result):"m"(value):"memory","st");return result;}
bool signed_positive(std::uint32_t n){return std::bit_cast<std::int32_t>(n)>0;}
#endif
}
void native_tu15_contact_shape_center(SourcePhysicsContactMemory&memory,
    NativeTu15ContactComponent shape,NativeTu15ContactVirtualOwner owner,
    std::uint32_t output,const SourcePhysicsContactRecordObserver&observe){
    Memory m(memory,observe);
    while(owner.identity==0x2e5da0u){shape.identity=m.word(shape.identity+0x30u);owner=m.owner(shape.identity,0x38u);}
    if(owner.identity==0x2e8d70u){
        if(output&15u)throw std::runtime_error("Native TU1.5 sphere center requires aligned vector output");
        const std::array<std::byte,16> zero{};m.bytes(0x2e8d7cu,output,zero);return;
    }
    if(owner.identity==0x3077c0u){
        auto coordinates=m.word(shape.identity+0x30u);m.word(0x3077c9u,output,m.word(coordinates));
        coordinates=m.word(shape.identity+0x30u);m.word(0x3077d1u,output+4u,m.word(coordinates+0x10u));
        coordinates=m.word(shape.identity+0x30u);m.word(0x3077dau,output+8u,m.word(coordinates+0x20u));m.word(0x3077ddu,output+0xcu,0);return;
    }
    if(owner.identity==0x2e7e10u){const auto value=m.vector_bytes(shape.identity+0x20u);if(output&15u)throw std::runtime_error("Native TU1.5 polygon center requires aligned vector output");m.bytes(0x2e7e18u,output,value);return;}
    if(owner.identity==0x306750u){const auto value=m.vector_bytes(shape.identity+0x10u);if(output&15u)throw std::runtime_error("Native TU1.5 box center requires aligned vector output");m.bytes(0x306758u,output,value);return;}
    throw std::runtime_error("Native TU1.5 contact shape requires its unsupported center owner");
}
std::uint32_t native_tu15_contact_shape_vertex_encoding()noexcept{return 0xffffffffu;}
void native_tu15_contact_shape_support(SourcePhysicsContactMemory&memory,
    NativeTu15ContactComponent shape,NativeTu15ContactVirtualOwner owner,
    std::uint32_t direction,std::uint32_t output,const NativeTu15ContactShapeScratch&scratch,
    NativeTu15PhysicsFrameFp&fp,unsigned depth,const SourcePhysicsContactRecordObserver&observe){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    Memory m(memory,observe);
    // Actual2E8D20 reads neither the shape nor the direction and leaves FP unchanged.
    if(owner.identity==0x2e8d20u){
        if(output&15u)throw std::runtime_error("Native TU1.5 sphere support requires aligned vector output");
        const std::array<std::byte,16> zero{};m.bytes(0x2e8d2cu,output,zero);return;
    }
    Environment environment(fp,depth);
    if(owner.identity==0x2e5db0u){
        const NativeTu15ContactComponent child{m.word(shape.identity+0x30u)};const auto method=m.owner(child.identity,0x30u);
        if(!scratch.child)throw std::runtime_error("Native TU1.5 sweep support requires its owned child DATA");
        environment.publish();try{native_tu15_contact_shape_support(memory,child,method,direction,output,*scratch.child,fp,depth,observe);environment.resume();}catch(...){environment.resume();throw;}
        const auto sweep=m.vector(shape.identity+0x10u),input=m.vector(direction),product=multiply(sweep,input);
        const auto xy=add_scalar(_mm_shuffle_ps(product,product,0x55),product),dot=add_scalar(_mm_shuffle_ps(product,product,0xaa),xy);m.word(0x2e5df3u,scratch.scalar,low(dot));
        const auto value=m.word(scratch.scalar);
        // Source2E5DF7 has already loaded ST0 when2E5DFB reads zero.
        asm volatile("flds %0"::"m"(value):"memory","st");
        const auto zero=m.word(0x46a0e8u);std::uint16_t relation;
        asm volatile("fcomps %1;fnstsw %0":"=a"(relation):"m"(zero):"memory","st");
        if(!(relation&0x4100u)){
            const auto vertex=m.vector(output);const auto tag=m.word(output+0xcu);asm volatile("flds %0"::"m"(tag):"memory","st");
            m.vector(0x2e5e11u,output,add(vertex,sweep));m.pop_float(0x2e5e14u,output+0xcu);
        }return;
    }
    if(owner.identity==0x2e7d90u){
        const auto second=m.vector(shape.identity+0x20u),first=m.vector(shape.identity+0x10u),input=m.vector(direction);
        const auto product=multiply(subtract(second,first),input),xy=add_scalar(_mm_shuffle_ps(product,product,0x55),product),dot=add_scalar(_mm_shuffle_ps(product,product,0xaa),xy);
        m.word(0x2e7dcau,scratch.scalar,low(dot));const auto value=m.word(scratch.scalar),zero=m.word(0x46a0e8u);const auto status=compare(value,zero);
        const bool use_second=(status&0x0500u)!=0x0100u;const auto feature=use_second?0x10u:0;
        m.vector(use_second?0x2e7dffu:0x2e7de9u,output,use_second?second:first);m.word(use_second?0x2e7e02u:0x2e7decu,output+0xcu,feature|0x3f000000u);return;
    }
    if(owner.identity==0x306790u){
        const auto vertex=m.vector(shape.identity+0x10u);m.vector(0x30679du,output,vertex);
        const auto sign=m.vector(0x461960u),input=m.vector(direction),value=_mm_xor_ps(vertex,_mm_and_ps(input,sign));
        const auto feature=std::uint32_t(_mm_movemask_ps(value))&7u;
        m.vector(0x3067bfu,output,value);m.word(0x3067c2u,output+0xcu,feature|0x3f000000u);return;
    }
    if(owner.identity==0x1f6610u){
        const auto count=m.word(shape.identity+0x14u);auto best=scalar(m.word(0x44aac8u));std::uint32_t selected{};
        if(signed_positive(count)){
            const auto input=m.vector(direction);for(std::uint32_t n=0;n<count;++n){
                const auto vertex=m.vector(shape.identity+0x20u+n*16u),product=multiply(vertex,input);
                const auto xy=add_scalar(_mm_shuffle_ps(product,product,0x55),product),dot=add_scalar(_mm_shuffle_ps(product,product,0xaa),xy);m.word(0x1f6656u,scratch.scalar,low(dot));
                const auto candidate=scalar(m.word(scratch.scalar));if(simd_greater(candidate,best)){best=candidate;selected=n;}
            }
        }
        m.vector(0x1f6681u,output,m.vector(shape.identity+(selected+2u)*16u));return;
    }
    if(owner.identity==0x3077f0u){
        const auto coordinates=m.word(shape.identity+0x30u),count=m.word(shape.identity+0x34u)<<2u;
        m.word(0x307802u,scratch.scalar,0xff7fffffu);auto maximum=scalar(m.word(scratch.scalar));maximum=_mm_shuffle_ps(maximum,maximum,0);std::uint32_t selected{};
        if(signed_positive(count)){
            const auto x=m.word(direction);m.word(0x307828u,scratch.scalar,x);const auto y=m.word(direction+4u);const auto x_value=scalar(m.word(scratch.scalar));const auto z=m.word(direction+8u);m.word(0x307838u,scratch.scalar,y);const auto y_value=scalar(m.word(scratch.scalar));m.word(0x307842u,scratch.scalar,z);const auto z_value=scalar(m.word(scratch.scalar));
            const auto dx=_mm_shuffle_ps(x_value,x_value,0),dy=_mm_shuffle_ps(y_value,y_value,0),dz=_mm_shuffle_ps(z_value,z_value,0);
            for(std::uint32_t first=0;signed_positive(count-first);first+=4u){
                const auto p=coordinates+(first>>2u)*0x30u;const auto z_term=multiply(dz,m.vector(p+0x20u)),y_term=multiply(dy,m.vector(p+0x10u)),x_term=multiply(dx,m.vector(p));auto dots=add(x_term,y_term);dots=add(dots,z_term);
                const auto mask=improvements(maximum,dots);m.vector(0x307892u,scratch.candidate_dots,dots);
                if(mask){
                    const auto greater=[&](unsigned a,unsigned b){const auto left=m.word(scratch.candidate_dots+a*4u),right=m.word(scratch.candidate_dots+b*4u);return!(compare(left,right)&0x4100u);};
                    unsigned lane{};
                    if(mask==1u)lane=0;else if(mask==2u)lane=1;else if(mask==4u)lane=2;else if(mask==8u)lane=3;
                    else if(mask==14u){if(greater(1,2))lane=greater(1,3)?1:3;else lane=greater(2,3)?2:3;}
                    else if(mask==13u){if(greater(0,2))lane=greater(0,3)?0:3;else lane=greater(2,3)?2:3;}
                    else if(greater(0,1)){if(greater(0,2))lane=greater(0,3)?0:3;else lane=greater(2,3)?2:3;}
                    else{if(greater(1,2))lane=greater(1,3)?1:3;else lane=greater(2,3)?2:3;}
                    const auto value=m.word(scratch.candidate_dots+lane*4u);selected=first+lane;m.word(0x30792bu,scratch.scalar,round_scalar(value));maximum=scalar(m.word(scratch.scalar));maximum=_mm_shuffle_ps(maximum,maximum,0);
                }
            }
        }
        const auto p=coordinates+(selected>>2u)*0x30u+(selected&3u)*4u;
        m.word(0x307965u,output,m.word(p));m.word(0x30796bu,output+4u,m.word(p+0x10u));m.word(0x307972u,output+8u,m.word(p+0x20u));m.word(0x30797bu,output+0xcu,selected|0x3f000000u);return;
    }
    throw std::runtime_error("Native TU1.5 contact shape requires its unsupported support owner");
#else
    (void)memory;(void)shape;(void)owner;(void)direction;(void)output;(void)scratch;(void)fp;(void)depth;(void)observe;throw std::runtime_error("Exact native TU1.5 contact shape backend unavailable");
#endif
}
} // namespace halo2
