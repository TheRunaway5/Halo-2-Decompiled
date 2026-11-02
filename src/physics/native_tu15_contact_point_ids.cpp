#include "halo2/native_tu15_contact_point_ids.hpp"

namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory&memory_;
    const SourcePhysicsContactRecordObserver&observe_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o)
        :memory_(m),observe_(o){}
    std::uint32_t read(std::uint32_t p,unsigned n=4)const {
        std::array<std::byte,4>b{};memory_.read(p,std::span(b).first(n));
        std::uint32_t value{};
        for(unsigned k=0;k<n;++k)value|=std::to_integer<unsigned>(b[k])<<(8u*k);
        return value;
    }
    void write(std::uint32_t pc,std::uint32_t p,std::uint32_t value,unsigned n=4){
        std::array<std::byte,4>b{};
        for(unsigned k=0;k<n;++k)b[k]=std::byte(value>>(8u*k));
        SourcePhysicsContactRecordStore event{pc,p,{},{}};
        if(observe_){event.before.resize(n);memory_.read(p,event.before);}
        memory_.write(p,std::span(b).first(n));
        if(observe_){event.written.assign(b.begin(),b.begin()+n);observe_(event);}
    }
};
} // namespace

std::uint32_t native_tu15_allocate_contact_point_id(SourcePhysicsContactMemory&memory,
    NativeTu15ArrayHeader header,std::uint32_t index,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15ArrayGrowthServices&services,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15ContactPointIdStats*stats){
    if(stats)++stats->attempts;
    try {
        Memory m(memory,observe);
        const auto initial_count=m.read(header.identity+4u);
        auto candidate=initial_count-1u;
        if(!(candidate&0x80000000u)){
            const auto data=m.read(header.identity);
            do {
                if(m.read(data+candidate,1)==0xffu){
                    m.write(0x325f71u,data+candidate,index,1);
                    if(stats){++stats->reused;++stats->completed;}
                    return candidate;
                }
                --candidate;
            }while(!(candidate&0x80000000u));
        }
        const auto capacity=m.read(header.identity+8u)&0x7fffffffu;
        if(initial_count==capacity){
            NativeTu15ArrayGrowthMemory storage(memory,header,observe);
            native_tu15_grow_array(storage,1u,fp,services);
            if(stats)++stats->growths;
        }
        const auto count=m.read(header.identity+4u),data=m.read(header.identity);
        m.write(0x325f59u,data+count,index,1);
        const auto next_count=m.read(header.identity+4u)+1u;
        m.write(0x325f60u,header.identity+4u,next_count);
        if(stats){++stats->appended;++stats->completed;}
        return next_count-1u;
    }catch(...){if(stats)++stats->refusals;throw;}
}
} // namespace halo2
