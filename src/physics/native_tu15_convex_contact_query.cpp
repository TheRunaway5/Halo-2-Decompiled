#include "halo2/native_tu15_convex_contact_query.hpp"
#include "halo2/native_tu15_inverse_geometry.hpp"
#include <cstring>
#include <stdexcept>
#include <type_traits>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif
namespace halo2 {
namespace {
template<class F>void require(const F&f,const char*why){if(!f)throw std::runtime_error(why);}
class Memory {
    SourcePhysicsContactMemory&memory_;const SourcePhysicsContactRecordObserver&observe_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):memory_(m),observe_(o){}
    std::uint32_t word(std::uint32_t p,unsigned size=4)const{std::array<std::byte,4>b{};memory_.read(p,std::span(b).first(size));std::uint32_t n{};for(unsigned k=0;k<size;++k)n|=std::to_integer<unsigned>(b[k])<<(k*8u);return n;}
    void bytes(std::uint32_t pc,std::uint32_t p,std::span<const std::byte>b){SourcePhysicsContactRecordStore event{pc,p,{},{}};if(observe_){event.before.resize(b.size());memory_.read(p,event.before);event.written.assign(b.begin(),b.end());}memory_.write(p,b);if(observe_)observe_(event);}
    void word(std::uint32_t pc,std::uint32_t p,std::uint32_t n,unsigned size=4){std::array<std::byte,4>b{};for(unsigned k=0;k<size;++k)b[k]=std::byte(n>>(k*8u));bytes(pc,p,std::span(b).first(size));}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    __m128 vector(std::uint32_t p)const{if(p&15u)throw std::runtime_error("Native TU1.5 convex query requires aligned vector DATA");std::array<std::byte,16>b{};memory_.read_vector(p,b);__m128 value;std::memcpy(&value,b.data(),16);return value;}
    void vector(std::uint32_t pc,std::uint32_t p,__m128 value){if(p&15u)throw std::runtime_error("Native TU1.5 convex query requires aligned vector DATA");std::array<std::byte,16>b{};std::memcpy(b.data(),&value,16);bytes(pc,p,b);}
    void scalar_store(std::uint32_t pc,std::uint32_t p,bool pop){std::uint32_t bits;if(pop)asm volatile("fstps %0":"=m"(bits)::"memory","st");else asm volatile("fsts %0":"=m"(bits)::"memory");word(pc,p,bits);}
#endif
};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Environment {
    alignas(16)std::array<std::byte,108>host_{};std::array<std::byte,28>native_{};
    NativeTu15PhysicsFrameFp&fp_;unsigned previous_;bool active_{};
    void validate()const{if((fp_.control_word&0x3fu)!=0x3fu||(fp_.status&0x8080u)||(fp_.mxcsr&0x1f80u)!=0x1f80u||(fp_.mxcsr&0xffff0000u))throw std::runtime_error("Native TU1.5 convex query requires its unsupported FP exception owner");}
public:
    Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth):fp_(fp),previous_(_mm_getcsr()){
        if(depth>7)throw std::runtime_error("Native TU1.5 convex query requires its overfull x87 owner");validate();asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");std::memcpy(native_.data(),host_.data(),28);resume();
    }
    ~Environment(){if(active_)publish();asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(previous_);}
    void publish(){asm volatile("fnstcw %0":"=m"(fp_.control_word));asm volatile("fnstsw %0":"=a"(fp_.status));fp_.mxcsr=_mm_getcsr();active_=false;}
    void resume(){validate();const std::uint16_t empty=0xffff;std::memcpy(native_.data(),&fp_.control_word,2);std::memcpy(native_.data()+4,&fp_.status,2);std::memcpy(native_.data()+8,&empty,2);asm volatile("fldenv %0"::"m"(native_):"memory");_mm_setcsr(fp_.mxcsr);active_=true;}
    template<class F>decltype(auto)dependency(F&&f){publish();try{if constexpr(std::is_void_v<std::invoke_result_t<F>>){f();resume();return;}else{auto result=f();resume();return result;}}catch(...){resume();throw;}}
};
__m128 scalar(std::uint32_t n){__m128 value{};std::memcpy(&value,&n,4);return value;}
std::uint32_t low(__m128 value){std::uint32_t n;std::memcpy(&n,&value,4);return n;}
__m128 multiply(__m128 left,__m128 right){asm volatile("mulps %1,%0":"+x"(left):"x"(right):"memory");return left;}
__m128 add(__m128 left,__m128 right){asm volatile("addps %1,%0":"+x"(left):"x"(right):"memory");return left;}
__m128 subtract(__m128 left,__m128 right){asm volatile("subps %1,%0":"+x"(left):"x"(right):"memory");return left;}
__m128 add_scalar(__m128 left,__m128 right){asm volatile("addss %1,%0":"+x"(left):"x"(right):"memory");return left;}
__m128 broadcast(std::uint32_t n){const auto value=scalar(n);return _mm_shuffle_ps(value,value,0);}
void push(std::uint32_t bits){asm volatile("flds %0"::"m"(bits):"memory","st");}
void plus(std::uint32_t bits){asm volatile("fadds %0"::"m"(bits):"memory","st");}
void minus(std::uint32_t bits){asm volatile("fsubs %0"::"m"(bits):"memory","st");}
void reversed_minus(std::uint32_t bits){asm volatile("fsubrs %0"::"m"(bits):"memory","st");}
void scalar_multiply(std::uint32_t bits){asm volatile("fmuls %0"::"m"(bits):"memory","st");}
void negate(){asm volatile("fchs":::"memory","st");}
std::uint16_t compare_pop(std::uint32_t bits){std::uint16_t status;asm volatile("fcomps %1\n\tfnstsw %0":"=a"(status):"m"(bits):"memory","st");return status;}
__m128 transpose_direction(__m128 x,__m128 y,__m128 z,__m128 value){const auto xy0=_mm_shuffle_ps(x,y,0x44),xy1=_mm_shuffle_ps(x,y,0xee),zz0=_mm_shuffle_ps(z,z,0x44),zz1=_mm_shuffle_ps(z,z,0xee);const auto tx=_mm_shuffle_ps(xy0,zz0,0x88),ty=_mm_shuffle_ps(xy0,zz0,0xdd),tz=_mm_shuffle_ps(xy1,zz1,0x88);const auto yterm=multiply(ty,_mm_shuffle_ps(value,value,0x55)),xterm=multiply(tx,_mm_shuffle_ps(value,value,0)),zterm=multiply(tz,_mm_shuffle_ps(value,value,0xaa));return add(add(xterm,yterm),zterm);}
void profile(Memory&m,std::uint32_t data,bool fast,const NativeTu15ConvexContactServices&s){const auto cursor=m.word(0x48c974u);if(cursor>=m.word(0x48c978u))return;m.word(fast?0x2f8675u:0x2f870bu,cursor,fast?0x41cd3cu:0x41cd34u);require(s.timestamp_counter,"Native TU1.5 convex query requires its actual timestamp owner");const auto stamp=std::uint32_t(s.timestamp_counter());m.word(fast?0x2f867du:0x2f8713u,data+0x14u,stamp);const auto retained=m.word(data+0x14u);m.word(fast?0x2f8685u:0x2f871bu,cursor+4u,retained);m.word(fast?0x2f868bu:0x2f8721u,0x48c974u,cursor+0xcu);}
#endif
}
void native_tu15_rotate_contact_direction(SourcePhysicsContactMemory&memory,
    std::uint32_t output,std::uint32_t basis,std::uint32_t source,
    NativeTu15PhysicsFrameFp&fp,const SourcePhysicsContactRecordObserver&observe){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    Memory m(memory,observe);Environment environment(fp,0);const auto input=m.vector(source),z=m.vector(basis+0x20u),y=m.vector(basis+0x10u);const auto z_term=multiply(z,_mm_shuffle_ps(input,input,0xaa)),y_term=multiply(y,_mm_shuffle_ps(input,input,0x55)),x=m.vector(basis),x_term=multiply(x,_mm_shuffle_ps(input,input,0));m.vector(0x2e479eu,output,add(add(x_term,y_term),z_term));
#else
    (void)memory;(void)output;(void)basis;(void)source;(void)fp;(void)observe;throw std::runtime_error("Exact native TU1.5 contact rotation unavailable");
#endif
}
void native_tu15_build_convex_contact_result(SourcePhysicsContactMemory&memory,
    std::uint32_t simplex,std::uint32_t plane,std::uint32_t output,
    std::uint32_t retained_scalar,NativeTu15PhysicsFrameFp&fp,
    const SourcePhysicsContactRecordObserver&observe){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    Memory m(memory,observe);Environment environment(fp,0);const auto distance=m.word(plane+0xcu);m.word(0x30d363u,output+0x20u,distance);const auto sign=m.vector(0x461960u),input=m.vector(plane),normal=_mm_xor_ps(input,sign);m.vector(0x30d373u,output,normal);
    if(m.word(simplex)==1u){const auto point=m.vector(simplex+0x20u);m.word(0x30d37fu,output+0x24u,0x3f800000u);m.vector(0x30d386u,output+0x10u,point);return;}
    if(m.word(simplex+4u)==1u){const auto distance_now=m.word(plane+0xcu);m.word(0x30d39au,retained_scalar,distance_now);const auto amount=broadcast(m.word(retained_scalar)),point=m.vector(simplex+0xa0u),offset=multiply(amount,normal),result=add(point,offset);m.word(0x30d3b8u,output+0x24u,0);m.vector(0x30d3bfu,output+0x10u,result);return;}
    const auto point=m.vector(simplex+0x130u);m.word(0x30d3d1u,output+0x24u,0x3f000000u);m.vector(0x30d3d8u,output+0x10u,point);
#else
    (void)memory;(void)simplex;(void)plane;(void)output;(void)retained_scalar;(void)fp;(void)observe;throw std::runtime_error("Exact native TU1.5 contact result builder unavailable");
#endif
}
std::uint32_t native_tu15_query_convex_contact(SourcePhysicsContactMemory&memory,
    NativeTu15ContactChild child,std::uint32_t valid_result,NativeTu15ContactPayload first,
    NativeTu15ContactPayload second,NativeTu15ContactContext context,std::uint32_t output,
    NativeTu15ConvexContactScratch scratch,NativeTu15PhysicsFrameFp&fp,unsigned depth,
    const NativeTu15ConvexContactServices&services,const SourcePhysicsContactRecordObserver&observe){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(scratch.identity&15u)throw std::runtime_error("Native TU1.5 convex query requires aligned retained DATA");
    Memory m(memory,observe);Environment environment(fp,depth);const auto p=scratch.identity,first_shape=m.word(first.identity);push(m.word(child.identity+0x2cu));const auto initial=compare_pop(m.word(0x46a0e8u));const auto second_shape=m.word(second.identity);m.word(0x2f8654u,p+0x18u,first_shape);m.word(0x2f8658u,p+0x1cu,second_shape);
    if(!(initial&0x4100u)){
        profile(m,p,true,services);const auto a=m.word(first.identity+8u);const auto a0=m.vector(a);const auto b=m.word(second.identity+8u);const auto a1=m.vector(a+0x10u),b1=m.vector(b+0x10u),normal=m.vector(child.identity+0x20u);const auto b_now=m.word(second.identity+8u);push(m.word(b_now+0xcu));const auto ad=subtract(a1,a0),b0=m.vector(b),bd=subtract(b1,b0),delta=subtract(bd,ad),product=multiply(delta,normal),xy=add_scalar(_mm_shuffle_ps(product,product,0x55),product),dot=add_scalar(_mm_shuffle_ps(product,product,0xaa),xy);m.word(0x2f86d8u,p+0x14u,low(dot));const auto a_now=m.word(first.identity+8u);plus(m.word(a_now+0xcu));plus(m.word(p+0x14u));reversed_minus(m.word(child.identity+0x2cu));m.scalar_store(0x2f86e9u,child.identity+0x2cu,false);if(!(compare_pop(m.word(0x46a0e8u))&0x4100u)){m.word(0x2f88bdu,valid_result,0,1);return valid_result;}
    }
    profile(m,p,false,services);const auto b=m.word(second.identity+8u),a=m.word(first.identity+8u);
    environment.dependency([&]{NativeTu15InverseGeometryMemory data(memory,p+0x30u,a+0x20u,b+0x20u,observe);native_tu15_compose_inverse_affine_geometry(data,fp);});
    const auto first_now=m.word(p+0x18u),second_now=m.word(p+0x1cu);m.word(0x2f875du,p+0x7cu,0,1);
    const auto solved=environment.dependency([&]{require(services.solve_simplex,"Native TU1.5 convex query requires actual30E930 simplex owner");return services.solve_simplex(p+0x70u,{first_now},{second_now},p+0x30u,child.identity+0xcu,p+0x20u,fp);});
    if(solved){m.word(0x2f88bdu,valid_result,0,1);return valid_result;}
    environment.dependency([&]{native_tu15_build_convex_contact_result(memory,p+0x70u,p+0x20u,output,scratch.result_scalar,fp,observe);});
    const auto transform=m.word(first.identity+8u);m.word(0x2f8793u,p+0x14u,output+0x40u);
    environment.dependency([&]{native_tu15_rotate_contact_direction(memory,output+0x40u,transform+0x20u,output,fp,observe);});
    push(m.word(output+0x20u));const auto distance_reference=m.word(p+0x18u);minus(m.word(distance_reference+0xcu));const auto second_reference=m.word(p+0x1cu);minus(m.word(second_reference+0xcu));m.scalar_store(0x2f87b0u,p+0x18u,true);const auto distance=m.word(p+0x18u);push(m.word(p+0x18u));m.word(0x2f87bcu,output+0x20u,distance);const auto relation=compare_pop(m.word(context.identity+8u));
    if((relation&0x0500u)==0x0100u){
        push(m.word(distance_reference+0xcu));const auto normal=m.vector(output),z=m.vector(p+0x50u);negate();const auto y=m.vector(p+0x40u);m.scalar_store(0x2f87dfu,p+0x14u,true);const auto amount=m.word(p+0x14u);push(m.word(p+0x18u));negate();m.scalar_store(0x2f87f5u,p+0x14u,true);const auto point=m.vector(output+0x10u),offset=multiply(broadcast(amount),normal),translation=m.vector(p+0x60u),shifted=add(point,offset);m.vector(0x2f880cu,output+0x10u,shifted);const auto depth_amount=m.word(p+0x14u);const auto normal_now=m.vector(output),point_now=m.vector(output+0x10u),correction=multiply(broadcast(depth_amount),normal_now),combined=add(point_now,correction),x=m.vector(p+0x30u),relative=subtract(combined,translation);m.vector(0x2f887eu,output+0x30u,transpose_direction(x,y,z,relative));m.word(0x2f8882u,valid_result,1,1);return valid_result;
    }
    if(!m.word(child.identity+0x18u)){const auto settings=m.word(context.identity+0xcu);if(m.word(settings,1)){
        const auto direction=m.word(p+0x14u);m.vector(0x2f88a4u,child.identity+0x20u,m.vector(direction));push(m.word(context.identity+8u));scalar_multiply(m.word(0x46a190u));reversed_minus(m.word(p+0x18u));m.scalar_store(0x2f88b5u,child.identity+0x2cu,true);
    }}
    m.word(0x2f88bdu,valid_result,0,1);return valid_result;
#else
    (void)memory;(void)child;(void)valid_result;(void)first;(void)second;(void)context;(void)output;(void)scratch;(void)fp;(void)depth;(void)services;(void)observe;throw std::runtime_error("Exact native TU1.5 convex query backend unavailable");
#endif
}
} // namespace halo2
