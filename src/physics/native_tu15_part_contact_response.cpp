#include "halo2/native_tu15_part_contact_response.hpp"
#include <bit>
#include <cstring>
#include <stdexcept>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif
namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory&m_;const SourcePhysicsContactRecordObserver&o_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):m_(m),o_(o){}
    std::uint32_t read(std::uint32_t p,unsigned width=4)const{
        std::array<std::byte,4>b{};m_.read(p,std::span(b).first(width));std::uint32_t v{};
        for(unsigned n=0;n<width;++n)v|=std::to_integer<unsigned>(b[n])<<(8u*n);return v;
    }
    void write(std::uint32_t pc,std::uint32_t p,std::uint32_t v,unsigned width=4){
        std::array<std::byte,4>b{};for(unsigned n=0;n<width;++n)b[n]=std::byte(v>>(8u*n));
        SourcePhysicsContactRecordStore event{pc,p,{},{}};
        if(o_){event.before.resize(width);m_.read(p,event.before);event.written.assign(b.begin(),b.begin()+width);}
        m_.write(p,std::span(b).first(width));if(o_)o_(event);
    }
};
std::int32_t byte_part(std::uint32_t v){return std::bit_cast<std::int8_t>(std::uint8_t(v));}
std::uint32_t word_id(std::uint32_t v){return std::bit_cast<std::uint32_t>(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(v))));}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
using Scalar=__m128;using Vector=std::array<Scalar,3>;
Scalar scalar(std::uint32_t v){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(v)));}
std::uint32_t bits(Scalar v){return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(v));}
Scalar add(Scalar a,Scalar b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar mul(Scalar a,Scalar b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar divide(Scalar a,Scalar b){asm volatile("divss %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar integer(std::uint32_t n){Scalar v=_mm_setzero_ps();const auto value=std::bit_cast<std::int32_t>(n);asm volatile("cvtsi2ss %1,%0":"+x"(v):"r"(value):"memory");return v;}
bool above(Scalar a,Scalar b){unsigned char result;asm volatile("comiss %2,%1;seta %0":"=qm"(result):"x"(a),"x"(b):"cc","memory");return result!=0;}
class Environment {
    NativeTu15PhysicsFrameFp&fp_;alignas(16)std::array<std::byte,108>host_{};
    unsigned host_mx_{};bool active_{};
public:
    Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth):fp_(fp){
        if(depth||(fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||
           (fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))
            throw std::runtime_error("Native TU1.5 part response requires its qualified empty/masked FP profile");
        host_mx_=_mm_getcsr();asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");resume();
    }
    void resume(){std::array<std::byte,28>env{};std::memcpy(env.data(),host_.data(),28);
        const std::uint16_t empty=0xffff;std::memcpy(env.data(),&fp_.control_word,2);std::memcpy(env.data()+4,&fp_.status,2);std::memcpy(env.data()+8,&empty,2);
        asm volatile("fldenv %0"::"m"(env):"memory");_mm_setcsr(fp_.mxcsr);active_=true;
    }
    void suspend(){std::array<std::byte,28>env{};asm volatile("fnstenv %0":"=m"(env)::"memory");
        std::memcpy(&fp_.control_word,env.data(),2);std::memcpy(&fp_.status,env.data()+4,2);fp_.mxcsr=_mm_getcsr();
        asm volatile("frstor %0"::"m"(host_):"memory");_mm_setcsr(host_mx_);active_=false;
    }
    ~Environment(){if(active_)suspend();}
};
struct Magnitude {std::uint32_t rounded{};long double absolute{};};
Magnitude magnitude(const Vector&v){
    const auto x=bits(v[0]),y=bits(v[1]),z=bits(v[2]);Magnitude out;
    asm volatile("flds %4;fmuls %4;flds %3;fmuls %3;faddp;flds %2;fmuls %2;faddp;fsqrt;fsts %0;fabs;fstpt %1"
        :"=m"(out.rounded),"=m"(out.absolute):"m"(x),"m"(y),"m"(z):"memory","st","st(1)");
    return out;
}
bool small(const Magnitude&length,std::uint32_t threshold){
    unsigned char result;
    asm volatile("fldt %1;flds %2;fcomip %%st(1),%%st;seta %0;fstp %%st(0)"
        :"=qm"(result):"m"(length.absolute),"m"(threshold):"cc","memory","st","st(1)");
    return result!=0;
}
std::uint32_t add_magnitude(const Vector&v,std::uint32_t sum){
    const auto x=bits(v[0]),y=bits(v[1]),z=bits(v[2]);std::uint32_t out;
    asm volatile("flds %4;fmuls %4;flds %3;fmuls %3;faddp;flds %2;fmuls %2;faddp;fsqrt;fadds %1;fstps %0"
        :"=m"(out):"m"(sum),"m"(x),"m"(y),"m"(z):"memory","st","st(1)");return out;
}
Vector defaults(Memory&m,std::uint32_t location){
    const auto p=m.read(location),x=m.read(p),y=m.read(p+4u),z=m.read(p+8u);
    return{scalar(x),scalar(y),scalar(z)};
}
#endif
} // namespace
void native_tu15_update_part_contact_response(SourcePhysicsContactMemory&memory,
    std::uint32_t slot,std::int32_t part,std::uint32_t component,
    NativeTu15PartContactResponseScratch scratch,NativeTu15PhysicsFrameFp&fp,unsigned depth,
    const NativeTu15ArrayGrowthServices&allocation,const NativeTu15ShaderFunctionEvaluationServices&evaluation,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15PartContactResponseStats*stats){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(stats)++stats->calls;Memory m(memory,observe);Environment environment(fp,depth);
    auto normal=defaults(m,0x474d04u);const auto contacts=m.read(component+0x8cu);
    std::array<std::uint32_t,3>materials{};std::uint32_t material_count{};
    if(std::bit_cast<std::int32_t>(contacts)>0){
        const auto rows=m.read(component+0x88u);
        for(std::uint32_t n=0;n<contacts;++n){const auto row=rows+n*72u;
            if(byte_part(m.read(row+0x45u,1))!=part||!(m.read(row+0x44u,1)&0x20u)||material_count>=3u)continue;
            const auto scenario=m.read(0x4ecc10u),table=m.read(scenario+0x2cu),index=m.read(row+0xcu),material=table+index*8u;
            if(material_count){const auto id=word_id(m.read(material+6u,2));bool duplicate{};
                for(unsigned i=0;i<material_count;++i)if(materials[i]==id){duplicate=true;break;}
                if(duplicate)continue;
            }
            const auto x=scalar(m.read(row+0x28u));const auto id=word_id(m.read(material+6u,2));
            normal[0]=add(x,normal[0]);normal[1]=add(scalar(m.read(row+0x2cu)),normal[1]);normal[2]=add(scalar(m.read(row+0x30u)),normal[2]);
            materials[material_count++]=id;
        }
    }
    if(!material_count){environment.suspend();native_tu15_clear_part_contact_response(memory,slot,scratch.removed_property,fp,allocation,observe);if(stats)++stats->inactive;return;}
    if(stats)stats->materials+=material_count;
    const auto normal_length=magnitude(normal);
    if(small(normal_length,m.read(0x46a0ecu)))normal=defaults(m,0x474d10u);
    else{const auto reciprocal=divide(scalar(m.read(0x46a0d0u)),scalar(normal_length.rounded));
        normal[1]=mul(normal[1],reciprocal);normal[2]=mul(normal[2],reciprocal);normal[0]=mul(reciprocal,normal[0]);}
    const auto dot_limit=scalar(m.read(0x46a130u));std::uint32_t hits{},processed{};
    const auto eligible=[&](std::uint32_t row){return byte_part(m.read(row+0x45u,1))==part&&!(m.read(row+0x44u,1)&0x20u);};
    const auto dot=[&](std::uint32_t row,bool y_before_x){
        auto z=scalar(m.read(row+0x30u));Scalar value;
        if(y_before_x){auto y=scalar(m.read(row+0x2cu));y=mul(y,normal[1]);z=mul(z,normal[2]);value=add(z,y);value=add(value,mul(scalar(m.read(row+0x28u)),normal[0]));}
        else{auto x=scalar(m.read(row+0x28u));x=mul(x,normal[0]);z=mul(z,normal[2]);value=add(z,x);value=add(value,mul(scalar(m.read(row+0x2cu)),normal[1]));}
        if(above(value,dot_limit))++hits;
    };
    if(std::bit_cast<std::int32_t>(contacts)>=4){const auto rows=m.read(component+0x88u),groups=((contacts-4u)>>2u)+1u;
        processed=groups*4u;for(std::uint32_t group=0;group<groups;++group)for(unsigned lane=0;lane<4;++lane){const auto row=rows+(group*4u+lane)*72u;if(eligible(row))dot(row,lane!=1u);}}
    if(std::bit_cast<std::int32_t>(processed)<std::bit_cast<std::int32_t>(contacts)){
        const auto rows=m.read(component+0x88u);for(auto index=processed;index<contacts;++index){const auto row=rows+index*72u;if(eligible(row))dot(row,false);}}
    if(stats)stats->opposing_contacts+=hits;
    auto response=defaults(m,0x474d04u);std::uint32_t magnitudes{};
    for(unsigned n=0;n<material_count;++n){environment.suspend();
        const auto found=native_tu15_material_surface_velocity(memory,materials[n],scratch.material_vector,scratch.conveyor,fp,depth,evaluation,observe,stats?&stats->conveyor:nullptr);
        environment.resume();Vector current;
        if(found){current={scalar(m.read(scratch.material_vector)),scalar(m.read(scratch.material_vector+4u)),scalar(m.read(scratch.material_vector+8u))};}
        else current=defaults(m,0x474d04u);
        response[0]=add(current[0],response[0]);response[1]=add(current[1],response[1]);response[2]=add(current[2],response[2]);
        magnitudes=add_magnitude(current,magnitudes);
    }
    const auto response_length=magnitude(response);
    if(!small(response_length,m.read(0x46a0ecu))){const auto reciprocal=divide(scalar(m.read(0x46a0d0u)),scalar(response_length.rounded));
        response[0]=mul(reciprocal,response[0]);response[1]=mul(response[1],reciprocal);response[2]=mul(response[2],reciprocal);}
    const auto flags=m.read(component+4u);auto scale=scalar(m.read(0x46a0d0u));if(!(flags&(1u<<18u)))scale=scalar(m.read(0x46a104u));
    scale=mul(scale,scalar(magnitudes));scale=divide(scale,integer(material_count));
    m.write(0x1e0ef5u,slot+0x30u,bits(mul(scale,response[0])));
    response[1]=mul(response[1],scale);response[2]=mul(response[2],scale);
    m.write(0x1e0f04u,slot+0x34u,bits(response[1]));m.write(0x1e0f09u,slot+0x38u,bits(response[2]));m.write(0x1e0f0eu,slot+0x3cu,0);
    if(stats)++stats->response_vectors;
    if(hits){const auto body=m.read(slot+0x40u);m.write(0x1e0fc3u,slot+0x44u,2u,1);
        environment.suspend();native_tu15_replace_part_contact_property(memory,body,0,scratch.removed_property,fp,allocation,observe);if(stats)++stats->zero_properties;
    }else{m.write(0x1e0f19u,slot+0x44u,1u,1);const auto current_flags=m.read(component+4u),body=m.read(slot+0x40u);
        const auto value=(current_flags&(1u<<18u))?0u:slot+0x30u;
        environment.suspend();native_tu15_replace_part_contact_property(memory,body,value,scratch.removed_property,fp,allocation,observe);
        if(stats){if(value)++stats->vector_properties;else ++stats->zero_properties;}
    }
#else
    (void)memory;(void)slot;(void)part;(void)component;(void)scratch;(void)fp;(void)depth;(void)allocation;(void)evaluation;(void)observe;(void)stats;
    throw std::runtime_error("Exact native TU1.5 part response arithmetic backend unavailable");
#endif
}
} // namespace halo2
