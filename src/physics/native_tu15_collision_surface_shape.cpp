#include "halo2/native_tu15_collision_surface_shape.hpp"
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
    void word(std::uint32_t pc,std::uint32_t p,std::uint32_t value,unsigned size=4){SourcePhysicsContactRecordStore event{pc,p,{},{}};if(observe_){event.before.resize(size);memory_.read(p,event.before);}std::array<std::byte,4>b{};for(unsigned k=0;k<size;++k)b[k]=std::byte(value>>(k*8u));memory_.write(p,std::span(b).first(size));if(observe_){event.written.assign(b.begin(),b.begin()+size);observe_(event);}}
};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Environment {
    NativeTu15PhysicsFrameFp&fp_;unsigned previous_;
public:
    explicit Environment(NativeTu15PhysicsFrameFp&fp):fp_(fp),previous_(_mm_getcsr()){if((fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))throw std::runtime_error("Native TU1.5 collision surface requires its unsupported SIMD exception owner");_mm_setcsr(fp.mxcsr);}
    ~Environment(){fp_.mxcsr=_mm_getcsr();_mm_setcsr(previous_);}
};
__m128 scalar(std::uint32_t n){__m128 value{};std::memcpy(&value,&n,4);return value;}
std::uint32_t bits(__m128 value){std::uint32_t n;std::memcpy(&n,&value,4);return n;}
__m128 multiply(__m128 left,__m128 right){asm volatile("mulss %1,%0":"+x"(left):"x"(right):"memory");return left;}
__m128 add(__m128 left,__m128 right){asm volatile("addss %1,%0":"+x"(left):"x"(right):"memory");return left;}
bool needs_scale(__m128 scale,__m128 one){unsigned char result;asm volatile("ucomiss %2,%1\n\tlahf\n\ttestb $0x44,%%ah\n\tsetp %0":"=m"(result):"x"(scale),"x"(one):"rax","cc","memory");return result!=0;}
#endif
}
NativeTu15ContactComponent native_tu15_build_collision_surface_shape(
    SourcePhysicsContactMemory&memory,NativeTu15ContactComponent shape,
    NativeTu15CollisionSurfaceArguments arguments,NativeTu15CollisionSurfaceTransform transform,
    NativeTu15CollisionSurfaceScratch scratch,NativeTu15PhysicsFrameFp&fp,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15CollisionSurfaceStats*stats){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    Memory m(memory,observe);Environment environment(fp);const auto p=shape.identity,a=arguments.identity,t=transform.identity;
    if(stats)++stats->calls;
    const auto radius=m.word(0x48b880u),initial_surface=m.word(a+0xcu),one=m.word(0x46a0d0u);
    m.word(0x1f5ffcu,p+8u,0);m.word(0x1f5fffu,p+0x14u,0);
    const auto category=m.word(a+4u,1);m.word(0x1f6006u,p+0x1bu,category,1);
    const auto resource=m.word(a+8u),model=m.word(a);
    m.word(0x1f6013u,p+0x1cu,resource);m.word(0x1f6016u,p+6u,1,2);
    m.word(0x1f601cu,p+0xcu,radius);m.word(0x1f6021u,p,0x4620d0u);m.word(0x1f6027u,p+0x10u,initial_surface);
    const auto surfaces=m.word(model+0x2cu),surface=surfaces+initial_surface*8u;
    m.word(0x1f6030u,a,surface);auto edge_index=m.word(surface+2u,2);m.word(0x1f6039u,a+4u,edge_index);
    auto surface_index=initial_surface;bool first=true;
    do{
        if(!first)surface_index=m.word(a+0xcu);first=false;
        const auto edges=m.word(model+0x34u),edge=edges+edge_index*12u;
        const auto edge_surface=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(m.word(edge+0xau,2)))));
        const auto side=std::uint32_t(edge_surface==surface_index),vertex_index=m.word(edge+side*2u,2);
        const auto vertices=m.word(model+0x3cu);auto vertex=vertices+vertex_index*16u;
        if(t){
            const auto scale=scalar(m.word(t));auto x=scalar(m.word(vertex)),y=scalar(m.word(vertex+4u)),z=scalar(m.word(vertex+8u));
            if(needs_scale(scale,scalar(one))){x=multiply(scale,x);y=multiply(scalar(m.word(t)),y);z=multiply(scalar(m.word(t)),z);}
            const auto first_z=scalar(m.word(t+0x1cu)),first_y=scalar(m.word(t+0x10u));
            auto z_term=multiply(first_z,z),y_term=multiply(first_y,y);
            auto value=add(z_term,y_term);const auto x_term=multiply(scalar(m.word(t+4u)),x);value=add(value,x_term);value=add(value,scalar(m.word(t+0x28u)));
            const auto next_y=scalar(m.word(t+0x14u));m.word(0x1f60d1u,scratch.transformed_vertex,bits(value));
            z_term=multiply(scalar(m.word(t+0x20u)),z);y_term=multiply(next_y,y);value=add(z_term,y_term);value=add(value,multiply(scalar(m.word(t+8u)),x));value=add(value,scalar(m.word(t+0x2cu)));
            m.word(0x1f60fau,scratch.transformed_vertex+4u,bits(value));
            z_term=multiply(scalar(m.word(t+0x24u)),z);const auto last_y=scalar(m.word(t+0x18u));y_term=multiply(last_y,y);const auto last_x=scalar(m.word(t+0xcu));value=add(z_term,y_term);value=add(value,multiply(last_x,x));value=add(value,scalar(m.word(t+0x30u)));
            m.word(0x1f6128u,scratch.transformed_vertex+8u,bits(value));vertex=scratch.transformed_vertex;if(stats)++stats->transformed_vertices;
        }
        const auto count=m.word(p+0x14u),z=m.word(vertex+8u),y=m.word(vertex+4u),x=m.word(vertex);
        const auto output=p+(count+2u)*16u;
        m.word(0x1f6147u,output,x);m.word(0x1f614cu,output+4u,y);m.word(0x1f6151u,output+8u,z);m.word(0x1f6156u,output+12u,0);
        const auto tag_count=m.word(p+0x14u);m.word(0x1f6169u,p+tag_count*16u+0x2cu,tag_count|0x3f000000u);
        const auto next_count=m.word(p+0x14u),first_edge=m.word(a+4u);
        m.word(0x1f6175u,p+0x14u,next_count+1u);edge_index=m.word(edge+side*2u+4u,2);if(stats)++stats->vertices;
        if(edge_index==first_edge)break;
    }while(true);
    const auto final_surface=m.word(a),material=m.word(final_surface+6u,2);m.word(0x1f618du,p+0x18u,material,2);
    const auto flags=m.word(final_surface+4u,1),key=m.word(a+0x10u);m.word(0x1f619au,p+0x1au,flags,1);m.word(0x1f619du,p+8u,key);return shape;
#else
    (void)memory;(void)shape;(void)arguments;(void)transform;(void)scratch;(void)fp;(void)observe;(void)stats;throw std::runtime_error("Exact native TU1.5 collision surface backend unavailable");
#endif
}
} // namespace halo2
