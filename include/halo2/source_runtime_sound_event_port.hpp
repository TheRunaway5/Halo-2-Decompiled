#pragma once
#include "halo2/source_runtime_sound_frontend_events.hpp"
#include <memory>

namespace halo2 {
struct SourceRuntimeSoundInterruptGate;
// Concrete native registration/dispatcher resources for the late389378 calls.
// The legacy vector output is an opaque cookie bound to the retained eventfd,
// not an Xbox PCI vector or host IRQL. Full38E53D/D8 admission is still required.
class SourceRuntimeSoundEventPort final {
public:
    struct State {
        std::uint64_t generation{},route_identity{};
        NativeRuntimeAddress native_route{};
        int interrupt_descriptor=-1,timer_descriptor=-1,dispatcher_descriptor=-1;
        bool timer_initialized{},interrupt_initialized{},connected{},retired{},faulted{};
        std::uint32_t route_cookie{};
        std::uint64_t delivered_irqs{},cancelled_irqs{};
        bool pending_irq{};
    };
    // Bank and frontend outlive this owner. Output and control leases pin the
    // actual shared device/worker. Source callbacks stay on its creating thread.
    SourceRuntimeSoundEventPort(SourceRuntimeMemory&,SourceRuntimeSoundResetResources&,
        SourceRuntimeSoundFrontendEvents&,std::shared_ptr<SdlSoundPlatform>);
    // Destruction, including an already retired owner, must occur on its
    // immutable creating thread. Foreign destruction terminates before any
    // mutable state, callback, source bank or native FD teardown is touched.
    // The exclusive timed-notification owner must release its lease first.
    ~SourceRuntimeSoundEventPort();
    SourceRuntimeSoundEventPort(const SourceRuntimeSoundEventPort&)=delete;
    bool try_call(SourcePhysicsContactMemory&,const SourcePhysicsContactRuntimeCall&,
        SourcePhysicsWorldRuntimeAbi&,const SourcePhysicsContactRecordObserver& = {});
    // Explicit bounded native component route. A signal requires the actual
    // retained frontend's idle trap; this does not admit full source38E53D/D8.
    void signal_known_idle();
    bool dispatch_known_idle(SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
        const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
    State state()const;
    void retire_generation();
    class CallScope final {
    public:
        explicit CallScope(SourceRuntimeSoundEventPort*);
        ~CallScope();
        CallScope(const CallScope&)=delete;
    private:
        SourceRuntimeSoundEventPort* owner_;
        int exceptions_;
    };
private:
    friend class SourceRuntimeSoundTimedNotifications;
    struct TimerLease {
        const SourceRuntimeSoundResetResources::ControlLease control;
        const std::uint64_t generation;
        const int descriptor;
    };
    std::shared_ptr<const TimerLease> claim_timer_binding(const SourceRuntimeMemory&,
        const SourceRuntimeSoundResetResources::ControlLease&,SourceRuntimeSoundFrontendEvents&);
    int require_timer_lease(const SourceRuntimeMemory&,const SourceRuntimeSoundResetResources::ControlLease&,
        const std::shared_ptr<const TimerLease>&)const;
    void release_timer_lease(const std::shared_ptr<const TimerLease>&);
    int require_timer_binding(const SourceRuntimeMemory&,const SourceRuntimeSoundResetResources::ControlLease&)const;
    friend class SourceRuntimeSoundInterruptPolicy;
    std::shared_ptr<SourceRuntimeSoundInterruptGate> attach_disabled_source_policy(
        const SourceRuntimeMemory&,const SourceRuntimeSoundResetResources::ControlLease&);
    void fault_parent_call()noexcept;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
// Cold mutable cleanup cells and exact immutable import operands. No resource
// identity, callback, connected flag, timer outcome or readiness is preset.
void mount_native_tu15_sound_event_port_data(SourceRuntimeMemory&);
}
