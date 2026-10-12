#pragma once
#include "halo2/physics_platform_call.hpp"
#include "halo2/source_process_heap_runtime.hpp"
#include "halo2/source_process_shutdown.hpp"
#include "halo2/source_process_startup_records.hpp"
#include "halo2/source_runtime_host_virtual_memory.hpp"

namespace halo2 {
struct SourceProcessHeapMemoryCall {
    std::uint32_t instruction, base_cell, size_cell;
    SourceVirtualAllocationRequest request;
    SourceVirtualAllocationResult result;
};
using SourceProcessHeapMemoryObserver = std::function<void(const SourceProcessHeapMemoryCall &)>;
// Actual constructor reserve/commit call frames, authenticated separately for
// both images. Uses the same retained bank and actual host resource owner.
// Native success ABI: EAX=status, stdcall return, other CPU state preserved.
// Host resource errors stop; no Xbox failure status or kernel clobber is guessed.
bool source_try_native_process_heap_memory_call(SourceRuntimeMemory &,
    const SourcePhysicsContactRuntimeCall &, SourcePhysicsContactRuntimeAbi &,
    SourcePhysicsContactImage, SourceRuntimeHostVirtualMemory &,
    const SourceProcessHeapMemoryObserver & = {});
bool source_try_native_process_shutdown_call(SourceRuntimeMemory &,
    const SourcePhysicsContactRuntimeCall &, SourcePhysicsContactRuntimeAbi &,
    SourcePhysicsContactImage, SourceProcessShutdownRegistry &,
    const SourceProcessShutdownObserver & = {});
// Actual startup frames only. Declared native void ABI preserves CPU state
// except ESP consuming the complete stdcall frame; no Xbox clobber claim.
bool source_try_native_process_startup_record_call(SourceRuntimeMemory &,
    const SourcePhysicsContactRuntimeCall &, SourcePhysicsContactRuntimeAbi &,
    SourcePhysicsContactImage, const SourceProcessStartupRecordObserver & = {});
struct SourceProcessHeapDataImport {
    std::uint32_t instruction, actual_instruction, cell, ordinal, bytes;
};
class SourceProcessHeapDataUnavailable final : public std::runtime_error {
public:
    explicit SourceProcessHeapDataUnavailable(SourceProcessHeapDataImport request)
        : std::runtime_error("Original startup requires a retained hardware-info data owner before import 322"),
          request(request) {}
    SourceProcessHeapDataImport request;
};
// Called before the original imported-data load. A supplied owner must bind
// the actual import cell to readable retained storage, not return a value for
// the instruction or choose a synthetic hardware profile.
using SourceProcessHeapDataImportOwner = std::function<void(SourceRuntimeMemory &, const SourceProcessHeapDataImport &)>;
// Continue the compiled original constructor with shared native storage/lock
// owners. Other reached callees retain their full ABI and explicit ownership.
// Installing the process-heap global belongs to the original startup caller.
void source_run_process_heap_with_native_platform(SourceRuntimeMemory &,
    std::uint32_t canonical_entry, SourcePhysicsContactRuntimeAbi &,
    SourcePhysicsContactImage, SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRuntimeServices &, SourceRuntimeHostVirtualMemory &,
    PhysicsPlatformLocks &, const PhysicsPlatformLockServices &,
    const SourcePhysicsContactRecordObserver & = {},
    const SourceProcessHeapMemoryObserver & = {}, const PhysicsPlatformLockObserver & = {},
    SourceProcessShutdownRegistry * = nullptr, const SourceProcessShutdownObserver & = {},
    const SourceProcessStartupRecordObserver & = {}, const SourceProcessHeapDataImportOwner & = {});
} // namespace halo2
