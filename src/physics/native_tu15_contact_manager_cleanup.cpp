#include "halo2/native_tu15_contact_manager_cleanup.hpp"
#include "native_tu15_contact_unwind_internal.hpp"
#include <stdexcept>
namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory& memory_;const SourcePhysicsContactRecordObserver& observer_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):memory_(m),observer_(o){}
    std::uint32_t word(std::uint32_t p,unsigned width=4)const{std::array<std::byte,4>b{};memory_.read(p,std::span(b).first(width));std::uint32_t v{};for(unsigned n=0;n<width;++n)v|=std::to_integer<unsigned>(b[n])<<(8u*n);return v;}
    void word(std::uint32_t pc,std::uint32_t p,std::uint32_t v){std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(v>>(8u*n));SourcePhysicsContactRecordStore e{pc,p,{},{}};if(observer_){e.before.resize(4);memory_.read(p,e.before);e.written.assign(b.begin(),b.end());}memory_.write(p,b);if(observer_)observer_(e);}
};
}
void native_tu15_destroy_contact_manager(SourcePhysicsContactMemory& memory,
    std::uint32_t manager,NativeTu15ContactManagerCleanupScratch scratch,NativeTu15PhysicsFrameFp& fp,
    const NativeTu15ContactManagerCleanupServices& services,const SourcePhysicsContactRecordObserver& observer,
    NativeTu15ActivationContactManagerStats* stats){
    Memory m(memory,observer);m.word(0x3231cfu,manager,0x41f124u);const auto current=m.word(manager+0xa4u);
    native_contact_detail::UnwindGuard base([&]{m.word(0x2f7a10u,manager,0x45edf0u);});
    native_contact_detail::UnwindGuard collector([&]{native_tu15_destroy_activation_collector(memory,manager+0x10u,fp,services.collector,observer,stats);});
    base.arm();collector.arm();
    if(current){const auto world=m.word(manager+8u);if(!services.update_world)throw std::runtime_error("Native TU1.5 contact manager requires its current world-update owner316AA0");services.update_world(world,scratch.result_byte,manager+0x10u,fp);}
    collector.disarm();native_tu15_destroy_activation_collector(memory,manager+0x10u,fp,services.collector,observer,stats);
    m.word(0x323209u,manager,0x45edf0u);base.disarm();
}
std::uint32_t native_tu15_delete_contact_manager(SourcePhysicsContactMemory& memory,
    std::uint32_t manager,std::uint32_t disposing,NativeTu15ContactManagerCleanupScratch scratch,
    NativeTu15PhysicsFrameFp& fp,const NativeTu15ContactManagerCleanupServices& services,
    const SourcePhysicsContactRecordObserver& observer,NativeTu15ActivationContactManagerStats* stats){
    native_tu15_destroy_contact_manager(memory,manager,scratch,fp,services,observer,stats);
    if(disposing&1u){const SourcePhysicsContactRecordObserver none;Memory m(memory,none);const auto size=m.word(manager+4u,2),allocator=m.word(0x48c938u),table=m.word(allocator),owner=m.word(table+0x14u);if(!services.collector.allocation.release)throw std::runtime_error("Native TU1.5 contact manager requires its actual scalar storage release");services.collector.allocation.release({allocator,owner,manager,size,0x1du},fp);}
    return manager;
}
} // namespace halo2
