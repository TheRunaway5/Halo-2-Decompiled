#include "halo2/native_tu15_main_heap_release.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory&memory_;
    const SourcePhysicsContactRecordObserver&observer_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):memory_(m),observer_(o){}
    std::uint32_t word(std::uint32_t p)const{
        std::array<std::byte,4>b{};memory_.read(p,b);std::uint32_t v{};
        for(unsigned n=0;n<4;++n)v|=std::to_integer<unsigned>(b[n])<<(n*8u);return v;
    }
    void word(std::uint32_t site,std::uint32_t p,std::uint32_t value){
        SourcePhysicsContactRecordStore event{site,p,{},{}};
        if(observer_){event.before.resize(4);memory_.read(p,event.before);}
        std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(value>>(n*8u));
        memory_.write(p,b);
        if(observer_){event.written.assign(b.begin(),b.end());observer_(event);}
    }
};
std::int32_t signed_word(std::uint32_t value){return std::bit_cast<std::int32_t>(value);}
}
void native_tu15_remove_main_heap_allocation(SourcePhysicsContactMemory&memory,
    std::uint32_t allocator,std::uint32_t data,const SourcePhysicsContactRecordObserver&observe,
    NativeTu15MainHeapReleaseStats*stats){
    if(stats)++stats->ledger_calls;Memory m(memory,observe);
    auto saved_count=m.word(allocator+0x20u);
    if(signed_word(saved_count)>0){
        const auto base=m.word(allocator+0x18u);const auto end=m.word(allocator+0x1cu);
        auto cursor=base+end-8u;std::uint32_t index{};
        for(;;){
            if(stats)++stats->visited;
            const auto candidate=m.word(cursor)+base;
            if(candidate==data){
                if(stats)++stats->matched;--saved_count;
                if(signed_word(index)<signed_word(saved_count)){
                    auto offset=index*8u;
                    for(;;){
                        const auto current_base=m.word(allocator+0x18u);
                        const auto current_end=m.word(allocator+0x1cu);
                        const auto source_base=current_base-offset;
                        const auto first=m.word(source_base+current_end-16u);
                        m.word(0x232cd6u,current_base-offset+current_end-8u,first);
                        const auto second=m.word(source_base+current_end-12u);
                        m.word(0x232cdeu,current_base-offset+current_end-4u,second);
                        const auto count=m.word(allocator+0x20u);++index;offset+=8u;
                        if(stats)++stats->moved_rows;
                        if(signed_word(index)>=signed_word(count-1u))break;
                    }
                }
                const auto count=m.word(allocator+0x20u);
                m.word(0x232cf3u,allocator+0x20u,count-1u);break;
            }
            const auto count=m.word(allocator+0x20u);++index;cursor-=8u;
            if(signed_word(index)>=signed_word(count))break;
        }
    }
    if(stats)++stats->completed;
}
void native_tu15_release_main_heap_allocation(SourcePhysicsContactMemory&memory,
    std::uint32_t allocator,std::uint32_t data,std::uint32_t bytes,std::uint32_t tag,
    NativeTu15PhysicsFrameFp&fp,unsigned depth,const NativeTu15MainHeapReleaseServices&services,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15MainHeapReleaseStats*stats){
    (void)bytes;(void)tag;if(stats)++stats->wrapper_calls;Memory m(memory,observe);
    const auto table=m.word(allocator);const auto owner=m.word(table+0xcu);
    if(owner==0x232c80u){native_tu15_remove_main_heap_allocation(memory,allocator,data,observe,stats);return;}
    if(!services.release){if(stats){++stats->unbound_services;++stats->unbound_owners[owner];}
        throw std::runtime_error("Native TU1.5 main heap release requires current virtual+C owner="+std::to_string(owner));}
    services.release(allocator,owner,data,fp,depth);
    if(stats)++stats->completed;
}
} // namespace halo2
