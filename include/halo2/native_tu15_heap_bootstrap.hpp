#pragma once
#include "halo2/native_tu15_heap_data.hpp"
#include "halo2/source_process_heap_platform.hpp"
#include "halo2/native_synchronization.hpp"
#include <thread>

namespace halo2 {
// The first native process-startup owner. Constructs the actual process heap
// and publishes its binding without an executable, captured bank or live-state
// template. Later game-arena/storage constructors must add their own typed
// owners; this object alone does not admit a map or create a gameplay world.
class NativeTu15HeapBootstrap final {
public:
    NativeTu15HeapBootstrap();
    NativeTu15HeapBootstrap(const NativeTu15HeapBootstrap&)=delete;
    NativeTu15HeapBootstrap& operator=(const NativeTu15HeapBootstrap&)=delete;
    NativeTu15HeapBootstrap(NativeTu15HeapBootstrap&&)=delete;
    NativeTu15HeapBootstrap& operator=(NativeTu15HeapBootstrap&&)=delete;
    SourceRuntimeMemory& memory() noexcept{return memory_;}
    const SourceRuntimeMemory& memory()const noexcept{return memory_;}
    NativeRuntimeAddress heap()const noexcept{return heap_;}
    // Continue the authenticated CRT GetProcessHeap/RtlAllocateHeap callers
    // against this constructed heap and its retained synchronization owner.
    bool try_crt_allocation_call(SourcePhysicsContactMemory&,const SourcePhysicsContactRuntimeCall&,
        SourcePhysicsContactRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
        const SourcePhysicsContactRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
    bool try_crt_growth_call(SourcePhysicsContactMemory&,const SourcePhysicsContactRuntimeCall&,
        SourcePhysicsContactRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
        const SourcePhysicsContactRuntimeServices&,const SourcePhysicsContactRecordObserver& = {},
        const SourceProcessHeapMemoryObserver& = {});
    bool try_renderer_allocation_call(SourcePhysicsContactMemory&,const SourcePhysicsContactRuntimeCall&,
        SourcePhysicsContactRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
        const SourcePhysicsContactRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
    std::uint32_t current_thread_token();
private:
    PhysicsPlatformLockServices lock_services();
    // Native resource owners retain this bank's identity and retire first.
    SourceRuntimeMemory memory_;
    SourceRuntimeHostVirtualMemory virtual_memory_;
    PhysicsPlatformLocks locks_;
    NativeSynchronization synchronization_{{0xf7000001u,0xf8000000ull}};
    std::mutex service_mutex_;
    std::map<std::thread::id,std::uint32_t> thread_tokens_;
    std::map<std::uint32_t,std::uint32_t> lock_events_;
    std::uint64_t next_thread_token_=1;
    NativeRuntimeAddress heap_;
};
}
