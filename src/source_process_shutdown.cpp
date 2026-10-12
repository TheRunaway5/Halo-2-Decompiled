#include "halo2/source_process_shutdown.hpp"
#include <bit>
#include <set>
#include <stdexcept>

namespace halo2 {
namespace {
std::uint32_t word(const SourceRuntimeMemory &m,std::uint32_t p) {
    const auto b=m.view(p,4);std::uint32_t value=0;
    for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(b[i])<<(i*8);
    return value;
}
void put(SourceRuntimeMemory &m,std::uint32_t p,std::uint32_t value,const SourceProcessShutdownObserver &observe) {
    SourceProcessShutdownStore change{p,std::vector<std::byte>(4),std::vector<std::byte>(4)};
    m.read(p,change.before);
    for(unsigned i=0;i<4;++i)change.written[i]=std::byte(value>>(i*8));
    m.write(p,change.written);if(observe)observe(change);
}
void visit(std::set<std::uint32_t> &seen,std::uint32_t p) {
    if(!seen.insert(p).second)throw std::runtime_error("Native shutdown registry contains cyclic record links");
}
}
SourceProcessShutdownRegistry::SourceProcessShutdownRegistry(SourceRuntimeMemory &m,std::uint32_t p)
    :memory_(m),head_(p) {
    // Actual primary static self-linked list initialization, at a native token.
    std::array<std::byte,8> initial;
    for(unsigned i=0;i<8;++i)initial[i]=std::byte(p>>((i%4)*8));
    memory_.mount(head_,initial,SourceRuntimeMemory::Access::ReadWrite,"native process shutdown registry head");
}
SourceProcessShutdownRegistry::~SourceProcessShutdownRegistry() { memory_.release(head_); }
void SourceProcessShutdownRegistry::registration(std::uint32_t record,bool add,
    const SourceProcessShutdownObserver &observe) {
    std::lock_guard guard(mutex_);
    auto entry=word(memory_,head_);
    std::set<std::uint32_t> seen;
    if(add) {
        const auto priority=std::bit_cast<std::int32_t>(word(memory_,record+4));
        while(entry!=head_) {
            visit(seen,entry);
            if(priority>std::bit_cast<std::int32_t>(word(memory_,entry-4)))break;
            entry=word(memory_,entry);
        }
        // Exact InsertTailList order. Strict comparison preserves equal order.
        const auto tail=word(memory_,entry+4),node=record+8;
        put(memory_,node,entry,observe);put(memory_,node+4,tail,observe);
        put(memory_,tail,node,observe);put(memory_,entry+4,node,observe);
    } else {
        while(entry!=head_) {
            visit(seen,entry);
            if(record==entry-8) {
                const auto next=word(memory_,entry),previous=word(memory_,entry+4);
                if(previous)put(memory_,previous,next,observe);
                if(next)put(memory_,next+4,previous,observe);
                break;
            }
            entry=word(memory_,entry);
        }
    }
}
std::vector<SourceProcessShutdownRecord> SourceProcessShutdownRegistry::records() const {
    std::lock_guard guard(mutex_);
    std::vector<SourceProcessShutdownRecord> result;std::set<std::uint32_t> seen;
    auto entry=word(memory_,head_);
    while(entry!=head_) {
        visit(seen,entry);const auto record=entry-8;
        result.push_back({record,word(memory_,record),std::bit_cast<std::int32_t>(word(memory_,record+4))});
        entry=word(memory_,entry);
    }
    return result;
}
} // namespace halo2
