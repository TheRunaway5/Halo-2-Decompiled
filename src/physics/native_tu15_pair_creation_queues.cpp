#include "halo2/native_tu15_pair_creation_queues.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory& memory_;
    const SourcePhysicsContactRecordObserver& observe_;
public:
    Memory(SourcePhysicsContactMemory& memory,const SourcePhysicsContactRecordObserver& observe)
        :memory_(memory),observe_(observe){}
    std::uint32_t word(std::uint32_t address)const{
        std::array<std::byte,4> bytes{};memory_.read(address,bytes);std::uint32_t value{};
        for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(bytes[n])<<(8u*n);
        return value;
    }
    void word(std::uint32_t site,std::uint32_t address,std::uint32_t value){
        std::array<std::byte,4> bytes{};for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(8u*n));
        SourcePhysicsContactRecordStore event{site,address,{},{}};
        if(observe_){event.before.resize(4);memory_.read(address,event.before);event.written.assign(bytes.begin(),bytes.end());}
        memory_.write(address,bytes);if(observe_)observe_(event);
    }
};
bool less(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b);}
}
void native_tu15_queue_body_pair_merge(SourcePhysicsContactMemory& memory,std::uint32_t world,
    const NativeTu15BodyPairInputs& inputs,NativeTu15PhysicsFrameFp& fp,
    const NativeTu15ArrayGrowthServices& allocation,const SourcePhysicsContactRecordObserver& observe){
    Memory m(memory,observe);const auto count=m.word(world+0x24u),capacity=m.word(world+0x28u)&0x7fffffffu;
    const auto needed=count+1u,header=world+0x20u;
    if(less(capacity,needed)){
        const auto doubled=capacity*2u,selected=less(needed,doubled)?doubled:needed;
        native_tu15_reserve_array(memory,{header},selected,8u,fp,allocation,observe);
    }
    if(!inputs.first)throw std::runtime_error("Native TU1.5 body merge queue requires its current first body input");
    const auto first=inputs.first();m.word(0x317230u,header+4u,needed);
    const auto first_count=m.word(world+0x24u),first_data=m.word(header);
    m.word(0x317238u,first_data+first_count*8u-8u,first);
    const auto second_data=m.word(header),second_count=m.word(world+0x24u);
    if(!inputs.second)throw std::runtime_error("Native TU1.5 body merge queue requires its current second body input");
    const auto second=inputs.second();m.word(0x317247u,second_data+second_count*8u-4u,second);
}
void native_tu15_queue_body_pair_merge(SourcePhysicsContactMemory& memory,std::uint32_t world,
    std::uint32_t first,std::uint32_t second,NativeTu15PhysicsFrameFp& fp,
    const NativeTu15ArrayGrowthServices& allocation,const SourcePhysicsContactRecordObserver& observe){
    native_tu15_queue_body_pair_merge(memory,world,{[first]{return first;},[second]{return second;}},fp,allocation,observe);
}
void native_tu15_defer_collision_pair(SourcePhysicsContactMemory& memory,std::uint32_t island,
    const std::function<std::uint32_t()>& input,NativeTu15PhysicsFrameFp& fp,
    const NativeTu15ArrayGrowthServices& allocation,const SourcePhysicsContactRecordObserver& observe){
    Memory m(memory,observe);const auto capacity=m.word(island+0x7cu)&0x7fffffffu,header=island+0x74u;
    const auto count=m.word(header+4u);
    if(count==capacity){NativeTu15ArrayGrowthMemory array(memory,{header},observe);native_tu15_grow_array(array,8u,fp,allocation);}
    const auto current_count=m.word(header+4u);
    if(!input)throw std::runtime_error("Native TU1.5 deferred collision pair requires its current input");
    const auto pair=input(),first=m.word(pair),data=m.word(header);
    m.word(0x320dcau,data+current_count*8u,first);
    const auto second=m.word(pair+4u);m.word(0x320dd0u,data+current_count*8u+4u,second);
    const auto after=m.word(header+4u);m.word(0x320dd9u,header+4u,after+1u);
}
void native_tu15_defer_collision_pair(SourcePhysicsContactMemory& memory,std::uint32_t island,
    std::uint32_t pair,NativeTu15PhysicsFrameFp& fp,const NativeTu15ArrayGrowthServices& allocation,
    const SourcePhysicsContactRecordObserver& observe){
    native_tu15_defer_collision_pair(memory,island,[pair]{return pair;},fp,allocation,observe);
}
} // namespace halo2
