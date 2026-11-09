#include "halo2/native_tu15_unit_equipment_profile.hpp"
#include <bit>
#include <stdexcept>
#include <xmmintrin.h>
#include <emmintrin.h>
namespace halo2 {namespace {
std::uint32_t read(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned width=4){
    std::array<std::byte,4>b{};m.read(p,std::span(b).first(width));std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(b[n])<<(n*8u);return value;
}
void store(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o,
    std::uint32_t pc,std::uint32_t p,std::uint32_t value,unsigned width,NativeTu15UnitEquipmentProfileStats*stats){
    std::array<std::byte,4>b{};for(unsigned n=0;n<width;++n)b[n]=std::byte(value>>(n*8u));
    SourcePhysicsContactRecordStore event{pc,p,{},{}};
    if(o){event.before.resize(width);m.read(p,event.before);event.written.assign(b.begin(),b.begin()+width);}
    m.write(p,std::span(b).first(width));if(stats)++stats->stores;if(o)o(event);
}
std::uint32_t subtract(std::uint32_t first,std::uint32_t second,NativeTu15PhysicsFrameFp&fp){
    if((fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))
        throw std::runtime_error("Native equipment profile requires masked SIMD arithmetic");
    const auto host=_mm_getcsr();_mm_setcsr(fp.mxcsr);
    auto a=_mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(first)));
    const auto b=_mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(second)));
    asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");
    const auto result=std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(a)));fp.mxcsr=_mm_getcsr();_mm_setcsr(host);return result;
}
template<class Service>void required(const Service&service,std::uint32_t owner,NativeTu15UnitEquipmentProfileStats*stats){
    if(service)return;if(stats)++stats->unbound_services;
    throw std::runtime_error("Native equipment profile requires actual child owner "+std::to_string(owner));
}
}
std::array<std::byte,0x44> source_unit_equipment_profile_bytes_tu15(const SourceUnitEquipmentProfileInputTu15&input){
    std::array<std::byte,0x44>out{};for(unsigned n=0;n<32;++n)out[n]=std::byte(std::uint8_t(input.name[n]));
    const auto put=[&](unsigned offset,std::uint32_t value,unsigned bytes){for(unsigned n=0;n<bytes;++n)out[offset+n]=std::byte(value>>(n*8u));};
    put(0x20,std::bit_cast<std::uint32_t>(input.health_damage),4);put(0x24,std::bit_cast<std::uint32_t>(input.shield_damage),4);
    for(unsigned n=0;n<2;++n){const auto&weapon=input.weapons[n];const auto offset=0x28u+n*12u;
        put(offset,weapon.source_tag_group.value_or(0x77656170u),4);put(offset+4u,weapon.definition,4);
        put(offset+8u,std::bit_cast<std::uint16_t>(weapon.rounds_loaded),2);put(offset+10u,std::bit_cast<std::uint16_t>(weapon.rounds_total),2);
        out[0x40u+n]=std::byte(input.grenades[n]);out[0x42u+n]=std::byte(input.source_tail[n]);}
    return out;
}
void native_tu15_apply_unit_equipment_profile(SourcePhysicsContactMemory&m,std::uint32_t unit,
    const NativeTu15UnitEquipmentProfileInput&input,bool clear_previous,bool mark_garbage,
    NativeTu15PhysicsFrameFp&fp,const NativeTu15UnitEquipmentProfileServices&services,
    const SourcePhysicsContactRecordObserver&o,NativeTu15UnitEquipmentProfileStats*stats){
    if(stats)++stats->calls;
    if(unit==0xffffffffu||!input.selected()){if(stats)++stats->absent_inputs;return;}
    const auto profile=input.data(),pool=read(m,0x4ecbd0u),headers=read(m,pool+0x44u),body=read(m,headers+(unit&0xffffu)*12u+8u);
    if(clear_previous){
        required(services.clear,0xcf4d0u,stats);if(stats)++stats->clears;services.clear(unit,fp);
        const auto one=read(m,0x46a0d0u);
        store(m,o,0xcf33fu,body+0xf0u,one,4,stats);store(m,o,0xcf347u,body+0xecu,one,4,stats);
        store(m,o,0xcf34fu,body+0x23eu,0,2,stats);
    }
    for(unsigned n=0;n<2;++n){
        const auto weapon_input=profile+0x28u+n*12u;if(read(m,weapon_input+4u)==0xffffffffu)continue;
        required(services.create,0xcf240u,stats);if(stats)++stats->weapon_requests;
        const auto weapon=services.create(unit,weapon_input,mark_garbage,fp);if(weapon==0xffffffffu)continue;
        if(stats)++stats->created;required(services.carry,0xcf5a0u,stats);
        if(services.carry(unit,weapon,fp)){if(stats)++stats->carried;}
        else{required(services.retire,0xba570u,stats);if(stats)++stats->retired;services.retire(weapon,fp);}
    }
    const auto shield=read(m,body+0xf0u),shield_damage=read(m,profile+0x24u);
    store(m,o,0xcf3adu,body+0xf0u,subtract(shield,shield_damage,fp),4,stats);
    const auto health=read(m,body+0xecu),health_damage=read(m,profile+0x20u);
    store(m,o,0xcf3c2u,body+0xecu,subtract(health,health_damage,fp),4,stats);
    const auto frag_added=read(m,profile+0x40u,1),frag=read(m,body+0x23eu,1),plasma=read(m,body+0x23fu,1);
    store(m,o,0xcf3dbu,body+0x23eu,frag+frag_added,1,stats);
    const auto plasma_added=read(m,profile+0x41u,1);
    store(m,o,0xcf3e6u,body+0x23fu,plasma+plasma_added,1,stats);
    if(stats)++stats->completed;
}
}
