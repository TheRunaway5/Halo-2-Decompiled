#pragma once
#include "halo2/source_tu15_timer.hpp"
namespace halo2 {
struct SourceTu15InterruptPortWrite {
    std::uint32_t instruction, port;
    std::uint8_t value;
};
using SourceTu15InterruptPortOwner=std::function<void(SourceTu15InterruptPortWrite)>;
void source_run_tu15_interrupt_runtime(SourcePhysicsContactMemory &,std::uint32_t actual_entry,
    SourcePhysicsContactRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRuntimeServices &,const SourcePhysicsContactRecordObserver & = {},
    const SourceTu15TimerDataOwner & = {},const SourceTu15InterruptPortOwner & = {});
// Compose the original timer's actual lower-IRQL caller with both complete
// lowering bodies. Context/clock reads, scheduler callees and PIC I/O remain
// explicit resource owners, including exceptional partial execution.
SourcePhysicsContactRuntimeServices source_tu15_interrupt_services(
    const SourcePhysicsContactRuntimeServices &,SourcePhysicsFactoryFloatingPoint,
    const SourceTu15TimerDataOwner &,const SourceTu15InterruptPortOwner & = {},
    const std::function<void(std::uint32_t)> &kernel_instruction = {});
void source_run_tu15_process_startup_with_interrupts(SourceRuntimeMemory &,std::uint32_t canonical_entry,
    SourcePhysicsContactRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRuntimeServices &,SourceRuntimeHostVirtualMemory &,PhysicsPlatformLocks &,
    const PhysicsPlatformLockServices &,SourceProcessShutdownRegistry &,
    const SourceTu15UserSettings &,const SourceTu15StatusTables &,
    const SourcePhysicsContactRecordObserver & = {},const SourcePhysicsContactRecordObserver & = {},
    const SourceProcessShutdownObserver & = {},const SourcePhysicsContactRecordObserver & = {},
    const SourceProcessHeapDataImportOwner & = {},const std::function<void(std::uint32_t)> & = {},
    const SourcePhysicsContactRecordObserver & = {},const SourceTu15TimerDataOwner & = {},
    const SourceTu15InterruptPortOwner & = {});
// Retain only the original read-only IRQL masks from authenticated kernel text.
void source_mount_tu15_interrupt_masks(SourceRuntimeMemory &,const std::filesystem::path &);
}
