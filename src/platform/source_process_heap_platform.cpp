#include "halo2/source_process_heap_platform.hpp"
#include "platform/source_process_heap_platform_bindings.hpp"
#include <algorithm>

namespace halo2 {
namespace {
std::uint32_t word(const SourceRuntimeMemory &memory, std::uint32_t address) {
    const auto bytes=memory.view(address,4);
    std::uint32_t result=0;
    for(unsigned i=0;i<4;++i) result|=std::to_integer<std::uint32_t>(bytes[i])<<(i*8);
    return result;
}
void put(SourceRuntimeMemory &memory, std::uint32_t address, std::uint32_t value) {
    std::array<std::byte,4> bytes;
    for(unsigned i=0;i<4;++i) bytes[i]=std::byte(value>>(i*8));
    memory.write(address,bytes);
}
}
bool source_try_native_process_heap_memory_call(SourceRuntimeMemory &memory,
    const SourcePhysicsContactRuntimeCall &call, SourcePhysicsContactRuntimeAbi &abi,
    SourcePhysicsContactImage image, SourceRuntimeHostVirtualMemory &vm,
    const SourceProcessHeapMemoryObserver &observe) {
    if(image!=SourcePhysicsContactImage::MapPack11 && image!=SourcePhysicsContactImage::TitleUpdate15)
        throw std::runtime_error("Unverified native process-heap platform call image");
    const auto &bindings=physics_platform_call_detail::heap_memory_bindings;
    const auto binding=std::find_if(bindings.begin(),bindings.end(),[&](const auto &b) {
        return b.instruction==call.instruction && b.target==call.target;
    });
    if(binding==bindings.end()) return false;
    if(call.tail || !vm.backs(memory))
        throw std::runtime_error("Native process-heap resource call requires its actual frame and bank owner");
    const auto esp=abi.gp[4], ebp=abi.gp[5];
    // Validate the complete actual pushed call frame before acquiring resources.
    memory.view(esp,24);
    const auto expected=image==SourcePhysicsContactImage::MapPack11?binding->return11:binding->return15;
    const auto bp=word(memory,esp+4), np=word(memory,esp+12);
    const bool reserve=call.instruction==0x2daa75u;
    if(word(memory,esp)!=expected || bp!=ebp-(reserve?0x1cu:0x20u) ||
        np!=ebp+(reserve?0x10u:0x14u))
        throw std::runtime_error("Native process-heap resource return/cell owner mismatch");
    const SourceVirtualAllocationRequest request{word(memory,bp),word(memory,esp+8),
        word(memory,np),word(memory,esp+16),word(memory,esp+20)};
    if(request.zero_bits || request.allocation_type!=(reserve?0x2000u:0x1000u) || request.protection!=4u)
        throw std::runtime_error("Native process-heap resource argument owner mismatch");
    memory.mutable_view(bp,4); memory.mutable_view(np,4);
    const auto result=vm.allocate(request);
    if(observe) observe({call.instruction,bp,np,request,result});
    put(memory,bp,result.base); put(memory,np,result.bytes);
    abi.gp[0]=result.status; abi.gp[4]=esp+24u;
    return true;
}
bool source_try_native_process_shutdown_call(SourceRuntimeMemory &memory,
    const SourcePhysicsContactRuntimeCall &call, SourcePhysicsContactRuntimeAbi &abi,
    SourcePhysicsContactImage image, SourceProcessShutdownRegistry &registry,
    const SourceProcessShutdownObserver &observe) {
    if(image!=SourcePhysicsContactImage::MapPack11 && image!=SourcePhysicsContactImage::TitleUpdate15)
        throw std::runtime_error("Unverified native process shutdown call image");
    if(call.instruction!=0x2d7fa5u || call.target!=0x8000002fu)return false;
    if(call.tail || !registry.backs(memory))
        throw std::runtime_error("Native shutdown registration requires actual frame/bank ownership");
    const bool later=image==SourcePhysicsContactImage::TitleUpdate15;
    const auto esp=abi.gp[4];memory.view(esp,12);
    if(word(memory,esp)!=(later?0x2dba5bu:0x2d7fabu) ||
        word(memory,esp+4)!=(later?0x46e2e8u:0x469228u) || word(memory,esp+8)!=1u)
        throw std::runtime_error("Native shutdown registration caller/argument mismatch");
    registry.registration(word(memory,esp+4),true,observe);
    abi.gp[4]=esp+12;return true;
}
bool source_try_native_process_startup_record_call(SourceRuntimeMemory &memory,
    const SourcePhysicsContactRuntimeCall &call, SourcePhysicsContactRuntimeAbi &abi,
    SourcePhysicsContactImage image, const SourceProcessStartupRecordObserver &observe) {
    if(image!=SourcePhysicsContactImage::MapPack11 && image!=SourcePhysicsContactImage::TitleUpdate15)
        throw std::runtime_error("Unverified native startup record call image");
    const bool dpc=call.instruction==0x2dc611u && call.target==0x8000006bu;
    const bool timer=call.instruction==0x2dc61eu && call.target==0x80000071u;
    if(!dpc && !timer)return false;
    if(call.tail)throw std::runtime_error("Native startup initializer requires its actual call frame");
    const bool later=image==SourcePhysicsContactImage::TitleUpdate15;
    const auto esp=abi.gp[4];memory.view(esp,dpc?16:12);
    if(dpc) {
        if(word(memory,esp)!=(later?0x2e00c7u:0x2dc617u) ||
            word(memory,esp+4)!=(later?0x56edc0u:0x566ea0u) ||
            word(memory,esp+8)!=(later?0x2e009du:0x2dc5edu) || word(memory,esp+12)!=0)
            throw std::runtime_error("Native DPC initializer caller/argument mismatch");
        source_initialize_process_dpc(memory,word(memory,esp+4),word(memory,esp+8),word(memory,esp+12),observe);
    } else {
        if(word(memory,esp)!=(later?0x2e00d4u:0x2dc624u) ||
            word(memory,esp+4)!=(later?0x56ede0u:0x566ec0u) || word(memory,esp+8)!=0)
            throw std::runtime_error("Native timer initializer caller/argument mismatch");
        source_initialize_process_timer(memory,word(memory,esp+4),word(memory,esp+8),observe);
    }
    abi.gp[4]=esp+(dpc?16:12);return true;
}
void source_run_process_heap_with_native_platform(SourceRuntimeMemory &memory,
    std::uint32_t entry, SourcePhysicsContactRuntimeAbi &abi, SourcePhysicsContactImage image,
    SourcePhysicsFactoryFloatingPoint fp, const SourcePhysicsContactRuntimeServices &services,
    SourceRuntimeHostVirtualMemory &vm, PhysicsPlatformLocks &locks,
    const PhysicsPlatformLockServices &platform, const SourcePhysicsContactRecordObserver &observe,
    const SourceProcessHeapMemoryObserver &memory_observe, const PhysicsPlatformLockObserver &lock_observe,
    SourceProcessShutdownRegistry *shutdown, const SourceProcessShutdownObserver &shutdown_observe,
    const SourceProcessStartupRecordObserver &startup_observe, const SourceProcessHeapDataImportOwner &data_owner) {
    if(!vm.backs(memory)) throw std::runtime_error("Native process-heap runtime bank owner mismatch");
    auto joined=services;
    joined.instruction=[&](std::uint32_t p) {
        if(p==0x2dc624u) {
            const bool later=image==SourcePhysicsContactImage::TitleUpdate15;
            const SourceProcessHeapDataImport request{p,later?0x2e00d4u:p,
                later?0x41b66cu:0x417bccu,322,8};
            if(!data_owner)throw SourceProcessHeapDataUnavailable(request);
            data_owner(memory,request);
            memory.view(word(memory,request.cell),request.bytes);
        }
        if(services.instruction)services.instruction(p);
    };
    joined.required_owner=[&](auto &bank,const auto &call,auto &state,const auto &original_observer) {
        if(shutdown && source_try_native_process_shutdown_call(memory,call,state,image,*shutdown,shutdown_observe))return;
        if(source_try_native_process_startup_record_call(memory,call,state,image,startup_observe))return;
        if(source_try_native_process_heap_memory_call(memory,call,state,image,vm,memory_observe)) return;
        if(source_try_native_physics_lock_call(bank,call,state,image,locks,platform,lock_observe)) return;
        if(!services.required_owner)
            throw std::runtime_error("Required original process-heap callee unavailable after native platform join");
        services.required_owner(bank,call,state,original_observer);
    };
    source_run_process_heap_runtime(memory,entry,abi,image,fp,joined,observe);
}
} // namespace halo2
