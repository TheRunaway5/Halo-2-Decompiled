#pragma once
#include "halo2/source_runtime_sound_reset_resources.hpp"
#include "halo2/source_runtime_sound_dma_resources.hpp"
#include <memory>

namespace halo2 {
// Known frontend trap/event resource component. The complete38E8C9 request
// sequence still has unresolved required fields; this owner never completes
// that helper or reports a configured audio device/FIFO/DSP.
class SourceRuntimeSoundFrontendEvents final {
public:
    enum class Mode { Halted,Running,Trapped };
    enum class Admission { Accepted,Backpressure };
    struct DeferredCall {
        std::uint64_t identity{},generation{};
        std::uint32_t source_dpc{},routine{},context{};
        NativeRuntimeAddress native{};
    };
    struct State {
        std::uint64_t generation{};
        Mode mode{};
        std::size_t command_capacity{},queued_commands{},queued_dpcs{},cancelled_commands{},cancelled_dpcs{};
        std::uint32_t interrupt_status{};
        bool idle_trap_enabled{},faulted{};
        std::uint64_t completed_dpcs{},completed_fences{};
    };
    // This establishes a real trap-only native event route and an actually
    // bounded command queue. It does not consume/accept unknown source IEN
    // bits3/7 or the unnamed low frontend policy bits in38E8C9.
    // The memory and DMA owners outlive this component. The retained control
    // lease pins the actual worker; each voice handoff pins its SDL output.
    SourceRuntimeSoundFrontendEvents(SourceRuntimeMemory&,SourceRuntimeSoundResetResources&,
        SourceRuntimeSoundDmaResources&,std::size_t command_capacity);
    ~SourceRuntimeSoundFrontendEvents();
    SourceRuntimeSoundFrontendEvents(const SourceRuntimeSoundFrontendEvents&)=delete;
    bool try_initialize_dpc(SourcePhysicsContactMemory&,const SourcePhysicsContactRuntimeCall&,
        SourcePhysicsWorldRuntimeAbi&);
    // Exact known38E8EB/FETFORCE1 request. This is a subresource request only;
    // earlier/later unowned source configuration requests must still stop.
    void prepare_idle_trap(std::uint32_t instruction,std::uint32_t address,std::uint32_t value);
    void bind(std::uint16_t,std::uint32_t source_channel,
        std::shared_ptr<SourceRuntimeSoundResetResources::VoiceLease>);
    // Admission requires an actual Success completion, no pending native PCM,
    // genuine source slot/notifier bindings and already-inactive nonpersistent
    // source voice state. It never clears active/loop state to fabricate idle.
    Admission idle_voice(std::uint16_t);
    void process();
    void source_mode_helper(SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
        const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
    bool dispatch_interrupt(SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
        const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
    std::optional<DeferredCall> pending_dpc()const;
    // Execute the registered original deferred thunk against its retained
    // actual event domain, ordered DMA fence and base voice notifications.
    // Unsupported virtual/timer/PIO branches fault and preserve queued work.
    // The record is removed only after the whole original thunk returns.
    bool dispatch_dpc(SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
        const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
    // Inspection does not dequeue work or manufacture callback success.
    State state()const;
    void retire_generation();
private:
    friend class SourceRuntimeSoundEventPort;
    friend class SourceRuntimeSoundTimedNotifications;
    void initialize_timed_dpc(SourcePhysicsContactMemory&,const SourcePhysicsContactRuntimeCall&,SourcePhysicsWorldRuntimeAbi&);
    bool queue_timed_dpc();
    bool remove_timed_dpc();
    void require_timer_queue_retired()const;
    bool dispatch_timed_dpc(SourcePhysicsWorldRuntimeAbi&,const std::function<void()>&);
    void require_timed_voice(std::uint32_t channel,std::uint32_t record)const;
    void require_binding(const SourceRuntimeMemory&,const SourceRuntimeSoundResetResources::ControlLease&)const;
    bool dispatch_registered_interrupt(SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
        const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver&);
    bool dispatch_interrupt_entry(std::uint32_t,SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
        const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver&);
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
