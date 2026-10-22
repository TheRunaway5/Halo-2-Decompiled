#include "halo2/source_tu15_startup_records.hpp"
#include <algorithm>
#include <stdexcept>

namespace halo2 {
namespace {
std::uint32_t word(const SourceRuntimeMemory &m,std::uint32_t p) {
    const auto b=m.view(p,4);std::uint32_t value=0;
    for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(b[i])<<(8*i);
    return value;
}
void put(SourceRuntimeMemory &m,std::uint32_t site,std::uint32_t p,
    unsigned width,std::uint32_t value,const SourcePhysicsContactRecordObserver &observe) {
    SourcePhysicsContactRecordStore change{site,p,std::vector<std::byte>(width),std::vector<std::byte>(width)};
    m.read(p,change.before);
    for(unsigned i=0;i<width;++i)change.written[i]=std::byte(value>>(8*i));
    m.write(p,change.written);if(observe)observe(change);
}
}
SourceProcessHeapDataImportOwner source_tu15_hardware_import_owner(
    const SourceRuntimeKernelHardwareInfo &hardware) {
    return [hardware](SourceRuntimeMemory &m,const SourceProcessHeapDataImport &request) {
        if(request.instruction!=0x2dc624u || request.actual_instruction!=0x2e00d4u ||
            request.cell!=0x41b66cu || request.ordinal!=322 || request.bytes!=8 ||
            hardware.address<0x80010000u || hardware.address>0xfffffff8u || hardware.owner.empty() ||
            word(m,request.cell)!=hardware.address ||
            !std::ranges::equal(m.view(hardware.address,8),hardware.bytes))
            throw std::runtime_error("TU1.5 hardware import does not match its actual retained owner");
    };
}
bool source_try_tu15_startup_record_call(SourceRuntimeMemory &m,
    const SourcePhysicsContactRuntimeCall &call,SourcePhysicsContactRuntimeAbi &abi,
    const SourcePhysicsContactRecordObserver &observe) {
    const bool dpc=call.instruction==0x2dc611u && call.target==0x8000006bu;
    const bool timer=call.instruction==0x2dc61eu && call.target==0x80000071u;
    if(!dpc && !timer)return false;
    if(call.tail)throw std::runtime_error("TU1.5 record initializer requires its pushed return frame");
    const auto sp=abi.gp[4];const unsigned frame=dpc?16:12;
    m.view(sp,frame);
    const auto p=word(m,sp+4),argument=word(m,sp+8);
    const auto context=dpc?word(m,sp+12):0u;
    if(word(m,sp)!=(dpc?0x2e00c7u:0x2e00d4u) || p!=(dpc?0x56edc0u:0x56ede0u) ||
        argument!=(dpc?0x2e009du:0u) || context!=0)
        throw std::runtime_error("TU1.5 record initializer caller/argument mismatch");
    m.mutable_view(p,dpc?28:40);
    const auto store=[&](unsigned offset,unsigned width,std::uint32_t value) {
        put(m,call.instruction,p+offset,width,value,observe);
    };
    if(dpc) {
        store(0,2,0x13);store(2,1,0);
        store(12,4,argument);store(16,4,context);
    } else {
        store(0,1,8);store(3,1,0);store(2,1,10);store(4,4,0);
        // The actual chained assignment writes Blink before Flink.
        store(12,4,p+8);store(8,4,p+8);
        store(28,4,0);store(24,4,0);
        // Independent i386 primary compilation retains low/high DWORD order.
        store(16,4,0);store(20,4,0);store(36,4,0);
    }
    abi.gp[4]=sp+frame;return true;
}
void source_run_tu15_process_startup_records(SourceRuntimeMemory &m,
    std::uint32_t entry,SourcePhysicsContactRuntimeAbi &abi,SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsContactRuntimeServices &services,SourceRuntimeHostVirtualMemory &vm,
    PhysicsPlatformLocks &locks,const PhysicsPlatformLockServices &platform,
    SourceProcessShutdownRegistry &shutdown,const SourcePhysicsContactRecordObserver &observe,
    const SourcePhysicsContactRecordObserver &record_observe,
    const SourceProcessShutdownObserver &shutdown_observe,
    const SourceProcessHeapDataImportOwner &data_owner) {
    auto joined=services;
    std::uint32_t instruction=0;
    joined.instruction=[&](auto p) {
        instruction=p;
        if(services.instruction)services.instruction(p);
    };
    const auto records=[&](const SourceProcessStartupRecordStore &change) {
        if(instruction!=0x2dc611u && instruction!=0x2dc61eu)
            throw std::runtime_error("TU1.5 record write lost its actual source caller");
        if(record_observe)record_observe({instruction,change.address,change.before,change.written});
    };
    source_run_process_heap_with_native_platform(m,entry,abi,SourcePhysicsContactImage::TitleUpdate15,
        fp,joined,vm,locks,platform,observe,{}, {},&shutdown,shutdown_observe,records,data_owner);
}
} // namespace halo2
