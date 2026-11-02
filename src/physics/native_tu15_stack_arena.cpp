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
std::uint32_t native_tu15_allocate_stack_arena(SourcePhysicsContactMemory& memory,
    std::uint32_t arena,std::uint32_t requested_bytes,NativeTu15PhysicsFrameFp& fp,
    const NativeTu15ArrayGrowthServices& services,const SourcePhysicsContactRecordObserver& observe){
    Memory m(memory,observe);
    const auto bytes=std::bit_cast<std::int32_t>(requested_bytes)>0x1000?requested_bytes:0x1000u;
    const auto allocator=m.word(0x48c938u),table=m.word(allocator),owner=m.word(table+0x10u);
    if(!services.allocate)throw std::runtime_error("Native TU1.5 stack arena requires its actual backing allocation owner");
    const auto block=services.allocate({allocator,owner,bytes+0x10u,0x12u},fp);
    m.word(0x2e10d1u,block,m.word(arena+8u));
    m.word(0x2e10d6u,block+4u,m.word(arena+0xcu));
    m.word(0x2e10dcu,block+8u,m.word(arena+0x10u));
    m.word(0x2e10e2u,block+0xcu,m.word(arena+0x14u));
    const auto result=block+0x10u;
    m.word(0x2e10eeu,arena+0xcu,bytes-requested_bytes);
    m.word(0x2e10f1u,arena+0x10u,block);
    m.word(0x2e10f4u,arena+0x14u,result);
    m.word(0x2e10fcu,arena+8u,result+requested_bytes);
    return result;
}
void native_tu15_release_stack_arena(SourcePhysicsContactMemory& memory,
    std::uint32_t arena,std::uint32_t block,std::uint32_t requested_bytes,
    NativeTu15PhysicsFrameFp& fp,const NativeTu15ArrayGrowthServices& services,
    const SourcePhysicsContactRecordObserver& observe){
    Memory m(memory,observe);(void)block;
    const auto boundary=m.word(arena+0x14u),remaining=m.word(arena+0xcu);
    const auto backing=boundary-0x10u,bytes=remaining+requested_bytes+0x10u;
    m.word(0x2e112au,arena+8u,m.word(backing));
    m.word(0x2e112fu,arena+0xcu,m.word(backing+4u));
    m.word(0x2e1135u,arena+0x10u,m.word(backing+8u));
    m.word(0x2e113du,arena+0x14u,m.word(backing+0xcu));
    const auto allocator=m.word(0x48c938u),table=m.word(allocator),owner=m.word(table+0x14u);
    if(!services.release)throw std::runtime_error("Native TU1.5 stack arena requires its actual backing release owner");
    services.release({allocator,owner,backing,bytes,0x12u},fp);
}
} // namespace halo2
