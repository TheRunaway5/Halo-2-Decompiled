#include "halo2/native_tu15_stack_arena.hpp"
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
        std::array<std::byte,4> bytes{};memory_.read(address,bytes);
        std::uint32_t value{};for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(bytes[n])<<(8u*n);
        return value;
    }
    void word(std::uint32_t site,std::uint32_t address,std::uint32_t value){
        std::array<std::byte,4> bytes{};for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(8u*n));
        SourcePhysicsContactRecordStore event{site,address,{},{}};
        if(observe_){event.before.resize(4);memory_.read(address,event.before);event.written.assign(bytes.begin(),bytes.end());}
        memory_.write(address,bytes);if(observe_)observe_(event);
    }
};
}
std::uint32_t native_tu15_allocate_stack_arena_with_phases(SourcePhysicsContactMemory& memory,
    std::uint32_t arena,const std::function<std::uint32_t()>&acquire_requested_bytes,NativeTu15PhysicsFrameFp& fp,
    const NativeTu15ArrayGrowthServices& services,const NativeTu15StackArenaPhases&phases,
    const SourcePhysicsContactRecordObserver& observe){
    Memory m(memory,observe);
    NativeTu15StackArenaPhaseData phase;
    const auto notify=[&](NativeTu15StackArenaPhase stage){if(phases.observe)phases.observe(stage,phase);};
    const auto requested_bytes=acquire_requested_bytes();
    const auto bytes=std::bit_cast<std::int32_t>(requested_bytes)>0x1000?requested_bytes:0x1000u;
    phase.bytes=bytes;phase.requested=requested_bytes;notify(NativeTu15StackArenaPhase::allocation_inputs);
    const auto allocator=m.word(0x48c938u),table=m.word(allocator);phase.allocator=allocator;phase.table=table;
    notify(NativeTu15StackArenaPhase::allocation_owner_lookup);const auto owner=m.word(table+0x10u);
    if(!services.allocate)throw std::runtime_error("Native TU1.5 stack arena requires its actual backing allocation owner");
    const auto block=services.allocate({allocator,owner,bytes+0x10u,0x12u},fp);
    phase.block=block;notify(NativeTu15StackArenaPhase::allocation_complete);
    constexpr std::array<std::uint32_t,4> publications{0x2e10d1u,0x2e10d6u,0x2e10dcu,0x2e10e2u};
    for(unsigned field=0;field<4;++field){phase.field=field;phase.value=m.word(arena+8u+field*4u);
        notify(NativeTu15StackArenaPhase::header_word);m.word(publications[field],block+field*4u,phase.value);}
    const auto result=block+0x10u;
    const auto current_requested=acquire_requested_bytes();phase.requested=current_requested;
    notify(NativeTu15StackArenaPhase::remaining_inputs);
    m.word(0x2e10eeu,arena+0xcu,bytes-current_requested);
    m.word(0x2e10f1u,arena+0x10u,block);
    m.word(0x2e10f4u,arena+0x14u,result);
    notify(NativeTu15StackArenaPhase::boundary_published);
    m.word(0x2e10fcu,arena+8u,result+current_requested);
    return result;
}
std::uint32_t native_tu15_allocate_stack_arena(SourcePhysicsContactMemory&memory,std::uint32_t arena,
    std::uint32_t requested_bytes,NativeTu15PhysicsFrameFp&fp,const NativeTu15ArrayGrowthServices&services,
    const SourcePhysicsContactRecordObserver&observe){
    return native_tu15_allocate_stack_arena_with_phases(memory,arena,[requested_bytes]{return requested_bytes;},fp,services,{},observe);
}
void native_tu15_release_stack_arena_with_phases(SourcePhysicsContactMemory& memory,
    std::uint32_t arena,const std::function<std::uint32_t()>&acquire_requested_bytes,
    NativeTu15PhysicsFrameFp& fp,const NativeTu15ArrayGrowthServices& services,
    const NativeTu15StackArenaPhases&phases,const SourcePhysicsContactRecordObserver& observe){
    Memory m(memory,observe);NativeTu15StackArenaPhaseData phase;phase.releasing=true;
    const auto notify=[&](NativeTu15StackArenaPhase stage){if(phases.observe)phases.observe(stage,phase);};
    const auto boundary=m.word(arena+0x14u),remaining=m.word(arena+0xcu);phase.boundary=boundary;phase.available=remaining;
    notify(NativeTu15StackArenaPhase::release_inputs);const auto requested_bytes=acquire_requested_bytes();
    const auto backing=boundary-0x10u,bytes=remaining+requested_bytes+0x10u;
    phase.block=backing;phase.bytes=bytes;phase.requested=requested_bytes;notify(NativeTu15StackArenaPhase::release_restore_inputs);
    constexpr std::array<std::uint32_t,4> publications{0x2e112au,0x2e112fu,0x2e1135u,0x2e113du};
    for(unsigned field=0;field<4;++field){phase.field=field;phase.value=m.word(backing+field*4u);
        notify(NativeTu15StackArenaPhase::header_word);m.word(publications[field],arena+8u+field*4u,phase.value);}
    const auto allocator=m.word(0x48c938u),table=m.word(allocator);phase.allocator=allocator;phase.table=table;
    notify(NativeTu15StackArenaPhase::release_owner_lookup);const auto owner=m.word(table+0x14u);
    if(!services.release)throw std::runtime_error("Native TU1.5 stack arena requires its actual backing release owner");
    services.release({allocator,owner,backing,bytes,0x12u},fp);
}
void native_tu15_release_stack_arena(SourcePhysicsContactMemory&memory,std::uint32_t arena,std::uint32_t,
    std::uint32_t requested_bytes,NativeTu15PhysicsFrameFp&fp,const NativeTu15ArrayGrowthServices&services,
    const SourcePhysicsContactRecordObserver&observe){
    native_tu15_release_stack_arena_with_phases(memory,arena,[requested_bytes]{return requested_bytes;},fp,services,{},observe);
}
} // namespace halo2
