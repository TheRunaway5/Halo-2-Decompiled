#pragma once
#include "halo2/source_tu15_status.hpp"

namespace halo2 {
struct SourceTu15TimerDataRequest { std::uint32_t instruction,address,bytes; };
class SourceTu15TimerDataUnavailable final : public std::runtime_error {
public:
    explicit SourceTu15TimerDataUnavailable(SourceTu15TimerDataRequest r)
        :std::runtime_error("Original timer requires its actual kernel context/clock resource owner"),request(r) {}
    SourceTu15TimerDataRequest request;
};
// Validate/bind actual retained data before the original read; never supply
// an instruction result, captured scheduler state or a successful timer return.
using SourceTu15TimerDataOwner=std::function<void(SourcePhysicsContactMemory &,SourceTu15TimerDataRequest)>;
// Complete original modified-Complex4627 bodies for149/150,129, timer queue
// initialization, DPC initialization and due/index/insertion helpers. Unknown
// context/clock, interrupt lowering and expiry owners remain explicit.
void source_run_tu15_timer_runtime(SourcePhysicsContactMemory &,std::uint32_t actual_entry,
    SourcePhysicsContactRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRuntimeServices &,const SourcePhysicsContactRecordObserver & = {},
    const SourceTu15TimerDataOwner & = {});
// Authenticates the complete captured immutable .text, then retains only the
// nine arithmetic-division constant bytes used by the timer index helper.
void source_mount_tu15_timer_constants(SourceRuntimeMemory &,const std::filesystem::path &kernel_text);
void source_run_tu15_process_startup_with_timer(SourceRuntimeMemory &,std::uint32_t canonical_entry,
    SourcePhysicsContactRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRuntimeServices &,SourceRuntimeHostVirtualMemory &,PhysicsPlatformLocks &,
    const PhysicsPlatformLockServices &,SourceProcessShutdownRegistry &,
    const SourceTu15UserSettings &,const SourceTu15StatusTables &,
    const SourcePhysicsContactRecordObserver & = {},const SourcePhysicsContactRecordObserver & = {},
    const SourceProcessShutdownObserver & = {},const SourcePhysicsContactRecordObserver & = {},
    const SourceProcessHeapDataImportOwner & = {},const std::function<void(std::uint32_t)> & = {},
    const SourcePhysicsContactRecordObserver & = {},const SourceTu15TimerDataOwner & = {});
}
