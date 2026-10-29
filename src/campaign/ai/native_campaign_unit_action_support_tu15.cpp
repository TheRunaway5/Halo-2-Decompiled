#include "halo2/native_campaign_unit_action_support_tu15.hpp"
#include <array>
#include <bit>
#include <sstream>
#include <stdexcept>
namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned n=4){std::array<std::byte,4>b{};m.read(p,std::span(b).first(n));std::uint32_t v{};for(unsigned i=0;i<n;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(i*8u);return v;}
void write(SourcePhysicsContactMemory&m,std::uint32_t p,std::uint32_t v,unsigned n=4){std::array<std::byte,4>b{};for(unsigned i=0;i<n;++i)b[i]=std::byte(v>>(i*8u));m.write(p,std::span(b).first(n));}
std::int16_t signed16(std::uint32_t v){return std::bit_cast<std::int16_t>(std::uint16_t(v));}
std::uint32_t object_offset(std::uint32_t datum){return (datum&0xffffu)*12u;}
// Actual8FDB0 is RET8 with no data/FP operations. It returns no defined
// result to the interrupt caller, which ignores volatile return registers.
void empty_interrupt(std::uint32_t,std::uint32_t){}
}
void native_campaign_unit_mark_update_tu15(SourcePhysicsContactMemory&m,std::uint32_t unit){
    const auto objects=read(m,0x4ecbd0u);auto rows=read(m,objects+0x44u);
    if((read(m,rows+object_offset(unit)+2u,1)&2u)||unit==0xffffffffu)return;
    do{
        const auto offset=object_offset(unit),header=rows+offset+2u;
        write(m,header,read(m,header,1)|2u,1);
        rows=read(m,objects+0x44u);const auto body=read(m,rows+offset+8u);unit=read(m,body+0x14u);
    }while(unit!=0xffffffffu);
}
void native_campaign_unit_interrupt_action_tu15(SourcePhysicsContactMemory&m,std::uint32_t unit,
        std::uint32_t type,const NativeCampaignUnitInterruptObserverTu15&observer){
    const auto objects=read(m,0x4ecbd0u),rows=read(m,objects+0x44u),body=read(m,rows+object_offset(unit)+8u);
    const auto component=body+std::uint32_t(std::int32_t(signed16(read(m,body+0x346u,2))));
    const auto descriptor=read(m,0x473d28u+type*4u),callback=read(m,descriptor+0xcu);
    if(callback){
        if(callback!=0x8fdb0u){std::ostringstream why;why<<"Native unit action interrupt missing callback at e8f41 -> "<<std::hex<<callback;throw std::runtime_error(why.str());}
        empty_interrupt(unit,type);if(observer)observer({callback,unit,type});
    }
    const auto bank=std::uint32_t(std::bit_cast<std::int32_t>(type)>>5u);
    const auto target=component+bank*4u+4u;write(m,target,read(m,target)&~(1u<<(type&31u)));
}
void native_campaign_unit_record_action_tu15(SourcePhysicsContactMemory&m,std::uint32_t history,
        const NativeCampaignUnitActionArgumentsTu15&arguments){
    auto oldest=std::bit_cast<std::int32_t>(read(m,history));unsigned selected{};
    for(unsigned i=1;i<4;++i){const auto tick=std::bit_cast<std::int32_t>(read(m,history+i*8u));if(tick<oldest){oldest=tick;selected=i;}}
    const auto clock=read(m,0x51df54u),tick=read(m,clock+8u),slot=history+selected*8u;
    write(m,slot,tick);
    if(!arguments.type)throw std::runtime_error("Native action notification type input unavailable");
    write(m,slot+4u,arguments.type(),2);
    if(!arguments.result)throw std::runtime_error("Native action notification result input unavailable");
    write(m,slot+6u,arguments.result(),2);
}
void native_campaign_unit_notify_action_tu15(SourcePhysicsContactMemory&m,std::uint32_t player,
        const NativeCampaignUnitActionArgumentsTu15&arguments){
    const auto players=read(m,0x4f54f4u),rows=read(m,players+0x44u);
    const auto local=signed16(read(m,rows+(player&0xffffu)*0x21cu+0x28u,2));if(local==-1)return;
    const auto views=read(m,0x52be64u),history=views+std::uint32_t(std::int32_t(local))*0x1b0u+0x150u;
    native_campaign_unit_record_action_tu15(m,history,arguments);
}
}
