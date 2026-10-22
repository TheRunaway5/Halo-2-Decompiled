#pragma once
#include "halo2/native_tu15_startup_platform.hpp"
#include <memory>

namespace halo2 {
struct NativeTu15TimerClock {
    std::function<std::uint64_t()> interrupt_time;
    std::function<std::uint64_t()> system_time;
};
// Real monotonic/UTC clock values in100ns units. Native boot clocks are not
// snapshots of Xbox kernel globals. Explicit procedure clock owners can be
// supplied to comparison tools; the production factory measures host time.
NativeTu15TimerClock native_tu15_timer_clock();
struct NativeTu15TimerObservation {
    std::uint64_t interrupt_time, due_time;
    std::int64_t interval;
    bool previously_inserted;
};
using NativeTu15TimerObserver=std::function<void(NativeTu15TimerObservation)>;
struct NativeTu15TimerDpc {
    std::uint32_t address,routine,context,argument1,argument2;
};
// Owns the actual relative one-shot service reached by TU1.5 process startup.
// Calls and source-bank access are serialized by the application's executor.
// A native monotonic timer owns expiration; polling runs on that same executor. Existing
// timer waiters and foreign timers/DPCs require their actual platform owner.
class NativeTu15StartupTimer final {
public:
    NativeTu15StartupTimer(SourceRuntimeMemory &,NativeTu15TimerClock,
                          std::uint32_t list_head=0x90001000u);
    ~NativeTu15StartupTimer();
    NativeTu15StartupTimer(const NativeTu15StartupTimer &)=delete;
    NativeTu15StartupTimer &operator=(const NativeTu15StartupTimer &)=delete;
    bool try_call(const SourcePhysicsContactRuntimeCall &,SourcePhysicsContactRuntimeAbi &,
                  const SourcePhysicsContactRecordObserver & = {},const NativeTu15TimerObserver & = {});
    bool cancel(const SourcePhysicsContactRecordObserver & = {});
    // Consumes real native expiration, signals the source timer and invokes
    // its retained actual DPC. An executor is required before consuming it.
    bool dispatch_due(const std::function<void(NativeTu15TimerDpc)> &,
                      const SourcePhysicsContactRecordObserver & = {});
    std::uint64_t remaining_100ns() const;
    int native_descriptor() const;
    bool backs(const SourceRuntimeMemory &) const;
private:
    struct State;
    std::unique_ptr<State> state_;
};
// Execute the complete original four-instruction TU1.5 power DPC. Import360
// remains an application lifecycle owner; no host shutdown is performed here.
void source_run_tu15_power_dpc(SourcePhysicsContactMemory &,SourcePhysicsContactRuntimeAbi &,
    SourcePhysicsFactoryFloatingPoint,const SourcePhysicsContactRuntimeServices &,
    const SourcePhysicsContactRecordObserver & = {});
// Connect the real timer operation directly to the original native startup
// caller. Every later process-heap/world owner remains an explicit dependency.
void source_run_native_tu15_startup_with_timer(const NativeTu15StartupPlatform &,
    NativeTu15StartupTimer &,SourceRuntimeMemory &,std::uint32_t canonical_entry,
    SourcePhysicsContactRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsContactRuntimeServices &,SourceRuntimeHostVirtualMemory &,
    PhysicsPlatformLocks &,const PhysicsPlatformLockServices &,SourceProcessShutdownRegistry &,
    const SourcePhysicsContactRecordObserver & = {},const SourcePhysicsContactRecordObserver & = {},
    const SourceProcessShutdownObserver & = {},const SourcePhysicsContactRecordObserver & = {},
    const NativeTu15TimerObserver & = {},const SourceProcessHeapMemoryObserver & = {},
    const PhysicsPlatformLockObserver & = {});
} // namespace halo2
