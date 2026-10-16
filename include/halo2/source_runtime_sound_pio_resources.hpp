#pragma once
#include "halo2/source_runtime_sound_table_limits.hpp"
#include <thread>

namespace halo2 {
// Bounded native PIO payload queue and actual notifier/input-page resources.
// Bank/executor outlive this owner. Nest source transactions inside EP/SDK/
// DMA/reset scopes; no callback may borrow pages across source mutation.
// This does not admit38E8C9, tracking, HRTF processing, voices, DSP or startup.
class SourceRuntimeSoundPioResources final {
    struct Queue;
    struct CpuMemory;
public:
    struct NotifierView {std::uint32_t source{};NativeRuntimeAddress native{};std::span<const std::byte> bytes;};
    struct State {
        NativeRuntimeAddress native_queue{},native_notifiers{},native_input_record{},native_input_page{};
        std::uint32_t payload_capacity_bytes{},available_payload_bytes{},queued_commands{},selected_input_sge{};
        std::uint64_t generation{},completed_commands{},cancelled_commands{};
        bool notifier_published{},input_page_bound{},retired{},faulted{},full_configuration_ready{};
    };
    SourceRuntimeSoundPioResources(SourceRuntimeMemory&,
        std::shared_ptr<SourceRuntimeSoundDmaResources>,
        std::shared_ptr<SourceRuntimeSoundEpOutputResources>,
        std::shared_ptr<SourceRuntimeSoundTableLimits>,std::shared_ptr<SdlSoundPlatform>,
        SourceRuntimeSoundResetResources::ControlLease);
    ~SourceRuntimeSoundPioResources();
    SourceRuntimeSoundPioResources(const SourceRuntimeSoundPioResources&)=delete;
    SourceRuntimeSoundPioResources& operator=(const SourceRuntimeSoundPioResources&)=delete;
    void require_control(const SourceRuntimeMemory&,const SourceRuntimeSoundResetResources::ControlLease&)const;
    // Genuine source38E0CE from its initial actual queue-capacity poll through
    // voice indices, five table bases and notifier-domain publication. Suspend
    // BEFORE38E140, the first unsupported tracking method. No source RET.
    std::uint32_t run_prefix(SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
        const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
    // Separately declared component only: unchanged804/808 CPU instructions
    // at38E159..38E16D. This does not execute/skip the earlier tracking methods
    // in any cold chain. It queues and drains a real retained input-page bind.
    std::uint32_t run_input_component(SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
        const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
    // Real bounded native operation for the already selected, retained page;
    // reports actual queue backpressure. No source ABI/ready result is given.
    bool queue_native_input_page();
    void drain();
    std::uint32_t available_payload_bytes()const;
    // Source channels address the actual64-byte row after its32-byte header.
    // Borrowed only until the next source transaction/reset/retirement.
    NotifierView notifier(std::uint16_t source_voice)const;
    State diagnostics()const;
    void retire(); // Fence/cancel pending work before releasing the exact claim.
private:
    SourceRuntimeMemory& memory_;
    const std::thread::id thread_=std::this_thread::get_id();
    std::shared_ptr<SourceRuntimeSoundDmaResources> dma_;
    std::shared_ptr<SourceRuntimeSoundEpOutputResources> ep_;
    std::shared_ptr<SourceRuntimeSoundTableLimits> tables_;
    std::shared_ptr<SdlSoundPlatform> output_;
    SourceRuntimeSoundResetResources::ControlLease control_;
    const std::uint64_t generation_;
    SourceRuntimeSoundDmaResources::Allocation notifiers_,input_page_;
    std::unique_ptr<Queue> queue_;
    std::unique_ptr<CpuMemory> cpu_;
    std::uint32_t pc_{},selected_{};
    std::uint64_t completed_{},cancelled_{};
    NativeRuntimeAddress input_record_{};
    bool claimed_{},running_{},notifier_published_{},selection_queued_{},input_queued_{},input_bound_{},retired_{},fault_{};
    void require_thread()const;
    void require_live()const;
    void write_action(std::uint32_t,std::uint32_t,std::span<const std::byte>);
    std::uint32_t run(std::uint32_t,std::uint32_t,SourcePhysicsWorldRuntimeAbi&,
        SourcePhysicsFactoryFloatingPoint,const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver&);
    bool enqueue(bool selection);
    void release_claim()noexcept;
    void abort()noexcept;
};
}
