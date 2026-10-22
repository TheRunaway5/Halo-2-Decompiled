#include "halo2/native_tu15_startup_timer.hpp"
#include "halo2/source_tu15_settings.hpp"

namespace halo2 {
namespace {
std::uint32_t word(const SourceRuntimeMemory &memory,std::uint32_t address) {
    const auto bytes=memory.view(address,4);std::uint32_t value=0;
    for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(8*i);
    return value;
}
}
void source_run_native_tu15_startup_with_timer(const NativeTu15StartupPlatform &platform,
    NativeTu15StartupTimer &timer,SourceRuntimeMemory &memory,std::uint32_t entry,
    SourcePhysicsContactRuntimeAbi &abi,SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsContactRuntimeServices &services,SourceRuntimeHostVirtualMemory &vm,
    PhysicsPlatformLocks &locks,const PhysicsPlatformLockServices &lock_platform,SourceProcessShutdownRegistry &shutdown,
    const SourcePhysicsContactRecordObserver &observe,const SourcePhysicsContactRecordObserver &record_observe,
    const SourceProcessShutdownObserver &shutdown_observe,const SourcePhysicsContactRecordObserver &query_observe,
    const NativeTu15TimerObserver &timer_observe,const SourceProcessHeapMemoryObserver &memory_observe,
    const PhysicsPlatformLockObserver &lock_observe) {
    if(!timer.backs(memory))throw std::runtime_error("Native startup timer bank owner mismatch");
    if(entry==0x2d7f8eu && (abi.eflags&0x400u))
        throw std::invalid_argument("Complete process startup requires its incoming clear-direction ABI");
    auto joined=services;
    std::uint32_t instruction=0;
    joined.instruction=[&](auto p) {instruction=p;if(services.instruction)services.instruction(p);};
    joined.required_owner=[&](auto &bank,const auto &call,auto &state,const auto &observer) {
        if(&bank!=&memory)throw std::runtime_error("Native startup timer call bank owner mismatch");
        if(timer.try_call(call,state,observer,timer_observe))return;
        if(platform.try_call(memory,call,state,query_observe))return;
        if(call.instruction==0x2dc64cu && call.target==0x2e0fe2u && !call.tail) {
            if(word(memory,state.gp[4])!=0x2e0101u)
                throw std::runtime_error("Native startup settings wrapper return mismatch");
            source_run_tu15_settings_runtime(bank,state,fp,joined,observer);return;
        }
        if(!services.required_owner)throw std::runtime_error("Native startup still requires its actual heap/world owners");
        services.required_owner(bank,call,state,observer);
    };
    const auto records=[&](const SourceProcessStartupRecordStore &change) {
        if(instruction!=0x2dc611u && instruction!=0x2dc61eu)
            throw std::runtime_error("Native timer startup record lost its source caller");
        if(record_observe)record_observe({instruction,change.address,change.before,change.written});
    };
    source_run_process_heap_with_native_platform(memory,entry,abi,SourcePhysicsContactImage::TitleUpdate15,
        fp,joined,vm,locks,lock_platform,observe,memory_observe,lock_observe,&shutdown,shutdown_observe,records,platform.data_owner());
}
} // namespace halo2
