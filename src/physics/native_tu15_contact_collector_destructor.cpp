#include "halo2/native_tu15_contact_collector_destructor.hpp"
#include <stdexcept>

namespace halo2 {
std::uint32_t native_tu15_delete_contact_collector(SourcePhysicsContactMemory& memory,
    std::uint32_t collector,std::uint32_t disposing,NativeTu15PhysicsFrameFp& fp,
    const NativeTu15ActivationContactManagerServices& services,
    const SourcePhysicsContactRecordObserver& observe,NativeTu15ActivationContactManagerStats* stats){
    native_tu15_destroy_activation_collector(memory,collector,fp,services,observe,stats);
    if(disposing&1u){
        const auto read=[&](std::uint32_t address,unsigned size=4){
            std::array<std::byte,4> bytes{};memory.read(address,std::span(bytes).first(size));
            std::uint32_t value{};for(unsigned n=0;n<size;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);return value;
        };
        const auto bytes=read(collector+4u,2),allocator=read(0x48c938u),table=read(allocator),owner=read(table+0x14u);
        if(!services.allocation.release)throw std::runtime_error("Native TU1.5 collector destructor requires its current storage release owner");
        services.allocation.release({allocator,owner,collector,bytes,0x27u},fp);
    }
    return collector;
}
} // namespace halo2
