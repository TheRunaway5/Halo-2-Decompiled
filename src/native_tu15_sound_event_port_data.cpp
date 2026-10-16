#include "halo2/source_runtime_sound_event_port.hpp"
#include <algorithm>
#include <array>
#include <stdexcept>
namespace halo2 {namespace {
void literal(SourceRuntimeMemory& memory,std::uint32_t address,std::uint32_t value) {
    std::array<std::byte,4> bytes{};for(unsigned i=0;i<4;++i)bytes[i]=std::byte(value>>(8*i));
    std::size_t covered=0;memory.visit_committed([&](auto first,auto present){
        const auto begin=std::max<std::uint64_t>(first,address),end=std::min<std::uint64_t>(std::uint64_t(first)+present.size(),std::uint64_t(address)+4);
        if(begin<end)covered+=std::size_t(end-begin);
    });
    if(covered) {
        if(covered!=4)throw std::invalid_argument("Sound event import is only partially owned");
        for(unsigned i=0;i<4;++i)if(!memory.is_read_only(address+i,1)||memory.view(address+i,1)[0]!=bytes[i])
            throw std::invalid_argument("Sound event import differs in immutable bytes/access");
        return;
    }
    memory.mount(address,bytes,SourceRuntimeMemory::Access::ReadOnly,"authenticated sound native event import");
}
}
void mount_native_tu15_sound_event_port_data(SourceRuntimeMemory& memory) {
    // The original source observes only ServiceRoutine/context/cleanup identity.
    // Private PCI/IRQL/dispatch-code fields have no mounted source substitute.
    memory.mount(0x3912d8,std::array<std::byte,8>{},SourceRuntimeMemory::Access::ReadWrite,"source sound interrupt cleanup cells, loader zero");
    for(const auto& [address,value]:std::array{std::pair{0x41b708u,0x80000071u},std::pair{0x41b70cu,0x8000006bu},
        std::pair{0x41b7a8u,0x8000002cu},std::pair{0x41b7b4u,0x8000006du},std::pair{0x41b7b0u,0x80000062u}})literal(memory,address,value);
}
}
