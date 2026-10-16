#include "halo2/source_runtime_sound_timed_notifications.hpp"
#include <algorithm>
#include <array>
#include <stdexcept>
namespace halo2 {
void mount_native_tu15_sound_timed_notification_data(SourceRuntimeMemory& memory) {
    constexpr std::array rows{std::pair{0x41b668u,0x80000080u},std::pair{0x41b6f8u,0x80000095u},
        std::pair{0x41b788u,0x80000061u},std::pair{0x41b7f0u,0x80000089u}};
    std::array<bool,rows.size()> owned{};
    for(std::size_t row=0;row<rows.size();++row) {
        const auto [address,value]=rows[row];std::size_t covered{};
        memory.visit_committed([&](auto first,auto bytes){
            const auto begin=std::max<std::uint64_t>(first,address),end=std::min<std::uint64_t>(std::uint64_t(first)+bytes.size(),std::uint64_t(address)+4);
            if(begin<end)covered+=std::size_t(end-begin);
        });
        if(!covered)continue;
        if(covered!=4)throw std::invalid_argument("Native timed sound import is only partially owned");
        for(unsigned i=0;i<4;++i)if(!memory.is_read_only(address+i,1)||memory.view(address+i,1)[0]!=std::byte(value>>(8*i)))
            throw std::invalid_argument("Native timed sound import differs in immutable bytes/access");
        owned[row]=true;
    }
    std::array<std::uint32_t,rows.size()> added{};std::size_t count{};
    try{for(std::size_t row=0;row<rows.size();++row)if(!owned[row]) {
        const auto [address,value]=rows[row];std::array<std::byte,4> bytes{};
        for(unsigned i=0;i<4;++i)bytes[i]=std::byte(value>>(8*i));
        memory.mount(address,bytes,SourceRuntimeMemory::Access::ReadOnly,"Authenticated timed sound kernel import operand");added[count++]=address;
    }}catch(...){while(count)memory.release(added[--count]);throw;}
}
}
