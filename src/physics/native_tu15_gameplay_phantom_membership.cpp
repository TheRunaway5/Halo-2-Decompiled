#include "halo2/native_tu15_gameplay_phantom_membership.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory&memory_;const SourcePhysicsContactRecordObserver&observe_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):memory_(m),observe_(o){}
    std::uint32_t word(std::uint32_t address,unsigned width=4)const{
        std::array<std::byte,4>b{};memory_.read(address,std::span(b).first(width));
        std::uint32_t result{};for(unsigned n=0;n<width;++n)result|=std::to_integer<unsigned>(b[n])<<(n*8u);return result;
    }
    std::uint32_t signed_byte(std::uint32_t address)const{
        return std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(word(address,1)))));
    }
    void word(std::uint32_t pc,std::uint32_t address,std::uint32_t value,unsigned width=4){
        std::array<std::byte,4>b{};for(unsigned n=0;n<width;++n)b[n]=std::byte(value>>(n*8u));
        SourcePhysicsContactRecordStore event{pc,address,{},{}};
        if(observe_){event.before.resize(width);memory_.read(address,event.before);event.written.assign(b.begin(),b.begin()+width);}
        memory_.write(address,std::span(b).first(width));if(observe_)observe_(event);
    }
    std::uint32_t property(std::uint32_t body,std::uint32_t key,std::uint32_t count)const{
        if(std::bit_cast<std::int32_t>(count)<=0)return 0;
        const auto entries=word(body+0x30u);
        for(std::uint32_t n=0;std::bit_cast<std::int32_t>(n)<std::bit_cast<std::int32_t>(count);++n)
            if(word(entries+n*8u)==key)return word(entries+n*8u+4u);
        return 0;
    }
};
void release(const NativeTu15ArrayGrowthServices&s,const NativeTu15ArrayReleaseRequest&request,NativeTu15PhysicsFrameFp&fp){
    if(!s.release)throw std::runtime_error("Native phantom membership requires its actual release owner");
    s.release(request,fp);
}
}
std::uint32_t native_tu15_phantom_body_property_2001(SourcePhysicsContactMemory&memory,std::uint32_t body){
    const SourcePhysicsContactRecordObserver none;Memory m(memory,none);return m.property(body,0x2001u,m.word(body+0x34u));
}
std::uint32_t native_tu15_phantom_body_property_2002(SourcePhysicsContactMemory&memory,std::uint32_t body){
    const SourcePhysicsContactRecordObserver none;Memory m(memory,none);return m.property(body,0x2002u,m.word(body+0x34u));
}
void native_tu15_remove_gameplay_phantom_membership(SourcePhysicsContactMemory&memory,
    const NativeTu15PhantomMembershipLeave&input,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15ArrayGrowthServices&allocation,const SourcePhysicsContactRecordObserver&observe){
    Memory m(memory,observe);
    // The second lookup rereads backing, but both use the source's first
    // cached signed count. The standalone property helpers have a fresh count.
    const auto property_count=m.word(input.other_body+0x34u);
    const auto other_component=m.property(input.other_body,0x2001u,property_count);
    const auto other_slot=m.property(input.other_body,0x2002u,property_count);
    if(other_component==0xffffffffu||other_component==input.component_datum)return;
    const auto header=m.word(input.component+0x94u);if(!header)return;
    const auto pool=m.word(0x52be5cu),components=m.word(pool+0x44u);
    const auto other_object=m.word(components+(other_component&0xffffu)*0xa0u+8u);
    auto remaining=m.word(header+4u);
    if(std::bit_cast<std::int32_t>(remaining)>0){
        auto entry=m.word(header);std::uint32_t index{};
        for(;;){
            if(m.word(entry+4u)==other_object&&m.signed_byte(entry+3u)==input.body_slot&&
                m.signed_byte(entry+1u)==other_slot&&m.signed_byte(entry)==input.phantom_index){
                const auto references=(m.word(entry+2u,1)-1u)&255u;
                m.word(0x1da404u,entry+2u,references,1);
                if(!references){
                    const auto own_object=m.word(input.component+8u),objects=m.word(0x4ecbd0u);
                    const auto object_rows=m.word(objects+0x44u);
                    const auto own_body=m.word(object_rows+(own_object&0xffffu)*12u+8u);
                    const auto definition=m.word(own_body),tags=m.word(0x4f0414u);
                    const auto definition_data=m.word(tags+(definition&0xffffu)*16u+8u);
                    const auto model=m.word(definition_data+0x38u);
                    const auto model_data=m.word(tags+(model&0xffffu)*16u+8u);
                    const auto physics=m.word(model_data+0x24u);
                    const auto physics_data=m.word(tags+(physics&0xffffu)*16u+8u);
                    const auto phantom_rows=m.word(physics_data+0x44u);
                    const auto shape_index=std::bit_cast<std::int16_t>(std::uint16_t(m.word(phantom_rows+input.phantom_index*12u+8u,2)));
                    const auto shape_rows=m.word(physics_data+0x2cu);
                    const auto shape=shape_rows+std::uint32_t(std::int32_t(shape_index)*0x68);
                    const auto last=m.word(header+4u)-1u,data=m.word(header);
                    m.word(0x1da477u,header+4u,last);
                    const auto key=m.word(data+last*8u);m.word(0x1da47du,data+index*8u,key);
                    const auto value=m.word(data+last*8u+4u);m.word(0x1da484u,data+index*8u+4u,value);
                    if(m.word(shape)&0x1000000u){
                        const auto rows=m.word(objects+0x44u),object_header=rows+(other_object&0xffffu)*12u;
                        const auto type=m.word(object_header+3u,1);
                        if((1u<<(type&31u))&1u){
                            const auto body=m.word(object_header+8u);
                            const auto current_owner=m.word(body+0x3e4u),own=m.word(input.component+8u);
                            if(current_owner==own)m.word(0x1da4c2u,body+0x3e4u,0xffffffffu);
                        }
                    }
                }
                break;
            }
            const auto count=m.word(header+4u);++index;entry+=8u;
            if(std::bit_cast<std::int32_t>(index)>=std::bit_cast<std::int32_t>(count))break;
        }
        remaining=m.word(header+4u);
    }
    if(remaining)return;
    // Reread the owned header at the source release phase. Its identity then
    // remains cached across the two real releases, including callback changes.
    const auto owned=m.word(input.component+0x94u);
    if(owned){
        const auto capacity=m.word(owned+8u);
        if(std::bit_cast<std::int32_t>(capacity)>=0){
            const auto allocator=m.word(0x48c938u),table=m.word(allocator);
            const auto data=m.word(owned),owner=m.word(table+0x14u);
            release(allocation,{allocator,owner,data,(capacity&0x7fffffffu)<<3u,0x12u},fp);
        }
        const auto allocator=m.word(0x48c938u),table=m.word(allocator),owner=m.word(table+0x14u);
        release(allocation,{allocator,owner,owned,12u,0x12u},fp);
    }
    m.word(0x1da512u,input.component+0x94u,0);
}
void native_tu15_leave_gameplay_phantom(SourcePhysicsContactMemory&memory,
    std::uint32_t shape,std::uint32_t self_root,std::uint32_t other_root,
    NativeTu15PhysicsFrameFp&fp,const NativeTu15ArrayGrowthServices&allocation,
    const SourcePhysicsContactRecordObserver&observe){
    Memory m(memory,observe);
    const auto self_type=m.word(self_root+0x18u),phantom=m.word(shape+8u);
    const auto self=self_type==1u?m.word(self_root+0x20u):0u;
    const auto other=m.word(other_root+0x18u)==1u?m.word(other_root+0x20u):0u;
    if(!self||!other)return;
    const auto datum=native_tu15_phantom_body_property_2001(memory,self);if(datum==0xffffffffu)return;
    const auto slot=native_tu15_phantom_body_property_2002(memory,self);
    const auto pool=m.word(0x52be5cu),entries=m.word(pool+0x44u);
    native_tu15_remove_gameplay_phantom_membership(memory,
        {other,entries+(datum&0xffffu)*0xa0u,datum,slot,phantom},fp,allocation,observe);
}
void native_tu15_cleanup_phantom_callback_agent(SourcePhysicsContactMemory&memory,
    NativeTu15ContactAgent agent,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15PhantomCallbackCleanupServices&services){
    const SourcePhysicsContactRecordObserver none;Memory m(memory,none);
    const auto leave=[&](bool second){
        const auto other=m.word(agent.identity+(second?12u:16u));
        const auto shape=m.word(agent.identity+(second?24u:20u));
        const auto table=m.word(shape),self=m.word(agent.identity+(second?16u:12u));
        const NativeTu15ContactVirtualOwner owner{m.word(table+0x2cu)};
        if(!services.leave)throw std::runtime_error("Native phantom callback cleanup requires its actual leave owner");
        services.leave({shape},owner,{self},{other},fp);
    };
    if(m.word(agent.identity+28u)==22u)leave(false);
    if(m.word(agent.identity+32u)==22u)leave(true);
    native_tu15_dispose_contact_reference(memory,agent.identity,fp,services.agents);
}
}
