#include "halo2/native_tu15_body_properties.hpp"
#include <bit>

namespace halo2 {
namespace {
class Properties {
    SourcePhysicsContactMemory& memory_;
    const SourcePhysicsContactRecordObserver& observer_;
public:
    Properties(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o)
        :memory_(m),observer_(o){}
    std::uint32_t read(std::uint32_t address) const {
        std::array<std::byte,4> bytes{};memory_.read(address,bytes);
        std::uint32_t value{};for(unsigned n=0;n<4;++n)
            value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);
        return value;
    }
    void write(std::uint32_t pc,std::uint32_t address,std::uint32_t value){
        std::array<std::byte,4> bytes{};
        for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(n*8u));
        SourcePhysicsContactRecordStore event{pc,address,{},{}};
        if(observer_){event.before.resize(4);memory_.read(address,event.before);
            event.written.assign(bytes.begin(),bytes.end());}
        memory_.write(address,bytes);if(observer_)observer_(event);
    }
    static bool less(std::uint32_t a,std::uint32_t b){
        return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b);
    }
    void remove(std::uint32_t body,std::uint32_t property,std::uint32_t output){
        const auto count=read(body+0x34u);std::uint32_t index{},entries{};
        bool found{};
        if(less(0,count)){
            entries=read(body+0x30u);
            do{if(read(entries+index*8u)==property){found=true;break;}++index;}
            while(less(index,count));
        }
        if(!found){write(0x31c483u,output,0);return;}
        const auto current_count=read(body+0x34u);
        // Source31C493 loads the removed key before its value, even though the
        // key is only copied to a dead private local afterward.
        const auto removed_key=read(entries+index*8u);(void)removed_key;
        const auto value=read(entries+index*8u+4u),last=current_count-1u;
        write(0x31c4a3u,body+0x34u,last);
        if(less(index,last))do{
            const auto current_entries=read(body+0x30u),entry=current_entries+index*8u;
            const auto next_key=read(entry+8u);write(0x31c4bau,entry,next_key);
            const auto next_value=read(entry+12u);write(0x31c4bfu,entry+4u,next_value);
            const auto current_end=read(body+0x34u);++index;
            if(!less(index,current_end))break;
        }while(true);
        write(0x31c4ceu,output,value);
    }
    void add(std::uint32_t body,std::uint32_t property,std::uint32_t value,
        NativeTu15PhysicsFrameFp&fp,const NativeTu15ArrayGrowthServices&allocation){
        const auto count=read(body+0x34u);std::uint32_t index{};
        if(less(0,count)){
            const auto entries=read(body+0x30u);
            do{if(read(entries+index*8u)==property)return;++index;}
            while(less(index,count));
        }
        const auto capacity=read(body+0x38u),header=body+0x30u;
        if(read(header+4u)==(capacity&0x7fffffffu)){
            NativeTu15ArrayGrowthMemory array(memory_,{header},observer_);
            native_tu15_grow_array(array,8u,fp,allocation);
        }
        const auto current_count=read(header+4u),entries=read(header);
        write(0x31c625u,entries+current_count*8u,property);
        write(0x31c628u,entries+current_count*8u+4u,value);
        write(0x31c62cu,header+4u,read(header+4u)+1u);
    }
    void replace(std::uint32_t body,std::uint32_t value,std::uint32_t output,
        NativeTu15PhysicsFrameFp&fp,const NativeTu15ArrayGrowthServices&allocation){
        const auto count=read(body+0x34u);std::uint32_t index{};
        if(less(0,count)){
            const auto entries=read(body+0x30u);
            do{if(read(entries+index*8u)==0x2003u){remove(body,0x2003u,output);break;}++index;}
            while(less(index,count));
        }
        add(body,0x2003u,value,fp,allocation);
    }
};
} // namespace
void native_tu15_remove_body_property(SourcePhysicsContactMemory&m,std::uint32_t body,
    std::uint32_t property,std::uint32_t output,const SourcePhysicsContactRecordObserver&o){
    Properties(m,o).remove(body,property,output);
}
void native_tu15_add_body_property(SourcePhysicsContactMemory&m,std::uint32_t body,
    std::uint32_t property,std::uint32_t value,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15ArrayGrowthServices&s,const SourcePhysicsContactRecordObserver&o){
    Properties(m,o).add(body,property,value,fp,s);
}
void native_tu15_replace_part_contact_property(SourcePhysicsContactMemory&m,
    std::uint32_t body,std::uint32_t value,std::uint32_t output,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15ArrayGrowthServices&s,const SourcePhysicsContactRecordObserver&o){
    Properties(m,o).replace(body,value,output,fp,s);
}
} // namespace halo2
