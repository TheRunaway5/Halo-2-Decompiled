#include "halo2/source_tu15_timer.hpp"
#include <fstream>
namespace halo2 { namespace {
std::uint32_t word(const SourceRuntimeMemory &m,std::uint32_t p) {
    auto b=m.view(p,4);std::uint32_t v=0;
    for(unsigned i=0;i<4;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(8*i);return v;
}
}
void source_mount_tu15_timer_constants(SourceRuntimeMemory &m,const std::filesystem::path &p) {
    std::ifstream in(p,std::ios::binary|std::ios::ate);
    if(!in||in.tellg()!=153136)throw std::runtime_error("Timer constants require the complete owned kernel .text");
    std::vector<std::byte> b(153136);in.seekg(0);
    if(!in.read(reinterpret_cast<char *>(b.data()),b.size()))throw std::runtime_error("Truncated owned kernel .text");
    std::uint64_t h=14695981039346656037ull;
    for(auto v:b)h=(h^std::to_integer<unsigned>(v))*1099511628211ull;
    if(h!=0xa1f4e7c5e58a4cb3ull)throw std::runtime_error("Timer constants require the authenticated owned modified Complex4627 image");
    m.mount(0x80010cc8,std::span<const std::byte>(b).subspan(0xa08,9),SourceRuntimeMemory::Access::ReadOnly,
        p.string()+" actual timer index constants");
}
void source_run_tu15_process_startup_with_timer(SourceRuntimeMemory &m,std::uint32_t entry,
    SourcePhysicsContactRuntimeAbi &abi,SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsContactRuntimeServices &services,SourceRuntimeHostVirtualMemory &vm,
    PhysicsPlatformLocks &locks,const PhysicsPlatformLockServices &platform,SourceProcessShutdownRegistry &shutdown,
    const SourceTu15UserSettings &settings,const SourceTu15StatusTables &tables,
    const SourcePhysicsContactRecordObserver &observe,const SourcePhysicsContactRecordObserver &record_observe,
    const SourceProcessShutdownObserver &shutdown_observe,const SourcePhysicsContactRecordObserver &query_observe,
    const SourceProcessHeapDataImportOwner &data_owner,const std::function<void(std::uint32_t)> &kernel_instruction,
    const SourcePhysicsContactRecordObserver &kernel_observe,const SourceTu15TimerDataOwner &timer_data_owner) {
    auto joined=services;
    joined.required_owner=[&](auto &bank,const auto &call,auto &state,const auto &observer) {
        if(&bank!=&m)throw std::runtime_error("TU1.5 timer startup bank owner mismatch");
        if(call.instruction==0x2dc5e6&&call.target==0x80000095) {
            const auto sp=state.gp[4];m.view(sp,20);
            if(call.tail||word(m,sp)!=0x2e009c||word(m,sp+4)!=0x56ede0||word(m,sp+8)!=0xb5659000||
                word(m,sp+12)!=0xffffffcd||word(m,sp+16)!=0x56edc0)
                throw std::runtime_error("TU1.5 timer requires its actual complete startup frame");
            SourcePhysicsContactRuntimeServices kernel=joined;kernel.instruction=kernel_instruction;
            source_run_tu15_timer_runtime(bank,0x800195d6,state,fp,kernel,kernel_observe,timer_data_owner);return;
        }
        if(!services.required_owner)throw std::runtime_error("Required original timer interrupt/expiry owner unavailable");
        services.required_owner(bank,call,state,observer);
    };
    source_run_tu15_process_startup_with_status(m,entry,abi,fp,joined,vm,locks,platform,shutdown,settings,tables,
        observe,record_observe,shutdown_observe,query_observe,data_owner,kernel_instruction,kernel_observe);
}
}
