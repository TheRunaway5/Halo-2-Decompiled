#include "halo2/native_tu15_model_constraint_lookup.hpp"
#include <bit>
#include <stdexcept>
#include <xmmintrin.h>
#include <emmintrin.h>
namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned width=4){
    std::array<std::byte,4>b{};m.read(p,std::span(b).first(width));std::uint32_t out{};
    for(unsigned n=0;n<width;++n)out|=std::to_integer<unsigned>(b[n])<<(n*8u);return out;
}
std::int32_t short_signed(std::uint32_t value){return std::bit_cast<std::int16_t>(std::uint16_t(value));}
bool less(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b);}
bool pair(std::uint32_t a,std::uint32_t b,std::uint32_t c,std::uint32_t d){return(a==c&&b==d)||(b==c&&a==d);}
class Environment {
    NativeTu15PhysicsFrameFp&fp_;unsigned previous_;
public:
    Environment(NativeTu15PhysicsFrameFp&fp):fp_(fp),previous_(_mm_getcsr()){
        if((fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))throw std::runtime_error("Native TU1.5 constraint lookup requires its unsupported SIMD trap owner");_mm_setcsr(fp.mxcsr);
    }
    ~Environment(){fp_.mxcsr=_mm_getcsr();_mm_setcsr(previous_);}
};
__m128 scalar(std::uint32_t v){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(v)));}
__m128 sub(__m128 a,__m128 b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 mul(__m128 a,__m128 b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 add(__m128 a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
bool greater(__m128 a,__m128 b){unsigned char out;asm volatile("comiss %2,%1; seta %0":"=qm"(out):"x"(a),"x"(b):"cc","memory");return out;}
class Lookup {
    const SourcePhysicsContactMemory&memory_;NativeTu15PhysicsFrameFp&fp_;NativeTu15ModelConstraintLookupStats*stats_;
    std::uint32_t word(std::uint32_t p,unsigned width=4)const{return read(memory_,p,width);}
    bool visible(std::uint32_t first,std::uint32_t second){ //22E0D0
        return native_tu15_model_constraint_bodies_visible(memory_,first,second,stats_);
    }
    bool matching(std::uint32_t instance,std::uint32_t request,std::uint32_t flags){ //22DEF0
        const auto request_kind=short_signed(word(request+0x3au,2)),instance_kind=short_signed(word(instance+0x9cu,2));
        const auto request_mode=word(request+0x38u,1),instance_mode=word(instance+0xeu,1),first=word(request+4u),second=word(request+0x10u),instance_second=word(instance+0x28u),request_class=word(request+0x34u);
        const auto instance_class=std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(word(instance+0xdu,1))));
        const auto request_node_b=word(request+0x18u,2),request_node_a=word(request+0xcu,2),instance_node_b=word(instance+0x2eu,2),instance_node_a=word(instance+0x2cu,2),instance_first=word(instance+0x24u);
        // Native22E040/22E000 retain both the datum and node unordered tests.
        if(instance_mode){if(!request_mode||!pair(instance_node_a,instance_node_b,request_node_a,request_node_b))return false;}
        else if(request_mode||!pair(instance_first,instance_second,first,second)||
            !pair(instance_node_a,instance_node_b,request_node_a,request_node_b)||
            instance_kind!=request_kind||std::uint32_t(instance_class)!=request_class)return false;
        if(stats_)++stats_->key_matches;
        if(std::uint8_t(flags)){
            if(stats_)++stats_->pose_queries;Environment environment(fp_);
            const auto request_y=scalar(word(request+0x20u)),instance_y=scalar(word(instance+0x6cu)),y=sub(request_y,instance_y);
            const auto request_x=scalar(word(request+0x1cu)),instance_x=scalar(word(instance+0x68u)),x=sub(request_x,instance_x);
            const auto request_z=scalar(word(request+0x24u)),instance_z=scalar(word(instance+0x70u)),z=sub(request_z,instance_z);
            const auto yy=mul(y,y),xx=mul(x,x),zz=mul(z,z),distance=add(add(xx,yy),zz);
            if(!greater(scalar(word(0x46a130u)),distance)){
                if(!greater(scalar(word(0x4500b4u)),distance))return false;
                const auto instance_z=scalar(word(instance+0x64u)),request_z=scalar(word(request+0x30u)),z_product=mul(instance_z,request_z);
                const auto instance_y=scalar(word(instance+0x60u)),request_y=scalar(word(request+0x2cu)),y_product=mul(instance_y,request_y),zy=add(z_product,y_product);
                const auto request_x=scalar(word(request+0x28u)),instance_x=scalar(word(instance+0x5cu)),x_product=mul(request_x,instance_x),dot=add(zy,x_product);
                if(!greater(dot,scalar(word(0x56b170u))))return false;
            }
        }
        return visible(first,second);
    }
public:
    Lookup(const SourcePhysicsContactMemory&m,NativeTu15PhysicsFrameFp&fp,NativeTu15ModelConstraintLookupStats*stats):memory_(m),fp_(fp),stats_(stats){}
    std::uint32_t run(NativeTu15PhysicsComponent component,NativeTu15ModelConstraintRequest request,std::uint32_t flags){ //1DD710
        if(stats_)++stats_->calls;const auto datum=word(component.identity+0x20u);std::uint32_t index{};
        for(;;){
            std::uint32_t count{};
            if(datum!=0xffffffffu){const auto pool=word(0x52d690u),entries=word(pool+0x44u);count=std::uint32_t(short_signed(word(entries+(datum&0xffffu)*64u+2u,2)));}
            if(!less(index,count))return 0xffffffffu;
            const auto lists=word(0x52d690u),entries=word(lists+0x44u),candidate=word(entries+(datum&0xffffu)*64u+index*4u+4u),pool=word(0x52d68cu),instances=word(pool+0x44u);
            if(stats_)++stats_->candidates;
            if(matching(instances+(candidate&0xffffu)*160u,request.identity,flags)){if(stats_)++stats_->found;return candidate;}
            ++index;
        }
    }
};
}
bool native_tu15_model_constraint_bodies_visible(const SourcePhysicsContactMemory&memory,
 std::uint32_t first,std::uint32_t second,NativeTu15ModelConstraintLookupStats*stats){
    if(stats)++stats->body_visibility_queries;
    const auto objects=read(memory,0x4ecbd0u),manager=read(memory,0x52be5cu);
    const auto is_biped=[&](std::uint32_t datum){
        if(datum==0xffffffffu)return false;
        const auto components=read(memory,manager+0x44u),object=read(memory,components+(datum&0xffffu)*160u+8u);
        const auto headers=read(memory,objects+0x44u),type=read(memory,headers+(object&0xffffu)*12u+3u,1);
        return((1u<<(type&31u))&1u)!=0;
    };
    const auto first_biped=is_biped(first),second_biped=is_biped(second);
    bool first_visible=true,second_visible=true;
    if(first_biped){const auto bank=read(memory,manager+0x44u);first_visible=(read(memory,bank+(first&0xffffu)*160u+4u)&0x800u)!=0;}
    if(second_biped){const auto bank=read(memory,manager+0x44u);second_visible=(read(memory,bank+(second&0xffffu)*160u+4u)&0x800u)!=0;}
    return first_visible&&second_visible;
}
std::uint32_t native_tu15_find_model_constraint(const SourcePhysicsContactMemory&memory,
 NativeTu15PhysicsComponent component,NativeTu15ModelConstraintRequest request,std::uint32_t flags,
 NativeTu15PhysicsFrameFp&fp,NativeTu15ModelConstraintLookupStats*stats){return Lookup(memory,fp,stats).run(component,request,flags);}
} // namespace halo2
