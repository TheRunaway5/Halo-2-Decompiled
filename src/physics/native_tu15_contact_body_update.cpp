#include "halo2/native_tu15_contact_body_update.hpp"
#include <bit>
#include <cstring>
#include <stdexcept>
#include <xmmintrin.h>
#include <emmintrin.h>
namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned n=4){
    std::array<std::byte,4>b{};m.read(p,std::span(b).first(n));std::uint32_t out{};
    for(unsigned i=0;i<n;++i)out|=std::to_integer<unsigned>(b[i])<<(i*8u);return out;
}
class Memory {
    SourcePhysicsContactMemory&m_;const SourcePhysicsContactRecordObserver&o_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):m_(m),o_(o){}
    std::uint32_t word(std::uint32_t p,unsigned n=4)const{return read(m_,p,n);}
    void store(std::uint32_t site,std::uint32_t p,std::uint32_t value,unsigned n=4){
        std::array<std::byte,4>b{};for(unsigned i=0;i<n;++i)b[i]=std::byte(value>>(i*8u));
        write(site,p,std::span(b).first(n));
    }
    void write(std::uint32_t site,std::uint32_t p,std::span<const std::byte>b){
        SourcePhysicsContactRecordStore event{site,p,{},{}};
        if(o_){event.before.resize(b.size());m_.read(p,event.before);event.written.assign(b.begin(),b.end());}
        m_.write(p,b);if(o_)o_(event);
    }
    __m128 vector(std::uint32_t p)const{
        if(p&15u)throw std::runtime_error("Native TU1.5 contact body requires source MOVAPS alignment");
        alignas(16)std::array<std::byte,16>b{};m_.read(p,b);__m128 v;std::memcpy(&v,b.data(),16);return v;
    }
    void vector(std::uint32_t site,std::uint32_t p,__m128 v){
        if(p&15u)throw std::runtime_error("Native TU1.5 contact body requires source MOVAPS alignment");
        alignas(16)std::array<std::byte,16>b{};std::memcpy(b.data(),&v,16);write(site,p,b);
    }
};
using Scalar=__m128;
Scalar scalar(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(bits)));}
std::uint32_t bits(Scalar v){return std::bit_cast<std::uint32_t>(_mm_cvtsi128_si32(_mm_castps_si128(v)));}
Scalar add(Scalar a,Scalar b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar sub(Scalar a,Scalar b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar mul(Scalar a,Scalar b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar divide(Scalar a,Scalar b){asm volatile("divss %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar mul_vector(Scalar a,Scalar b){asm volatile("mulps %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar sub_vector(Scalar a,Scalar b){asm volatile("subps %1,%0":"+x"(a):"x"(b):"memory");return a;}
Scalar add_vector(Scalar a,Scalar b){asm volatile("addps %1,%0":"+x"(a):"x"(b):"memory");return a;}
struct Comparison {bool below{},equal{},unordered{};bool above()const{return!below&&!equal;}bool ordered_zero()const{return equal&&!unordered;}};
Comparison compare(Scalar a,Scalar b,bool unordered=false){
    unsigned char below,equal,parity;
    if(unordered)asm volatile("ucomiss %4,%3;setb %0;sete %1;setp %2":"=qm"(below),"=qm"(equal),"=qm"(parity):"x"(a),"x"(b):"cc","memory");
    else asm volatile("comiss %4,%3;setb %0;sete %1;setp %2":"=qm"(below),"=qm"(equal),"=qm"(parity):"x"(a),"x"(b):"cc","memory");
    return{below!=0,equal!=0,parity!=0};
}
class SimdScope {
    NativeTu15PhysicsFrameFp&fp_;unsigned host_;bool active_{};
public:
    explicit SimdScope(NativeTu15PhysicsFrameFp&fp):fp_(fp),host_(_mm_getcsr()){resume();}
    void resume(){if((fp_.mxcsr&0x1f80u)!=0x1f80u||(fp_.mxcsr&0xffff0000u))throw std::runtime_error("Native TU1.5 contact body requires its SIMD trap owner");_mm_setcsr(fp_.mxcsr);active_=true;}
    void suspend(){fp_.mxcsr=_mm_getcsr();_mm_setcsr(host_);active_=false;}
    ~SimdScope(){if(active_)fp_.mxcsr=_mm_getcsr();_mm_setcsr(host_);}
};
class X87Scope {
    alignas(16)std::array<std::byte,108>host_{};NativeTu15PhysicsFrameFp&fp_;
public:
    X87Scope(NativeTu15PhysicsFrameFp&fp,unsigned depth):fp_(fp){
        if(depth>7||(fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u))throw std::runtime_error("Native TU1.5 contact body requires its x87 fault owner");
        asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        std::array<std::byte,28>env{};std::memcpy(env.data(),host_.data(),28);const std::uint16_t empty=0xffffu;
        std::memcpy(env.data(),&fp.control_word,2);std::memcpy(env.data()+4,&fp.status,2);std::memcpy(env.data()+8,&empty,2);asm volatile("fldenv %0"::"m"(env):"memory");
    }
    ~X87Scope(){asm volatile("fnstcw %0":"=m"(fp_.control_word));asm volatile("fnstsw %0":"=a"(fp_.status));asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");}
};
std::int32_t integer_round(std::uint32_t value,NativeTu15PhysicsFrameFp&fp,unsigned depth){
    X87Scope scope(fp,depth);std::int32_t result;asm volatile("flds %1;fistpl %0":"=m"(result):"m"(value):"memory","st");return result;
}
std::uint32_t extended_scale(std::uint32_t value,std::uint32_t factor,NativeTu15PhysicsFrameFp&fp,unsigned depth){
    X87Scope scope(fp,depth);std::uint32_t result;asm volatile("flds %1;fmuls %2;fstps %0":"=m"(result):"m"(value),"m"(factor):"memory","st");return result;
}
Scalar integer_float(std::int32_t v){auto result=_mm_setzero_ps();asm volatile("cvtsi2ss %1,%0":"+x"(result):"r"(v):"memory");return result;}
Scalar broadcast(Scalar v){return _mm_shuffle_ps(v,v,0);}
}
std::uint32_t native_tu15_contact_manager_point(const SourcePhysicsContactMemory&m,std::uint32_t manager,std::uint16_t contact){
    const auto indices=read(m,manager+0x30u),index=read(m,indices+contact,1),data=read(m,manager+0x44u);return data+(index<<5u);
}
std::uint32_t native_tu15_contact_manager_properties(const SourcePhysicsContactMemory&m,std::uint32_t manager,std::uint16_t contact){
    const auto indices=read(m,manager+0x30u),index=read(m,indices+contact,1),data=read(m,manager+0xa0u);return data+index*0x1cu;
}
void native_tu15_publish_contact_body_property(SourcePhysicsContactMemory&memory,std::uint32_t datum,
    std::uint32_t row,std::uint32_t properties,const SourcePhysicsContactRecordObserver&observe){
    Memory m(memory,observe);const auto components=m.word(0x52be5cu),component_rows=m.word(components+0x44u),objects=m.word(0x4ecbd0u);
    const auto object_datum=m.word(component_rows+(datum&0xffffu)*160u+8u),headers=m.word(objects+0x44u),header=headers+(object_datum&0xffffu)*12u;
    const auto type=m.word(header+3u,1);bool suppress=false;
    if((1u<<(type&31u))&2u){const auto object=m.word(header+8u),tag=m.word(object)&0xffffu,tags=m.word(0x4f0414u),definition=m.word(tags+tag*16u+8u);
        if((m.word(definition+0x1ecu)>>19u)&1u)suppress=m.word(object+0x248u)!=0xffffffffu||((m.word(object+0x134u)>>1u)&1u);
    }
    m.store(suppress?0x1d7bdau:0x1d7bebu,properties+8u,suppress?0u:m.word(row+0x3cu));
    const auto last=m.word(row+0x40u);m.store(suppress?0x1d7be2u:0x1d7bf3u,properties+0xcu,last);
}
void native_tu15_contact_motion_point_velocity(SourcePhysicsContactMemory&memory,std::uint32_t motion,
    std::uint32_t point,std::uint32_t output,NativeTu15PhysicsFrameFp&fp,const SourcePhysicsContactRecordObserver&observe){
    Memory m(memory,observe);SimdScope scope(fp);const auto center=m.vector(motion+0x70u),location=m.vector(point),offset=sub_vector(location,center),angular=m.vector(motion+0x50u);
    const auto first=mul_vector(_mm_shuffle_ps(angular,angular,0xd2),_mm_shuffle_ps(offset,offset,0xc9));
    const auto second=mul_vector(_mm_shuffle_ps(angular,angular,0xc9),_mm_shuffle_ps(offset,offset,0xd2));
    const auto cross=sub_vector(second,first);m.vector(0x323a4cu,output,cross);const auto linear=m.vector(motion+0x40u);m.vector(0x323a59u,output,add_vector(cross,linear));
}
void native_tu15_apply_contact_motion_point_impulse(SourcePhysicsContactMemory&memory,
    std::uint32_t motion,std::uint32_t impulse,std::uint32_t point,NativeTu15PhysicsFrameFp&fp,
    const SourcePhysicsContactRecordObserver&observe){
    Memory m(memory,observe);SimdScope scope(fp);const auto inverse_mass=scalar(m.word(motion+0x2cu));
    const auto first_impulse=m.vector(impulse),linear=m.vector(motion+0x40u);
    const auto linear_change=mul_vector(broadcast(inverse_mass),first_impulse);
    m.vector(0x324fb0u,motion+0x40u,add_vector(linear,linear_change));
    const auto center=m.vector(motion+0x70u),location=m.vector(point),offset=sub_vector(location,center);
    const auto current_impulse=m.vector(impulse),basis_y=m.vector(motion+0x90u);
    const auto first=mul_vector(_mm_shuffle_ps(offset,offset,0xd2),_mm_shuffle_ps(current_impulse,current_impulse,0xc9));
    const auto second=mul_vector(_mm_shuffle_ps(offset,offset,0xc9),_mm_shuffle_ps(current_impulse,current_impulse,0xd2));
    const auto basis_x=m.vector(motion+0x80u),cross=sub_vector(second,first),basis_z=m.vector(motion+0xa0u);
    const auto xy_low=_mm_shuffle_ps(basis_x,basis_y,0x44),z_low=_mm_shuffle_ps(basis_z,basis_z,0x44);
    const auto z_high=_mm_shuffle_ps(basis_z,basis_z,0xee),xy_high=_mm_shuffle_ps(basis_x,basis_y,0xee);
    const auto y_change=mul_vector(_mm_shuffle_ps(xy_low,z_low,0xdd),_mm_shuffle_ps(cross,cross,0x55));
    const auto x_scalar=broadcast(cross),inverse_inertia=m.vector(motion+0xe0u);
    const auto x_change=mul_vector(_mm_shuffle_ps(xy_low,z_low,0x88),x_scalar),current_y=m.vector(motion+0x90u);
    auto local=add_vector(x_change,y_change);
    const auto z_change=mul_vector(_mm_shuffle_ps(xy_high,z_high,0x88),_mm_shuffle_ps(cross,cross,0xaa));
    local=add_vector(local,z_change);const auto current_z=m.vector(motion+0xa0u),scaled=mul_vector(inverse_inertia,local);
    const auto z_world=mul_vector(current_z,_mm_shuffle_ps(scaled,scaled,0xaa));
    const auto y_world=mul_vector(current_y,_mm_shuffle_ps(scaled,scaled,0x55));
    const auto current_x=m.vector(motion+0x80u),x_world=mul_vector(current_x,broadcast(scaled)),angular=m.vector(motion+0x50u);
    m.vector(0x325094u,motion+0x50u,add_vector(angular,add_vector(add_vector(x_world,y_world),z_world)));
}
void native_tu15_apply_contact_body_point_impulse(SourcePhysicsContactMemory&memory,
    NativeTu15PhysicsBody body,std::uint32_t impulse,std::uint32_t point,NativeTu15PhysicsFrameFp&fp,
    unsigned depth,const NativeTu15ContactBodyImpulseServices&services,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15ContactBodyUpdateStats*stats){
    Memory m(memory,observe);
    const auto missing=[&](std::uint32_t owner)->void{if(stats){++stats->unqualified_services;++stats->unqualified_owners[owner];}throw std::runtime_error("Native TU1.5 contact body impulse requires native owner="+std::to_string(owner));};
    const auto island=m.word(body.identity+0x44u);const bool active=island&&m.word(island+0x29u,1)!=0;
    if(!active&&m.word(body.identity+0x44u)){
        if(!services.activate_body)missing(0x315cb0u);services.activate_body(body,fp,depth);
    }
    const auto motion=m.word(body.identity+0x3cu),table=m.word(motion),owner=m.word(table+0x60u);
    if(owner==0x324f80u){native_tu15_apply_contact_motion_point_impulse(memory,motion,impulse,point,fp,observe);return;}
    if(owner==0x8fdb0u)return; // Authenticated RET8 with no DATA or FP effects.
    if(!services.motion_impulse)missing(owner);services.motion_impulse(motion,{owner},impulse,point,fp,depth);
}
void native_tu15_update_contact_body(SourcePhysicsContactMemory&memory,std::uint32_t manager,
    std::uint32_t row,bool skip_impulse,bool reversed,std::uint32_t datum,std::uint32_t slot,
    NativeTu15ContactBodyUpdateScratch scratch,NativeTu15PhysicsFrameFp&fp,unsigned depth,
    const NativeTu15ContactBodyUpdateServices&services,const SourcePhysicsContactRecordObserver&observe,
    NativeTu15ContactBodyUpdateStats*stats){
    if(stats)++stats->calls;Memory m(memory,observe);
    const auto missing=[&](std::uint32_t owner)->void{if(stats){++stats->unqualified_services;++stats->unqualified_owners[owner];}throw std::runtime_error("Native TU1.5 contact body requires native owner="+std::to_string(owner));};
    auto contact=m.word(row,2),table=m.word(manager),owner=m.word(table+0x18u);std::uint32_t point{};
    if(stats)++stats->point_queries;
    if(owner==0x323020u)point=native_tu15_contact_manager_point(memory,manager,std::uint16_t(contact));
    else{if(!services.point)missing(owner);point=services.point(manager,{owner},std::uint16_t(contact),fp,depth);}
    table=m.word(manager);contact=m.word(row,2);owner=m.word(table+0x14u);std::uint32_t properties{};
    if(stats)++stats->property_queries;
    if(owner==0x323040u)properties=native_tu15_contact_manager_properties(memory,manager,std::uint16_t(contact));
    else{if(!services.properties)missing(owner);properties=services.properties(manager,{owner},std::uint16_t(contact),fp,depth);}
    const auto components=m.word(0x52be5cu),rows=m.word(components+0x44u),component=rows+(datum&0xffffu)*160u;
    m.store(0x1d781du,row+0x34u,m.word(point+0xcu));
    const auto x=m.word(point),z=m.word(point+8u),y=m.word(point+4u);m.store(0x1d782cu,row+0x1cu,x);m.store(0x1d7832u,row+0x20u,y);m.store(0x1d7837u,row+0x24u,z);
    const auto nx=m.word(point+0x10u),nz=m.word(point+0x18u),ny=m.word(point+0x14u);m.store(0x1d7849u,row+0x28u,nx);m.store(0x1d784cu,row+0x2cu,ny);m.store(0x1d7851u,row+0x30u,nz);
    const auto bank=m.word(component+0x70u),offset=slot*96u,body=m.word(bank+offset+0x40u),motion=m.word(body+0x3cu),motion_table=m.word(motion);owner=m.word(motion_table+0x18u);
    if(stats)++stats->motion_type_queries;
    if(!native_tu15_activation_type(owner)){if(!services.inspect_motion_type)missing(owner);services.inspect_motion_type(motion,{owner},fp,depth);}
    const auto current_bank=m.word(component+0x70u),current_body=m.word(current_bank+offset+0x40u),current_motion=m.word(current_body+0x3cu);
    SimdScope scope(fp);const auto inverse_mass=scalar(m.word(current_motion+0x2cu)),zero=_mm_setzero_ps();auto divisor=scalar(m.word(0x46a0d0u));
    if(!compare(inverse_mass,zero,true).ordered_zero()){
        const auto reciprocal=divide(divisor,inverse_mass);
        if(!compare(divisor,reciprocal).above())divisor=!compare(inverse_mass,zero,true).ordered_zero()?divide(divisor,inverse_mass):zero;
    }
    auto force=scalar(m.word(properties));if(compare(force,zero).below)force=sub(zero,scalar(m.word(properties)));
    if((bits(force)&0x7f800000u)==0x7f800000u)force=zero;
    force=divide(force,divisor);m.store(0x1d78f2u,row+0x38u,bits(force));
    if(reversed){const auto first_normal=scalar(m.word(row+0x28u)),sign=scalar(m.word(0x46a0dcu));
        m.store(0x1d790au,row+0x28u,bits(mul(first_normal,sign)));
        for(unsigned i=1;i<3;++i){const auto at=row+0x28u+i*4u;const auto value=mul(scalar(m.word(at)),sign);m.store(0x1d790au+i*0x0eu,at,bits(value));}if(stats)++stats->reversed_normals;}
    else{scope.suspend();native_tu15_publish_contact_body_property(memory,datum,row,properties,observe);scope.resume();if(stats)++stats->property_publications;}
    const auto magnitude=scalar(m.word(row+0x38u));bool mark=compare(magnitude,scalar(m.word(0x46a240u))).above();
    if(!mark)mark=(m.word(row+0x44u,1)&8u)!=0;
    if(!mark){const auto clock=m.word(0x51df54u),ticks=m.word(clock+8u);const auto frequency=std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(m.word(clock+2u,2))));const auto threshold=mul(integer_float(frequency),scalar(m.word(0x46a238u)));scope.suspend();std::int32_t rounded;
        try{rounded=integer_round(bits(threshold),fp,depth);}catch(...){if(stats)++stats->unsupported_fp_profiles;throw;}scope.resume();
        const auto previous=m.word(component+0x10u);mark=std::bit_cast<std::int32_t>(ticks-previous)<rounded;
    }
    if(mark){const auto own=m.word(component+8u),objects=m.word(0x4ecbd0u),headers=m.word(objects+0x44u),object=m.word(headers+(own&0xffffu)*12u+8u),at=object+0xc1u;const auto flags=m.word(at,1);m.store(0x1d799fu,at,flags|8u,1);if(stats)++stats->object_flags;}
    if(skip_impulse||!(m.word(row+0x44u,1)&0x20u))return;
    const auto part=std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(m.word(row+0x45u,1))))),body_bank=m.word(component+0x70u),record=body_bank+part*96u;
    if(m.word(record+0x44u,1)!=2u||((m.word(component+4u)>>18u)&1u))return;
    const auto vx=m.word(record+0x30u),vy=m.word(record+0x34u),selected_body=m.word(record+0x40u),vz=m.word(record+0x38u);
    const auto scale=scalar(m.word(0x456914u));
    const auto vector=_mm_set_ps(0.f,std::bit_cast<float>(vz),std::bit_cast<float>(vy),std::bit_cast<float>(vx));
    const auto scaled=mul_vector(broadcast(scale),vector),squared=mul_vector(scaled,scaled);auto length=add(_mm_shuffle_ps(squared,squared,0xaa),add(_mm_shuffle_ps(squared,squared,0x55),squared));
    if(!compare(length,scalar(m.word(0x46a454u))).above())return;
    const auto selected_motion=m.word(selected_body+0x3cu),selected_table=m.word(selected_motion);owner=m.word(selected_table+0x58u);
    if(stats)++stats->point_velocity_queries;scope.suspend();
    if(owner==0x323a10u)native_tu15_contact_motion_point_velocity(memory,selected_motion,point,scratch.point_velocity,fp,observe);
    else{if(!services.point_velocity)missing(owner);services.point_velocity(selected_motion,{owner},point,scratch.point_velocity,fp,depth);}scope.resume();
    length=add(_mm_shuffle_ps(squared,squared,0xaa),add(_mm_shuffle_ps(squared,squared,0x55),squared));
    auto estimate=length;asm volatile("rsqrtss %1,%0":"+x"(estimate):"x"(length):"memory");
    auto correction=scalar(m.word(0x46a0e4u));auto term=mul(mul(length,estimate),estimate);correction=sub(correction,term);
    auto normalization=mul(scalar(m.word(0x46a0ccu)),estimate);normalization=mul(normalization,correction);const auto direction=mul_vector(broadcast(normalization),scaled);
    const auto relative=mul_vector(m.vector(scratch.point_velocity),direction),projection=add(_mm_shuffle_ps(relative,relative,0xaa),add(_mm_shuffle_ps(relative,relative,0x55),relative));
    auto impulse=sub_vector(scaled,mul_vector(broadcast(projection),direction));
    const auto mass_motion=m.word(selected_body+0x3cu);const auto mass_inverse=scalar(m.word(mass_motion+0x2cu));auto mass=zero;
    if(!compare(mass_inverse,zero,true).ordered_zero())mass=divide(scalar(m.word(0x46a0d0u)),mass_inverse);
    const auto factor=m.word(0x46a16cu);scope.suspend();std::uint32_t weight;
    try{weight=extended_scale(bits(mass),factor,fp,depth);}catch(...){if(stats)++stats->unsupported_fp_profiles;throw;}scope.resume();
    impulse=mul_vector(broadcast(scalar(weight)),impulse);m.vector(0x1d7b46u,scratch.impulse,impulse);
    if(!services.apply_point_impulse)missing(0x1e1170u);if(stats)++stats->impulse_requests;scope.suspend();services.apply_point_impulse({selected_body},scratch.impulse,point,fp,depth);scope.resume();
}
}
