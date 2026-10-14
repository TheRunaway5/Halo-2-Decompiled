#include "halo2/source_runtime_host_physical_memory.hpp"
#include <iostream>
using namespace halo2;
int main()try {
#if !defined(__linux__)
    return 77;
#endif
    SourceRuntimeMemory memory;
    SourceRuntimeHostPhysicalMemory physical(memory);
    const auto base=physical.allocate({0x2000,0x20000,0x21fff,0x1000,4});
    if(!base)throw std::runtime_error("Native physical allocation failed");
    const std::array marker{std::byte{0xab},std::byte{0xcd}};
    memory.write(base+0x1ffe,marker);
    // Actual map metadata requests at11F0A2/11F1C4 use byte addresses and
    // byte lengths. The native host must protect every intersecting page.
    physical.protect(base+0xd6c,0x980,4);
    physical.protect(base+0x814,0xe40,4);
    physical.protect(base+1,1,0x404);
    physical.protect(base+0x1fff,1,4);
    if(memory.view(base+0x1ffe,2)[0]!=marker[0] ||
        memory.view(base+0x1ffe,2)[1]!=marker[1])
        throw std::runtime_error("Byte-range protection changed retained source data");
    physical.protect(base+0x1fff,1,2);
    memory.write(base,marker); // The other page remains writable.
    SourcePhysicsContactMemory &engine=memory;
    bool write_rejected=false,view_rejected=false;
    try{engine.write(base+0x1000,marker);}catch(const std::runtime_error&){write_rejected=true;}
    try{(void)memory.mutable_view(base+0x1fff,1);}catch(const std::runtime_error&){view_rejected=true;}
    if(!write_rejected || !view_rejected || memory.view(base+0x1ffe,2)[0]!=marker[0])
        throw std::runtime_error("Physical read-only page was not enforced on retained engine storage");
    physical.protect(base+0x1ffe,1,4);
    engine.write(base+0x1ffe,marker);
    for(const auto request:std::array<std::array<std::uint32_t,3>,4>{{
        {base+0x1fff,2,4},{base-1,2,4},{base,1,0x104},{base,1,0x204}}}) {
        bool rejected=false;
        try{physical.protect(request[0],request[1],request[2]);}
        catch(const std::runtime_error&){rejected=true;}
        if(!rejected)throw std::runtime_error("Physical range escaped its allocation or unsupported protection was admitted");
    }
    SourceRuntimeMemory replacement;std::uint32_t reused;
    {
        SourceRuntimeHostPhysicalMemory retired(replacement);
        reused=retired.allocate({0x1000,0x30000,0x30fff,0x1000,4});
        retired.protect(reused,1,2);
    }
    replacement.mount(reused,std::vector<std::byte>(0x1000),SourceRuntimeMemory::Access::ReadWrite,"New owner after physical retirement");
    replacement.write(reused,marker);
    std::cout<<"PASS original map byte ranges, read-only engine writes/views, writable restoration, retirement and allocation/flag guards\n";
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
