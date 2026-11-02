#include "halo2/native_tu15_contact_cache.hpp"
#include "halo2/native_tu15_geometry.hpp"
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
    NativeTu15ContactVirtualOwner owner(std::uint32_t object,unsigned slot)const{return{word(word(object)+slot)};}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    __m128 vector(std::uint32_t p)const{if(p&15u)throw std::runtime_error("Native TU1.5 contact cache requires aligned vector DATA");std::array<std::byte,16>b{};memory_.read_vector(p,b);__m128 value;std::memcpy(&value,b.data(),16);return value;}
    void vector(std::uint32_t pc,std::uint32_t p,__m128 value){if(p&15u)throw std::runtime_error("Native TU1.5 contact cache requires aligned vector DATA");std::array<std::byte,16>b{};std::memcpy(b.data(),&value,16);bytes(pc,p,b);}
#endif
};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Environment {
    NativeTu15PhysicsFrameFp&fp_;unsigned previous_;
    void resume(){if((fp_.mxcsr&0x1f80u)!=0x1f80u||(fp_.mxcsr&0xffff0000u))throw std::runtime_error("Native TU1.5 contact cache requires its unsupported SIMD exception owner");_mm_setcsr(fp_.mxcsr);}
public:
    explicit Environment(NativeTu15PhysicsFrameFp&fp):fp_(fp),previous_(_mm_getcsr()){resume();}
    ~Environment(){fp_.mxcsr=_mm_getcsr();_mm_setcsr(previous_);}
    template<class F>decltype(auto)dependency(F&&f){fp_.mxcsr=_mm_getcsr();try{if constexpr(std::is_void_v<std::invoke_result_t<F>>){f();resume();return;}else{auto result=f();resume();return result;}}catch(...){resume();throw;}}
};
__m128 multiply(__m128 left,__m128 right){asm volatile("mulps %1,%0":"+x"(left):"x"(right):"memory");return left;}
__m128 add(__m128 left,__m128 right){asm volatile("addps %1,%0":"+x"(left):"x"(right):"memory");return left;}
__m128 subtract(__m128 left,__m128 right){asm volatile("subps %1,%0":"+x"(left):"x"(right):"memory");return left;}
__m128 add_scalar(__m128 left,__m128 right){asm volatile("addss %1,%0":"+x"(left):"x"(right):"memory");return left;}
#endif
}
void native_tu15_initialize_convex_contact_cache(SourcePhysicsContactMemory&memory,
    std::uint32_t cache,NativeTu15ContactComponent first,NativeTu15ContactComponent second,
    std::uint32_t transform,NativeTu15ContactCacheScratch scratch,
    NativeTu15PhysicsFrameFp&fp,const NativeTu15ContactCacheServices&services,
    const SourcePhysicsContactRecordObserver&observe){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(scratch.identity&15u)throw std::runtime_error("Native TU1.5 contact cache requires aligned DATA ownership");
    Memory m(memory,observe);Environment environment(fp);const auto p=scratch.identity;
    const auto first_center=m.owner(first.identity,0x38u);
    environment.dependency([&]{require(services.shape_center,"Native TU1.5 cache requires actual shape center owner");services.shape_center(first,first_center,p+0x60u,fp);});
    const auto second_center=m.owner(second.identity,0x38u);
    environment.dependency([&]{services.shape_center(second,second_center,p+0x50u,fp);});
    environment.dependency([&]{NativeTu15GeometryMemory data(memory,p+0x40u,transform,p+0x50u,observe);native_tu15_transform_geometry_position(data,fp);});
    const auto center=m.vector(p+0x60u),other=m.vector(p+0x40u);const auto first_support_table=m.word(first.identity);
    m.vector(0x3090b9u,p+0x10u,subtract(other,center));
    const NativeTu15ContactVirtualOwner first_support{m.word(first_support_table+0x30u)};
    environment.dependency([&]{require(services.support_vertex,"Native TU1.5 cache requires actual support vertex owner");services.support_vertex(first,first_support,p+0x10u,p+0x30u,fp);});
    const auto first_tag=m.word(p+0x3cu,2);const auto support=m.vector(p+0x30u),transformed_center=m.vector(p+0x40u);
    const auto delta=subtract(transformed_center,support);m.word(0x3090e0u,cache,first_tag,2);m.vector(0x3090e3u,p+0x10u,delta);
    environment.dependency([&]{NativeTu15GeometryMemory data(memory,p+0x20u,transform,p+0x10u,observe);native_tu15_transform_transposed_direction(data,fp);});
    const auto direction=m.vector(p+0x20u),sign=m.vector(0x461960u);const auto second_support_table=m.word(second.identity);
    m.vector(0x30910au,p+0x20u,_mm_xor_ps(direction,sign));
    const NativeTu15ContactVirtualOwner second_support{m.word(second_support_table+0x30u)};
    environment.dependency([&]{services.support_vertex(second,second_support,p+0x20u,p+0x70u,fp);});
    const auto second_tag=m.word(p+0x7cu,2);m.word(0x309117u,cache+2u,second_tag,2);
    m.word(0x309120u,cache+4u,0xffffu,2);m.word(0x309124u,cache+6u,0xffffu,2);
    m.word(0x30912au,cache+8u,1,1);m.word(0x30912du,cache+9u,1,1);
    const auto first_encoding=m.owner(first.identity,0x3cu);
    const auto first_value=environment.dependency([&]{require(services.vertex_encoding,"Native TU1.5 cache requires actual vertex encoding owner");return services.vertex_encoding(first,first_encoding,fp);});
    m.word(0x309137u,cache+0xau,first_value,1);const auto second_encoding=m.owner(second.identity,0x3cu);
    const auto second_value=environment.dependency([&]{return services.vertex_encoding(second,second_encoding,fp);});m.word(0x309142u,cache+0xbu,second_value,1);
#else
    (void)memory;(void)cache;(void)first;(void)second;(void)transform;(void)scratch;(void)fp;(void)services;(void)observe;throw std::runtime_error("Exact native TU1.5 contact cache backend unavailable");
#endif
}
void native_tu15_initialize_triangle_contact_cache(SourcePhysicsContactMemory&memory,
    std::uint32_t cache,NativeTu15ContactComponent first,NativeTu15ContactComponent triangle,
    std::uint32_t transform,NativeTu15ContactCacheScratch scratch,
    NativeTu15PhysicsFrameFp&fp,const NativeTu15ContactCacheServices&services,
    const SourcePhysicsContactRecordObserver&observe){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(scratch.identity&15u)throw std::runtime_error("Native TU1.5 triangle cache requires aligned DATA ownership");
    Memory m(memory,observe);Environment environment(fp);const auto p=scratch.identity;
    m.word(0x309162u,cache+2u,0,2);m.word(0x309168u,cache+4u,0x10u,2);m.word(0x30916eu,cache+6u,0x20u,2);
    const auto translation=m.vector(transform+0x30u),basis_z=m.vector(transform+0x20u),basis_y=m.vector(transform+0x10u),basis_x=m.vector(transform);
    for(unsigned axis=0;axis<3;++axis){
        const auto input=m.vector(triangle.identity+0x10u+axis*16u);
        const auto z_term=multiply(basis_z,_mm_shuffle_ps(input,input,0xaa));
        const auto y_term=multiply(basis_y,_mm_shuffle_ps(input,input,0x55));
        const auto x_term=multiply(basis_x,_mm_shuffle_ps(input,input,0));
        const auto translated_z=add(z_term,translation);auto value=add(x_term,y_term);value=add(value,translated_z);
        m.vector(0x3091c5u,p+0x40u+axis*16u,value);
    }
    const auto second=m.vector(p+0x50u),third=m.vector(p+0x60u);const auto center_table=m.word(first.identity);
    const auto a=subtract(third,second),b=subtract(second,m.vector(p+0x40u));
    const auto first_product=multiply(_mm_shuffle_ps(b,b,0xd2),_mm_shuffle_ps(a,a,0xc9));
    const auto second_product=multiply(_mm_shuffle_ps(b,b,0xc9),_mm_shuffle_ps(a,a,0xd2));
    m.vector(0x309215u,p+0x10u,subtract(second_product,first_product));
    const NativeTu15ContactVirtualOwner center_owner{m.word(center_table+0x38u)};
    environment.dependency([&]{require(services.shape_center,"Native TU1.5 triangle cache requires actual shape center owner");services.shape_center(first,center_owner,p+0x20u,fp);});
    const auto center=m.vector(p+0x20u),normal=m.vector(p+0x10u),first_point=m.vector(p+0x40u);
    const auto product=multiply(subtract(center,first_point),normal);
    const auto xy=add_scalar(_mm_shuffle_ps(product,product,0x55),product);
    const auto dot=add_scalar(_mm_shuffle_ps(product,product,0xaa),xy);
    if(_mm_movemask_ps(dot)&1)m.vector(0x309257u,p+0x10u,_mm_xor_ps(normal,m.vector(0x461960u)));
    const auto support_owner=m.owner(first.identity,0x30u);
    environment.dependency([&]{require(services.support_vertex,"Native TU1.5 triangle cache requires actual support vertex owner");services.support_vertex(first,support_owner,p+0x10u,p+0x30u,fp);});
    const auto tag=m.word(p+0x3cu,2);m.word(0x309272u,cache,tag,2);m.word(0x309275u,cache+8u,1,1);m.word(0x309279u,cache+9u,3,1);
    const auto encoding_owner=m.owner(first.identity,0x3cu);
    const auto encoding=environment.dependency([&]{require(services.vertex_encoding,"Native TU1.5 triangle cache requires actual vertex encoding owner");return services.vertex_encoding(first,encoding_owner,fp);});
    m.word(0x309285u,cache+0xau,encoding,1);m.word(0x309288u,cache+0xbu,3,1);
#else
    (void)memory;(void)cache;(void)first;(void)triangle;(void)transform;(void)scratch;(void)fp;(void)services;(void)observe;throw std::runtime_error("Exact native TU1.5 triangle contact cache backend unavailable");
#endif
}
} // namespace halo2
