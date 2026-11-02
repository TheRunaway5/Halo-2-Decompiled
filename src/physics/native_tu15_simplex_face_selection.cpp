#include "halo2/native_tu15_simplex_face_selection.hpp"
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
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    __m128 vector(std::uint32_t p)const{if(p&15u)throw std::runtime_error("Native TU1.5 simplex face selection require aligned input DATA");std::array<std::byte,16>b{};memory_.read_vector(p,b);__m128 value;std::memcpy(&value,b.data(),16);return value;}
    void vector(std::uint32_t pc,std::uint32_t p,__m128 value){if(p&15u)throw std::runtime_error("Native TU1.5 simplex face selection require aligned output");std::array<std::byte,16>b{};std::memcpy(b.data(),&value,16);bytes(pc,p,b);}
    void scalar(std::uint32_t pc,std::uint32_t p,bool pop){std::uint32_t value;if(pop)asm volatile("fstps %0":"=m"(value)::"memory","st");else asm volatile("fsts %0":"=m"(value)::"memory");word(pc,p,value);}
#endif
};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Environment {
    alignas(16)std::array<std::byte,108>host_{};std::array<std::byte,28>native_{};NativeTu15PhysicsFrameFp&fp_;unsigned previous_;bool active_=true;
public:
    Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth):fp_(fp),previous_(_mm_getcsr()){
        if(depth>6||(fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||(fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))throw std::runtime_error("Native TU1.5 simplex face selection require their unsupported FP owner");asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");std::memcpy(native_.data(),host_.data(),28);const std::uint16_t empty=0xffff;std::memcpy(native_.data(),&fp.control_word,2);std::memcpy(native_.data()+4,&fp.status,2);std::memcpy(native_.data()+8,&empty,2);asm volatile("fldenv %0"::"m"(native_):"memory");_mm_setcsr(fp.mxcsr);
    }
    void publish(){asm volatile("fnstcw %0":"=m"(fp_.control_word));asm volatile("fnstsw %0":"=a"(fp_.status));fp_.mxcsr=_mm_getcsr();active_=false;}
    void resume(){const std::uint16_t empty=0xffff;std::memcpy(native_.data(),&fp_.control_word,2);std::memcpy(native_.data()+4,&fp_.status,2);std::memcpy(native_.data()+8,&empty,2);asm volatile("fldenv %0"::"m"(native_):"memory");_mm_setcsr(fp_.mxcsr);active_=true;}
    ~Environment(){if(active_)publish();asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(previous_);}
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
__m128 absolute(__m128 value,__m128 mask){return _mm_and_ps(value,mask);}
__m128 cross(__m128 a,__m128 b){return subtract(multiply(_mm_shuffle_ps(a,a,0xc9),_mm_shuffle_ps(b,b,0xd2)),multiply(_mm_shuffle_ps(a,a,0xd2),_mm_shuffle_ps(b,b,0xc9)));}
__m128 triple_products(__m128 a,__m128 b,__m128 c,__m128 value){const auto ab0=_mm_shuffle_ps(a,b,0x44),ab1=_mm_shuffle_ps(a,b,0xee),cc0=_mm_shuffle_ps(c,c,0x44),cc1=_mm_shuffle_ps(c,c,0xee);const auto x=_mm_shuffle_ps(ab0,cc0,0x88),y=_mm_shuffle_ps(ab0,cc0,0xdd),z=_mm_shuffle_ps(ab1,cc1,0x88);const auto zterm=multiply(z,_mm_shuffle_ps(value,value,0xaa)),yterm=multiply(y,_mm_shuffle_ps(value,value,0x55)),xterm=multiply(x,_mm_shuffle_ps(value,value,0));return add(add(xterm,yterm),zterm);}
std::uint16_t compare_to_next_pop(){std::uint16_t status;asm volatile("fcomp %%st(1)\n\tfnstsw %0":"=a"(status)::"memory","st","st(1)");return status;}
std::uint16_t compare_without_pop(std::uint32_t value){std::uint16_t status;asm volatile("fcoms %1\n\tfnstsw %0":"=a"(status):"m"(value):"memory","st");return status;}
std::uint16_t compare_unordered_two_pop(){std::uint16_t status;asm volatile("fucompp\n\tfnstsw %0":"=a"(status)::"memory","st","st(1)");return status;}
void discard(){asm volatile("fstp %%st(0)":::"memory","st");}
bool greater(std::uint16_t status){return(status&0x4100u)==0;}
#endif
}
std::uint32_t native_tu15_refine_simplex_face(SourcePhysicsContactMemory&memory,
    std::uint32_t vertices,std::uint32_t point,std::uint32_t scores,
    std::uint32_t p,NativeTu15PhysicsFrameFp&fp,unsigned depth,
    const SourcePhysicsContactRecordObserver&observe){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    Memory m(memory,observe);Environment environment(fp,depth);
    m.word(0x30d1b0u,p+0xcu,vertices);
    for(unsigned n=0;n<3;++n){
        const auto first=std::uint32_t(std::int32_t(std::int8_t(m.word(0x41d762u+n,1)))),second=std::uint32_t(std::int32_t(std::int8_t(m.word(0x41d760u+n,1))));
        push(m.word(scores+first*4u));const auto first_status=compare_pop(m.word(0x46a0e8u));
        if(!(first_status&0x0100u)){
            push(m.word(scores+second*4u));const auto second_status=compare_pop(m.word(0x46a0e8u));
            if(!(second_status&0x0100u)){
                const auto origin=m.vector(m.word(p+0xcu)),apex=m.vector(vertices+0x30u),first_vertex=m.vector(vertices+first*16u),first_direction=subtract(first_vertex,origin),apex_delta=subtract(apex,origin),second_vertex=m.vector(vertices+second*16u),ray=m.vector(point),ray_delta=subtract(ray,origin),second_direction=subtract(second_vertex,origin);
                const auto along_first=dot(ray_delta,first_direction),along_second=dot(ray_delta,second_direction),apex_first=dot(apex_delta,first_direction),apex_second=dot(apex_delta,second_direction);
                m.word(0x30d25eu,p+0x14u,along_first);m.word(0x30d277u,p+0x1cu,along_second);m.word(0x30d28du,p+0x18u,apex_first);m.word(0x30d2abu,p+0x10u,apex_second);
                push(m.word(p+0x10u));mul(m.word(p+0x14u));push(m.word(p+0x18u));mul(m.word(p+0x1cu));
                if(greater(compare_two_pop()))m.word(0x30d2c8u,scores+second*4u,0xbf800000u);
                else m.word(0x30d2d1u,scores+first*4u,0xbf800000u);
            }
        }
        const auto cursor=m.word(p+0xcu);m.word(0x30d2e3u,p+0xcu,cursor+16u);
    }
    for(unsigned n=0;n<3;++n){push(m.word(scores+n*4u));if(greater(compare_pop(m.word(0x46a0e8u))))return n;}
    return 0xffffffffu;
#else
    (void)memory;(void)vertices;(void)point;(void)scores;(void)p;(void)fp;(void)depth;(void)observe;throw std::runtime_error("Exact native TU1.5 face refinement backend unavailable");
#endif
}
std::uint32_t native_tu15_select_simplex_face(SourcePhysicsContactMemory&memory,
    std::uint32_t vertices,std::uint32_t point,std::uint32_t scale,
    NativeTu15SimplexFaceSelectionScratch scratch,NativeTu15PhysicsFrameFp&fp,
    unsigned depth,const SourcePhysicsContactRecordObserver&observe){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    Memory m(memory,observe);Environment environment(fp,depth);const auto p=scratch.identity;
    const auto v1=m.vector(vertices+0x10u),v0=m.vector(vertices),v3=m.vector(vertices+0x30u),v2=m.vector(vertices+0x20u);
    const auto e10=subtract(v1,v0),e02=subtract(v0,v2),e30=subtract(v3,v0),e31=subtract(v3,v1),e32=subtract(v3,v2),e21=subtract(v2,v1);
    const auto normal0=cross(e21,e31),normal1=cross(e02,e32),normal2=cross(e10,e30),current=m.vector(point),apex=m.vector(vertices+0x30u),delta=subtract(current,apex);
    m.word(0x30d691u,p+0x10u,scale);const auto scaled=multiply(broadcast(m.word(p+0x10u)),delta);
    m.vector(0x30d6abu,p+0x30u,normal0);m.vector(0x30d6c2u,p+0x40u,normal1);m.vector(0x30d6c7u,p+0x50u,normal2);
    const auto triples=triple_products(normal0,normal1,normal2,scaled),mask=m.vector(0x461950u),absolute_triples=absolute(triples,mask),signed_squares=multiply(triples,absolute_triples);m.vector(0x30d719u,p+0x20u,signed_squares);
    for(unsigned n=0;n<3;++n){const auto normal=m.vector(p+0x30u+n*16u);push(m.word(0x46a0e8u));const auto squared=dot(normal,normal);m.word(0x30d74au,p+0x10u,squared);push(m.word(p+0x10u));const auto status=compare_unordered_two_pop();const auto equal_bits=status&0x4400u;
        if(equal_bits==0x4000u||equal_bits==0x0400u)m.word(0x30d769u,p+0x14u+n*4u,0x7f7fffffu);
        else{push(m.word(p+0x20u+n*4u));divide(m.word(p+0x10u));m.scalar(0x30d763u,p+0x14u+n*4u,true);}
    }
    push(m.word(p+0x18u));std::uint32_t choice=1;push(m.word(p+0x14u));
    if(greater(compare_to_next_pop())){const auto old=m.word(p+0x14u);m.scalar(0x30d796u,p+0x14u,true);m.word(0x30d79au,p+0x10u,old);push(m.word(p+0x10u));choice=0;}
    if(greater(compare_without_pop(m.word(p+0x1cu)))){
        m.scalar(0x30d7afu,p+0x10u,true);const auto old=m.word(p+0x10u);push(m.word(p+0x1cu));m.word(0x30d7bbu,p+0x1cu,old);push(m.word(p+0x14u));
        if(greater(compare_to_next_pop())){discard();push(m.word(p+0x14u));}
    }else choice=2;
    push(m.word(p+0x1cu));const auto threshold_status=compare_pop(m.word(0x46a454u));
    if((threshold_status&0x0500u)==0x0100u){discard();return 0xffffffffu;}
    push(m.word(0x46a190u));asm volatile("fmul %%st(1),%%st(0)":::"memory","st","st(1)");push(m.word(p+0x1cu));const auto ranking=compare_two_pop();discard();
    if(greater(ranking))return choice;
    m.vector(0x30d81au,p+0x20u,triples);environment.publish();std::uint32_t result{};
    try{result=native_tu15_refine_simplex_face(memory,vertices,point,p+0x20u,scratch.refinement,fp,depth,observe);}
    catch(...){environment.resume();throw;}
    environment.resume();return result;
#else
    (void)memory;(void)vertices;(void)point;(void)scale;(void)scratch;(void)fp;(void)depth;(void)observe;throw std::runtime_error("Exact native TU1.5 face selection backend unavailable");
#endif
}
} // namespace halo2
