#pragma once
#include "halo2/source_process_heap_platform.hpp"
#include "halo2/source_runtime_image.hpp"

namespace halo2 {
// TU1.5 startup's exact 107/113 call frames. These native void operations
// initialize only the fields owned by the pinned compatibility definitions.
// They preserve other CPU state and do not arm a timer, queue a DPC, supply
// hardware/settings data, install a heap or claim an Xbox kernel ABI.
bool source_try_tu15_startup_record_call(SourceRuntimeMemory &,
    const SourcePhysicsContactRuntimeCall &, SourcePhysicsContactRuntimeAbi &,
    const SourcePhysicsContactRecordObserver & = {});
// Validate the actual loader-bound record at the source import read. No flags,
// defaults or instruction results are supplied by this ownership callback.
SourceProcessHeapDataImportOwner source_tu15_hardware_import_owner(
    const SourceRuntimeKernelHardwareInfo &);

// Compose the existing source heap/platform/shutdown owners with those two
// record operations. Entry IDs are retained internal correspondence IDs;
// execution and return/argument validation select English TU1.5 exclusively.
void source_run_tu15_process_startup_records(SourceRuntimeMemory &,
    std::uint32_t canonical_entry, SourcePhysicsContactRuntimeAbi &,
    SourcePhysicsFactoryFloatingPoint, const SourcePhysicsContactRuntimeServices &,
    SourceRuntimeHostVirtualMemory &, PhysicsPlatformLocks &,
    const PhysicsPlatformLockServices &, SourceProcessShutdownRegistry &,
    const SourcePhysicsContactRecordObserver & = {},
    const SourcePhysicsContactRecordObserver & = {},
    const SourceProcessShutdownObserver & = {},
    const SourceProcessHeapDataImportOwner & = {});
} // namespace halo2
