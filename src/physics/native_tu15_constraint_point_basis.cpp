#include "halo2/native_tu15_constraint_point_basis.hpp"
#include <bit>
#include <stdexcept>
#include <xmmintrin.h>
#include <emmintrin.h>
namespace halo2 {
namespace {
class Environment {
    NativeTu15PhysicsFrameFp&fp_;unsigned previous_;
public:
    explicit Environment(NativeTu15PhysicsFrameFp&fp):fp_(fp),previous_(_mm_getcsr()){_mm_setcsr(fp.mxcsr);}
    ~Environment(){fp_.mxcsr=_mm_getcsr();_mm_setcsr(previous_);}
};
std::uint32_t read(const SourcePhysicsContactMemory&m,std::uint32_t p){std::array<std::byte,4>b{};m.read(p,b);
    std::uint32_t value{};for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(b[n])<<(8u*n);return value;}
void store(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o,std::uint32_t pc,std::uint32_t p,std::uint32_t value){
    std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(value>>(8u*n));SourcePhysicsContactRecordStore event{pc,p,{},{}};
    if(o){event.before.resize(4);m.read(p,event.before);event.written.assign(b.begin(),b.end());}m.write(p,b);if(o)o(event);
}
__m128 scalar(std::uint32_t value){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(value)));}
std::uint32_t bits(__m128 value){return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(value)));}
__m128 mul(__m128 a,__m128 b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 add(__m128 a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 sub(__m128 a,__m128 b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
bool greater(__m128 a,__m128 b){unsigned char value;asm volatile("comiss %2,%1; seta %0":"=qm"(value):"x"(a),"x"(b):"cc","memory");return value;}
bool below_or_unordered(__m128 a,__m128 b){unsigned char value;asm volatile("comiss %2,%1; setb %0":"=qm"(value):"x"(a),"x"(b):"cc","memory");return value;}
}
void native_tu15_constraint_point_basis(SourcePhysicsContactMemory&m,std::uint32_t component,
    std::uint32_t instance,std::uint32_t output,NativeTu15PhysicsFrameFp&fp,unsigned depth,
    const SourcePhysicsContactRecordObserver&observer,NativeTu15ConstraintPointBasisStats*stats){
    if(stats)++stats->calls;
    if(depth>3u||(fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||
       (fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u)){
        if(stats)++stats->unsupported_profiles;throw std::runtime_error("Native TU1.5 constraint basis requires its unsupported numerical-fault owner");}
    const auto manager=read(m,0x52be5cu),components=read(m,manager+0x44u),object_pool=read(m,0x4ecbd0u);
    const auto object_datum=read(m,components+(component&0xffffu)*160u+8u),objects=read(m,object_pool+0x44u);
    const auto object=read(m,objects+(object_datum&0xffffu)*12u+8u);
    store(m,observer,0x22e250u,output+0x18u,read(m,instance+0x5cu));
    store(m,observer,0x22e255u,output+0x1cu,read(m,instance+0x60u));
    store(m,observer,0x22e25bu,output+0x20u,read(m,instance+0x64u));
    {
        Environment environment(fp);
        const auto product=[&](std::uint32_t first,std::uint32_t second){
            const auto a=scalar(read(m,first));const auto b=scalar(read(m,second));return mul(a,b);};
        auto dot=product(object+0x78u,instance+0x64u);
        auto other=product(object+0x74u,instance+0x60u);dot=add(dot,other);
        other=product(object+0x70u,instance+0x5cu);dot=add(dot,other);
        if(below_or_unordered(dot,_mm_setzero_ps()))dot=sub(_mm_setzero_ps(),dot);
        auto axis=object+0x70u;
        if(!greater(scalar(read(m,0x46a194u)),dot)){axis=object+0x7cu;if(stats)++stats->up_axes_selected;}
        store(m,observer,0x22e2a8u,output,read(m,axis));
        store(m,observer,0x22e2adu,output+4u,read(m,axis+4u));
        store(m,observer,0x22e2b3u,output+8u,read(m,axis+8u));
        auto projection=product(output+0x20u,output+8u);
        other=product(output+0x1cu,output+4u);
        const auto y=scalar(read(m,output+0x1cu)),z=scalar(read(m,output+0x20u));projection=add(projection,other);
        other=product(output+0x18u,output);projection=add(projection,other);
        const auto px=mul(projection,scalar(read(m,output+0x18u))),py=mul(y,projection),pz=mul(z,projection);
        store(m,observer,0x22e2fbu,output,bits(sub(scalar(read(m,output)),px)));
        store(m,observer,0x22e308u,output+4u,bits(sub(scalar(read(m,output+4u)),py)));
        store(m,observer,0x22e318u,output+8u,bits(sub(scalar(read(m,output+8u)),pz)));
    }
    native_tu15_normalize_constraint_axis(m,output,fp,depth,observer,stats?&stats->normalize:nullptr);
    {
        Environment environment(fp);
        auto z=scalar(read(m,output+0x18u)),y=scalar(read(m,output+0x1cu));
        y=mul(y,scalar(read(m,output)));z=mul(z,scalar(read(m,output+4u)));
        auto other=scalar(read(m,output+0x18u));other=mul(other,scalar(read(m,output+8u)));
        auto x=scalar(read(m,output+0x20u));x=mul(x,scalar(read(m,output+4u)));z=sub(z,y);
        y=scalar(read(m,output));y=mul(y,scalar(read(m,output+0x20u)));y=sub(y,other);
        other=scalar(read(m,output+0x1cu));other=mul(other,scalar(read(m,output+8u)));x=sub(other,x);
        store(m,observer,0x22e369u,output+0xcu,bits(x));store(m,observer,0x22e36du,output+0x10u,bits(y));
        store(m,observer,0x22e372u,output+0x14u,bits(z));
    }
    native_tu15_normalize_constraint_axis(m,output+0xcu,fp,depth,observer,stats?&stats->normalize:nullptr);
}
} // namespace halo2
