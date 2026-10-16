#pragma once
#include "halo2/source_runtime_sound_event_port.hpp"

namespace halo2 {
// Exact source IEN=D8 with global-enable clear. This configures the real
// EventPort source callback gate, preserving pending events through later
// registration. It never declares the unresolved requested routes ready.
// Source bank and creating executor outlive this owner. The port outlives CPU
// calls; port retirement invalidates the retained gate before native teardown.
class SourceRuntimeSoundInterruptPolicy final {
    struct CpuMemory;
public:
    struct State {std::uint64_t generation{};NativeRuntimeAddress native_gate{};int dispatcher=-1;
        std::uint32_t pending_mask{},unresolved_requested_mask{};
        bool prepared{},source_delivery_enabled{},retired{},faulted{},full_configuration_ready{};};
    SourceRuntimeSoundInterruptPolicy(SourceRuntimeMemory&,SourceRuntimeSoundEventPort&,
        SourceRuntimeSoundResetResources::ControlLease);
    ~SourceRuntimeSoundInterruptPolicy();
    SourceRuntimeSoundInterruptPolicy(const SourceRuntimeSoundInterruptPolicy&)=delete;
    // Exact typed store seam for a caller's scoped CPU view; no other IEN
    // mask, global-enable value or source MMIO address is accepted.
    void apply_disabled_source_store(std::uint32_t instruction,std::uint32_t address,std::uint32_t value);
    // Executes the authenticated original38E8C9 MOV action store with an
    // unknown/empty observer preimage, then stops before
    //38E8D3's unowned frontend policy. No skipped instruction or return value.
    std::uint32_t run_initial_prefix(SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
        const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
    // Unchanged original38E53D. Disable is supported; any enable request is
    // rejected and aborts the actual shared transaction before delivery opens.
    void source_global_helper(SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
        const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
    State diagnostics()const;
    void retire();
private:
    SourceRuntimeMemory& memory_;
    const std::thread::id thread_=std::this_thread::get_id();
    std::shared_ptr<SourceRuntimeSoundInterruptGate> gate_;
    std::unique_ptr<CpuMemory> cpu_;
    std::uint32_t pc_{},source_level_{},cpu_fs_{};
    bool running_{};
    void require_thread()const;
    void require_live()const;
    void abort();
};
}
