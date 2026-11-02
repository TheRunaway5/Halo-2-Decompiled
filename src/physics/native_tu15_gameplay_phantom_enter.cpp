#include "halo2/native_tu15_gameplay_phantom_enter.hpp"
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
    std::uint32_t physics(std::uint32_t body)const{
        const auto definition=word(body),tags=word(0x4f0414u);
        const auto definition_data=word(tags+(definition&0xffffu)*16u+8u);
        const auto model=word(definition_data+0x38u),model_data=word(tags+(model&0xffffu)*16u+8u);
        const auto physics=word(model_data+0x24u);return word(tags+(physics&0xffffu)*16u+8u);
    }
    std::uint32_t shape(std::uint32_t physics,std::uint32_t phantom)const{
        const auto phantoms=word(physics+0x44u);
        const auto index=std::bit_cast<std::int16_t>(std::uint16_t(word(phantoms+phantom*12u+8u,2)));
        const auto shapes=word(physics+0x2cu);return shapes+std::uint32_t(std::int32_t(index)*0x68);
    }
};
}
bool native_tu15_admit_gameplay_phantom_member(SourcePhysicsContactMemory&memory,
    const NativeTu15PhantomMembershipPolicy&input){
    if(input.other_component_datum==0xffffffffu)return false;
    const SourcePhysicsContactRecordObserver none;Memory m(memory,none);
    const auto pool=m.word(0x52be5cu),components=m.word(pool+0x44u);
    const auto own_object=m.word(components+(input.component_datum&0xffffu)*0xa0u+8u);
    const auto objects=m.word(0x4ecbd0u),object_rows=m.word(objects+0x44u);
    const auto own_body=m.word(object_rows+(own_object&0xffffu)*12u+8u);
    const auto physics=m.physics(own_body);
    // Source reads the other component after both authored shape tables.
    const auto phantoms=m.word(physics+0x44u);
    const auto index=std::bit_cast<std::int16_t>(std::uint16_t(m.word(phantoms+input.phantom_index*12u+8u,2)));
    const auto shapes=m.word(physics+0x2cu);
    const auto other_component=components+(input.other_component_datum&0xffffu)*0xa0u;
    const auto other_object=m.word(other_component+8u),shape=shapes+std::uint32_t(std::int32_t(index)*0x68);
    const auto other_header=object_rows+(other_object&0xffffu)*12u;
    const auto other_body=m.word(other_header+8u),group=m.signed_byte(other_component+0x1cu);
    const auto type=m.word(other_header+3u,1),mask=1u<<(type&31u);
    bool attached{};
    if(mask&3u){const auto current_body=m.word(other_header+8u);attached=m.word(current_body+0x13cu)!=0xffffffffu;}
    const bool flying=(mask&1u)?((m.word(other_body+0x10au,1)>>2u)&1u)!=0:false;
    if(group){
        if(std::bit_cast<std::int32_t>(m.signed_byte(shape+4u))>std::bit_cast<std::int32_t>(group))return false;
        if(std::bit_cast<std::int32_t>(m.signed_byte(shape+5u))<std::bit_cast<std::int32_t>(group))return false;
    }
    const auto flags=m.word(shape);
    if(flags&(attached?8u:16u))return false;
    if(flying&&(flags&0x8000000u))return false;
    return !(flags&(1u<<((type+5u)&31u)));
}
void native_tu15_add_gameplay_phantom_membership(SourcePhysicsContactMemory&memory,
    const NativeTu15PhantomMembershipEnter&input,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15ArrayGrowthServices&allocation,const SourcePhysicsContactRecordObserver&observe){
    Memory m(memory,observe);const auto property_count=m.word(input.other_body+0x34u);
    const auto other_component=m.property(input.other_body,0x2001u,property_count);
    const auto other_slot=m.property(input.other_body,0x2002u,property_count);
    if(other_component==0xffffffffu||other_component==input.component_datum)return;
    if(!native_tu15_admit_gameplay_phantom_member(memory,{input.component_datum,other_component,input.phantom_index}))return;
    const auto pool=m.word(0x52be5cu),components=m.word(pool+0x44u);
    const auto other_object=m.word(components+(other_component&0xffffu)*0xa0u+8u);
    if(!m.word(input.component+0x94u)){
        const auto allocator=m.word(0x48c938u),table=m.word(allocator),owner=m.word(table+0x10u);
        if(!allocation.allocate)throw std::runtime_error("Native phantom enter requires its actual allocation owner");
        const auto header=allocation.allocate({allocator,owner,12u,0x12u},fp);
        if(header){
            m.word(0x1da1abu,header,0);m.word(0x1da1b1u,header+4u,0);
            m.word(0x1da1b8u,header+8u,0x80000000u);
        }
        m.word(0x1da1d9u,input.component+0x94u,header);
    }
    const auto header=m.word(input.component+0x94u),initial_count=m.word(header+4u);
    std::uint32_t record{};bool found{};
    if(std::bit_cast<std::int32_t>(initial_count)>0){
        auto entry=m.word(header);std::uint32_t n{};
        do{
            if(m.word(entry+4u)==other_object&&m.signed_byte(entry+3u)==input.body_slot&&
               m.signed_byte(entry+1u)==other_slot&&m.signed_byte(entry)==input.phantom_index){record=entry;found=true;break;}
            ++n;entry+=8u;
            if(std::bit_cast<std::int32_t>(n)>=std::bit_cast<std::int32_t>(m.word(header+4u)))break;
        }while(true);
    }
    if(!found){
        const auto own_object=m.word(input.component+8u),objects=m.word(0x4ecbd0u),rows=m.word(objects+0x44u);
        const auto own_body=m.word(rows+(own_object&0xffffu)*12u+8u),physics=m.physics(own_body);
        const auto shape=m.shape(physics,input.phantom_index),capacity=m.word(header+8u)&0x7fffffffu;
        if(initial_count==capacity){
            NativeTu15ArrayGrowthMemory array(memory,{header},observe);
            native_tu15_grow_array(array,8u,fp,allocation);
        }
        const auto count=m.word(header+4u),data=m.word(header);record=data+count*8u;
        m.word(0x1da2aau,header+4u,count+1u);
        m.word(0x1da2b1u,record+3u,input.body_slot,1);m.word(0x1da2b4u,record,input.phantom_index,1);
        m.word(0x1da2b6u,record+1u,other_slot,1);m.word(0x1da2b9u,record+2u,0,1);
        m.word(0x1da2bdu,record+4u,other_object);
        if(m.word(shape)&0x1000000u){
            const auto current_objects=m.word(0x4ecbd0u),current_rows=m.word(current_objects+0x44u);
            const auto other_header=current_rows+(other_object&0xffffu)*12u;
            if((1u<<(m.word(other_header+3u,1)&31u))&1u){
                const auto body=m.word(other_header+8u);
                if(m.word(body+0x3e4u)==0xffffffffu){
                    const auto own=m.word(input.component+8u);m.word(0x1da301u,body+0x3e4u,own);
                }
            }
        }
    }
    const auto references=(m.word(record+2u,1)+1u)&255u;m.word(0x1da307u,record+2u,references,1);
}
void native_tu15_enter_gameplay_phantom(SourcePhysicsContactMemory&memory,
    std::uint32_t shape,std::uint32_t self_root,std::uint32_t other_root,
    std::uint32_t,NativeTu15PhysicsFrameFp&fp,const NativeTu15ArrayGrowthServices&allocation,
    const SourcePhysicsContactRecordObserver&observe){
    Memory m(memory,observe);const auto self_type=m.word(self_root+0x18u),phantom=m.word(shape+8u);
    const auto self=self_type==1u?m.word(self_root+0x20u):0u;
    const auto other=m.word(other_root+0x18u)==1u?m.word(other_root+0x20u):0u;
    if(!self||!other)return;
    const auto datum=native_tu15_phantom_body_property_2001(memory,self);if(datum==0xffffffffu)return;
    const auto slot=native_tu15_phantom_body_property_2002(memory,self);
    const auto pool=m.word(0x52be5cu),entries=m.word(pool+0x44u);
    native_tu15_add_gameplay_phantom_membership(memory,
        {other,entries+(datum&0xffffu)*0xa0u,datum,slot,phantom},fp,allocation,observe);
}
}
