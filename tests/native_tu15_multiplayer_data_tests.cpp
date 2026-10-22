#include "halo2/native_tu15_multiplayer_data.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <set>
#include <span>
#include <stdexcept>

namespace {
using namespace halo2;
#include "fixtures/native_tu15_multiplayer_data.original.inc"
void need(bool value,const char* why){if(!value)throw std::runtime_error(why);}
template<class F>void rejects(F&& f){bool rejected=false;try{f();}catch(const std::invalid_argument&){rejected=true;}need(rejected,"Conflicting owner was accepted");}
}
int main()try {
    SourceRuntimeMemory memory;
    mount_native_tu15_multiplayer_data(memory);
    const auto cold=memory.checkpoint();
    need(cold.regions.size()==59,"Undeclared surrounding storage was mounted");
    std::size_t bytes=0;unsigned cases=0;
    for(const auto& row:original_multiplayer_data_regions) {
        need(std::ranges::equal(memory.view(row.address,row.bytes.size()),row.bytes),"Authenticated original cold data differs");
        need(memory.is_read_only(row.address,row.bytes.size())==row.read_only,"Original source access differs");
        need(memory.source_address(memory.native_address(row.address,row.bytes.size()),row.bytes.size())==row.address,"Original data identity was lost");
        bytes+=row.bytes.size();++cases;
        for(const auto offset:{-1,0,int(row.bytes.size())-1}) {
            SourceRuntimeMemory conflict;
            conflict.mount(row.address+offset,std::array<std::byte,2>{},SourceRuntimeMemory::Access::ReadOnly,"foreign partial owner");
            const auto before=conflict.checkpoint();rejects([&]{mount_native_tu15_multiplayer_data(conflict);});
            need(conflict.checkpoint()==before,"Rejected partial overlap published another region");++cases;
        }
        SourceRuntimeMemory reserved;
        reserved.reserve(row.address&~std::uint32_t(3),4,4,"foreign uncommitted reservation");
        const auto before=reserved.checkpoint();rejects([&]{mount_native_tu15_multiplayer_data(reserved);});
        need(reserved.checkpoint()==before,"Rejected uncommitted overlap published another region");++cases;
        if(!row.read_only) {
            const auto mark=std::vector<std::byte>(row.bytes.size(),std::byte{0xa5});
            memory.write(row.address,mark);const auto published=memory.checkpoint();
            rejects([&]{mount_native_tu15_multiplayer_data(memory);});
            need(memory.checkpoint()==published,"Repeated cold declaration reset a real publication");
            SourceRuntimeMemory restored;restored.restore(published);
            need(restored.checkpoint()==published,"Mutable source owner changed during authority restore");
            ++cases;
        }
    }
    need(bytes==2264,"Cold metadata extent differs");
    memory.restore(cold);
    const std::array mark{std::byte{0xff}};
    bool rejected=false;try{memory.write(0x45fce0u,mark);}catch(const std::exception&){rejected=true;}
    need(rejected&&memory.checkpoint()==cold,"Immutable method table accepted a write");++cases;
    SourceRuntimeMemory neighbors;
    std::set<std::uint32_t> adjacent;
    for(const auto& row:original_multiplayer_data_regions) {
        for(const auto address:{row.address-1,row.address+std::uint32_t(row.bytes.size())})
            if(!std::ranges::any_of(original_multiplayer_data_regions,[&](const auto& other){
                return address>=other.address&&address<other.address+other.bytes.size();
            }))adjacent.insert(address);
    }
    for(const auto address:adjacent)
        neighbors.mount(address,mark,SourceRuntimeMemory::Access::ReadOnly,"adjacent independent byte");
    mount_native_tu15_multiplayer_data(neighbors);
    for(const auto address:adjacent) {
        need(neighbors.view(address,1).front()==mark.front(),"Adjacent independent owner was changed");++cases;
    }
    std::cout<<cases<<" original-data/ownership cases passed; 59 exact regions, 2264 bytes\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
