#include "halo2/source_tu15_interrupt.hpp"
#include <fstream>
namespace halo2 {
void source_mount_tu15_interrupt_masks(SourceRuntimeMemory &memory,const std::filesystem::path &path) {
    std::ifstream in(path,std::ios::binary|std::ios::ate);
    if(!in||in.tellg()!=153136)throw std::runtime_error("IRQL masks require the complete owned kernel text");
    std::vector<std::byte> bytes(153136);in.seekg(0);
    if(!in.read(reinterpret_cast<char *>(bytes.data()),bytes.size()))throw std::runtime_error("Truncated kernel text");
    std::uint64_t h=14695981039346656037ull;
    for(auto b:bytes)h=(h^std::to_integer<unsigned>(b))*1099511628211ull;
    if(h!=0xa1f4e7c5e58a4cb3ull)throw std::runtime_error("IRQL masks require authenticated modified Complex4627 text");
    memory.mount(0x800142a4,std::span<const std::byte>(bytes).subspan(0x3fe4,128),
        SourceRuntimeMemory::Access::ReadOnly,path.string()+" original IRQL masks");
}
SourcePhysicsContactRuntimeServices source_tu15_interrupt_services(
    const SourcePhysicsContactRuntimeServices &base,SourcePhysicsFactoryFloatingPoint fp,
    const SourceTu15TimerDataOwner &data,const SourceTu15InterruptPortOwner &ports,
    const std::function<void(std::uint32_t)> &kernel_instruction) {
    auto joined=base;
    joined.required_owner=[base,fp,data,ports,kernel_instruction](auto &memory,const auto &call,auto &abi,const auto &observe) {
        if(call.target==0x80018ba0 && call.instruction==0x800195c7 && !call.tail) {
            auto kernel=base;if(kernel_instruction)kernel.instruction=kernel_instruction;
            source_run_tu15_interrupt_runtime(memory,call.target,abi,fp,kernel,observe,data,ports);return;
        }
        if(!base.required_owner)throw std::runtime_error("Original interrupt scheduler/dispatch owner unavailable");
        base.required_owner(memory,call,abi,observe);
    };
    return joined;
}
void source_run_tu15_process_startup_with_interrupts(SourceRuntimeMemory &memory,std::uint32_t entry,
    SourcePhysicsContactRuntimeAbi &abi,SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsContactRuntimeServices &services,SourceRuntimeHostVirtualMemory &vm,
    PhysicsPlatformLocks &locks,const PhysicsPlatformLockServices &platform,SourceProcessShutdownRegistry &shutdown,
    const SourceTu15UserSettings &settings,const SourceTu15StatusTables &tables,
    const SourcePhysicsContactRecordObserver &observe,const SourcePhysicsContactRecordObserver &record_observe,
    const SourceProcessShutdownObserver &shutdown_observe,const SourcePhysicsContactRecordObserver &query_observe,
    const SourceProcessHeapDataImportOwner &hardware,const std::function<void(std::uint32_t)> &kernel_instruction,
    const SourcePhysicsContactRecordObserver &kernel_observe,const SourceTu15TimerDataOwner &data,
    const SourceTu15InterruptPortOwner &ports) {
    const auto joined=source_tu15_interrupt_services(services,fp,data,ports,kernel_instruction);
    source_run_tu15_process_startup_with_timer(memory,entry,abi,fp,joined,vm,locks,platform,shutdown,settings,tables,
        observe,record_observe,shutdown_observe,query_observe,hardware,kernel_instruction,kernel_observe,data);
}
}
