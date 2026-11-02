#include "halo2/native_tu15_reversed_query_process.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory& memory_;
    const SourcePhysicsContactRecordObserver& observe_;
public:
    Memory(SourcePhysicsContactMemory& m,
        const SourcePhysicsContactRecordObserver& o):memory_(m),observe_(o){}
    std::uint32_t word(std::uint32_t p) const {
        std::array<std::byte,4> b{};memory_.read(p,b);
        std::uint32_t result{};
        for(unsigned k=0;k<4;++k)
            result|=std::to_integer<unsigned>(b[k])<<(k*8u);
        return result;
    }
    std::array<std::byte,16> vector(std::uint32_t p) const {
        if(p&15u)
            throw std::runtime_error("Native reversed query requires aligned contact vectors");
        std::array<std::byte,16> b{};memory_.read_vector(p,b);return b;
    }
    void vector(std::uint32_t p,const std::array<std::byte,16>& b) {
        if(p&15u)
            throw std::runtime_error("Native reversed query requires aligned contact vectors");
        SourcePhysicsContactRecordStore event{0x2f1a97u,p,{},{}};
        if(observe_){event.before.resize(b.size());memory_.read(p,event.before);}
        memory_.write(p,b);
        if(observe_){event.written.assign(b.begin(),b.end());observe_(event);}
    }
};
bool less(std::uint32_t a,std::uint32_t b) {
    return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b);
}
}
void native_tu15_process_reversed_query_agent(SourcePhysicsContactMemory& memory,
    NativeTu15IslandCollisionAgent agent,NativeTu15PhysicsBody first,
    NativeTu15PhysicsBody second,NativeTu15PhysicsStepConfiguration configuration,
    NativeTu15IslandContactOutput output,NativeTu15PhysicsFrameFp& fp,
    const NativeTu15CompoundProcessServices& services,
    const SourcePhysicsContactRecordObserver& observe,
    NativeTu15ReversedQueryProcessStats* stats) {
    if(stats)++stats->calls;
    try {
        Memory m(memory,observe);
        const auto initial_header=m.word(output.identity);
        auto index=m.word(initial_header+4u);
        if(!services.process_child)
            throw std::runtime_error("Native reversed query requires actual2F0F10 processing");
        if(stats)++stats->child_calls;
        services.process_child({agent,{0x2f0f10u},{second.identity},
            {first.identity},configuration,output},fp);
        if(stats)++stats->child_returns;
        auto header=m.word(output.identity);
        auto count=m.word(header+4u);
        auto offset=index*48u;
        while(less(index,count)) {
            const auto data=m.word(header);
            auto vector=m.vector(data+offset+16u);
            const auto mask=m.vector(0x461960u);
            for(unsigned lane=0;lane<16;++lane)vector[lane]^=mask[lane];
            m.vector(data+offset+16u,vector);
            if(stats)++stats->flipped_records;
            header=m.word(output.identity);
            count=m.word(header+4u);
            ++index;offset+=48u;
        }
        if(stats)++stats->completed;
    } catch(...) {
        if(stats)++stats->refusals;
        throw;
    }
}
} // namespace halo2
