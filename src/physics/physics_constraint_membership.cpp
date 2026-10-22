#include "halo2/physics_constraint_membership.hpp"
#include "halo2/physics_body_state.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
using M=SourcePhysicsContactMemory;using O=SourcePhysicsContactRecordObserver;
using S=SourcePhysicsContactBatchServices;
std::uint32_t r(const M&m,std::uint32_t p,unsigned n=4) {
    std::array<std::byte,4>b{};m.read(p,std::span(b).first(n));std::uint32_t x=0;
    for(unsigned k=0;k<n;++k)x|=std::to_integer<std::uint32_t>(b[k])<<(k*8u);
    return x;
}
std::int32_t si(std::uint32_t x){return std::bit_cast<std::int32_t>(x);}
void w(M&m,const O&o,std::uint32_t site,std::uint32_t p,unsigned n,std::uint32_t x) {
    SourcePhysicsContactRecordStore a{site,p,{},{}};a.before.resize(n);a.written.resize(n);m.read(p,a.before);
    for(unsigned k=0;k<n;++k)a.written[k]=std::byte((x>>(k*8u))&255u);
    m.write(p,a.written);if(o)o(a);
}
std::uint32_t required(M&m,const S&s,std::uint32_t site,std::uint32_t entry,
    std::optional<std::uint32_t> ecx,std::uint32_t esp,std::vector<std::uint32_t>args,const O&o) {
    if(!s.required_owner)throw std::runtime_error("Required original constraint/world membership owner unavailable");
    return s.required_owner(m,{site,entry,esp,ecx,std::move(args)},o);
}
}
void source_queue_physics_island_merge_1_1(M&m,std::uint32_t world,
    std::uint32_t first,std::uint32_t second,const O&o) {
    const auto bank=world+0x20u,count=r(m,bank+4)+1u,capacity=r(m,bank+8)&0x7fffffffu;
    if(si(capacity)<si(count))source_grow_physics_body_bank_1_1(
        m,bank,si(count)<si(capacity*2u)?capacity*2u:count,8,o);
    w(m,o,0x3136e0,bank+4,4,count);
    w(m,o,0x3136e8,r(m,bank)+r(m,bank+4)*8u-8u,4,first);
    w(m,o,0x3136f7,r(m,bank)+r(m,bank+4)*8u-4u,4,second);
}
void source_physics_constraint_requirements_1_1(M&m,std::uint32_t constraint,std::uint32_t output,const O&o) {
    w(m,o,0x322326,output,4,0);w(m,o,0x322328,output+4,4,0);w(m,o,0x32232b,output+12,4,0);w(m,o,0x32232e,output+8,4,24);
    const auto count=r(m,constraint+0x94),next=count+2u;
    auto bytes=(next*3u)<<4u,indices=count*4u+0x2cu;
    w(m,o,0x322348,output,4,((count*3u)<<4u)+0x80u);
    w(m,o,0x32235e,output+4,4,bytes);w(m,o,0x322361,output+8,4,indices);w(m,o,0x322364,output+12,4,next);
    if(si(count)>=2){bytes+=32;indices+=4;w(m,o,0x322370,output+4,4,bytes);w(m,o,0x322373,output+8,4,indices);w(m,o,0x322376,output+12,4,next+1u);}
}
void source_physics_ragdoll_constraint_requirements_1_1(M&m,std::uint32_t output,const O&o) {
    w(m,o,0x319ac4,output,4,0);w(m,o,0x319aca,output+8,4,0x44);
    w(m,o,0x319ad1,output+4,4,0x110);w(m,o,0x319ad8,output+12,4,7);
}
void source_physics_hinge_constraint_requirements_1_1(M&m,std::uint32_t constraint,std::uint32_t output,const O&o) {
    w(m,o,0x31909a,output,4,0);w(m,o,0x3190a0,output+8,4,0x54);
    w(m,o,0x3190a7,output+4,4,0xf0);w(m,o,0x3190ae,output+12,4,6);
    if(std::bit_cast<float>(r(m,constraint+0x94))!=std::bit_cast<float>(r(m,0x465048))) {
        w(m,o,0x3190c4,output+8,4,0x6c);w(m,o,0x3190cb,output+4,4,0x150);
        w(m,o,0x3190d2,output+12,4,9);
    }
}
void source_accumulate_physics_constraint_requirements_1_1(M&m,std::uint32_t island,std::uint32_t constraint,std::uint32_t requirements,const O&o) {
    (void)constraint;auto x=r(m,requirements),prior=r(m,island+8);if(si(prior)>si(x))x=prior;
    w(m,o,0x31c530,island+8,4,x);const auto next=r(m,requirements+4);if(si(x)<=si(next))x=next;
    auto sum=r(m,island+12);w(m,o,0x31c53f,island+8,4,x);sum+=r(m,requirements+4);w(m,o,0x31c547,island+12,4,sum);
    x=r(m,requirements+8);sum=r(m,island+16)+x;x=r(m,island+24);w(m,o,0x31c555,island+16,4,sum);
    x+=r(m,requirements+12);w(m,o,0x31c55d,island+24,4,x);
}
void source_insert_physics_constraint_into_island_1_1(M&m,std::uint32_t island,std::uint32_t constraint,std::uint32_t esp,const S&s,const O&o) {
    const auto frame=esp-28u,bank=island+0x58;
    if(r(m,bank+4)==(r(m,bank+8)&0x7fffffffu))source_double_physics_body_bank_1_1(m,bank,4,o);
    w(m,o,0x31d2de,r(m,bank)+r(m,bank+4)*4u,4,constraint);w(m,o,0x31d2e9,bank+4,4,r(m,bank+4)+1u);
    const auto requirements=frame+12u,method=r(m,r(m,constraint)+40);
    if(method==0x322320)source_physics_constraint_requirements_1_1(m,constraint,requirements,o);
    else if(method==0x319ac0)source_physics_ragdoll_constraint_requirements_1_1(m,requirements,o);
    else if(method==0x319090)source_physics_hinge_constraint_requirements_1_1(m,constraint,requirements,o);
    else required(m,s,0x31d2f1,method,constraint,frame-8u,{requirements},o);
    const auto accumulate=r(m,r(m,island)+8);
    if(accumulate==0x31c520)source_accumulate_physics_constraint_requirements_1_1(m,island,constraint,requirements,o);
    else required(m,s,0x31d2fe,accumulate,island,frame-12u,{constraint,requirements},o);
    w(m,o,0x31d301,constraint+6,2,r(m,constraint+6,2)+1u);w(m,o,0x31d306,constraint+20,4,island);
}
void source_notify_physics_constraint_added_1_1(M&m,std::uint32_t world,std::uint32_t constraint,std::uint32_t esp,const S&s,const O&o) {
    auto index=r(m,world+0x64)-1u;
    while(si(index)>=0) {
        const auto listener=r(m,r(m,world+0x60)+index*4u);
        if(listener)required(m,s,0x31e7ee,r(m,r(m,listener)+8),listener,esp-20u,{constraint},o);
        --index;
    }
    index=r(m,world+0x64)-1u;
    while(si(index)>=0) {
        if(!r(m,r(m,world+0x60)+index*4u)) {
            const auto count=r(m,world+0x64)-1u;w(m,o,0x31e811,world+0x64,4,count);
            auto k=index;while(si(k)<si(r(m,world+0x64))) {
                const auto bank=r(m,world+0x60),value=r(m,bank+k*4u+4u);w(m,o,0x31e82a,bank+k*4u,4,value);++k;
            }
        }
        --index;
    }
}
std::uint32_t source_insert_physics_constraint_into_world_1_1(M&m,std::uint32_t world,std::uint32_t constraint,std::uint32_t esp,const S&s,const O&o) {
    const auto body_b=r(m,constraint+16),island_b=r(m,body_b+68),body_a=r(m,constraint+12),island_a=r(m,body_a+68),fixed=r(m,world+44);
    const auto frame=esp-16u;
    if(island_a==fixed) {
        source_insert_physics_constraint_into_island_1_1(m,island_b,constraint,frame-8u,s,o);
        if(!r(m,island_b+41,1))required(m,s,0x313852,0x3131c0,world,frame-8u,{island_b},o);
    }else {
        if(island_b!=fixed&&island_a!=island_b)source_queue_physics_island_merge_1_1(m,world,body_a,body_b,o);
        source_insert_physics_constraint_into_island_1_1(m,island_a,constraint,frame-8u,s,o);
        if(!r(m,island_a+41,1))required(m,s,0x313852,0x3131c0,world,frame-8u,{island_a},o);
    }
    source_notify_physics_constraint_added_1_1(m,world,constraint,frame-12u,s,o);return constraint;
}
} // namespace halo2
