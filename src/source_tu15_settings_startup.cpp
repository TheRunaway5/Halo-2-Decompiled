#include "halo2/source_tu15_settings.hpp"
namespace halo2 {
namespace {
std::uint32_t word(const SourceRuntimeMemory&m,std::uint32_t p){auto b=m.view(p,4);std::uint32_t value{};for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(b[i])<<(8*i);return value;}
}
void source_run_tu15_process_startup_with_settings(SourceRuntimeMemory &m,
    std::uint32_t entry,SourcePhysicsContactRuntimeAbi &abi,SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsContactRuntimeServices &services,SourceRuntimeHostVirtualMemory &vm,
    PhysicsPlatformLocks &locks,const PhysicsPlatformLockServices &platform,
    SourceProcessShutdownRegistry &shutdown,const SourceTu15UserSettings &settings,
    const SourcePhysicsContactRecordObserver &observe,const SourcePhysicsContactRecordObserver &record_observe,
    const SourceProcessShutdownObserver &shutdown_observe,const SourcePhysicsContactRecordObserver &query_observe,
    const SourceProcessHeapDataImportOwner &data_owner) {
    auto joined=services;
    joined.required_owner=[&](auto &bank,const auto &call,auto &state,const auto &observer) {
        if(&bank!=&m)throw std::runtime_error("TU1.5 settings startup bank owner mismatch");
        if(source_try_tu15_settings_query(m,call,state,settings,query_observe))return;
        if(call.instruction==0x2dc64c && call.target==0x2e0fe2 && !call.tail) {
            if(word(m,state.gp[4])!=0x2e0101)throw std::runtime_error("TU1.5 settings wrapper return mismatch");
            source_run_tu15_settings_runtime(bank,state,fp,joined,observer);return;
        }
        if(!services.required_owner)throw std::runtime_error("Required TU1.5 settings status/timer owner unavailable");
        services.required_owner(bank,call,state,observer);
    };
    source_run_tu15_process_startup_records(m,entry,abi,fp,joined,vm,locks,platform,shutdown,
        observe,record_observe,shutdown_observe,data_owner);
}
} // namespace halo2
