#pragma once
#include "halo2/source_runtime_sound_ep_output_resources.hpp"
#include "halo2/source_runtime_sound_reset_resources.hpp"

namespace halo2 {
// Physical page prerequisite for genuine38F104 channel construction. The
// source CPU owns channel fields, clears, statistics and descriptor writes.
// Codec/interrupt registration, channel reset/poll, restart and consumption
// are unowned; these pages do not make an AC97 channel or DSP producer ready.
// Bank, executor and native SDK source objects outlive every live operation.
// There must be no EP callback leases during source channel construction.
// Nest CallScope within EP/SDK/DMA/reset scopes; an interruption aborts the
// real control exclusion and retains source/page bytes for diagnostics.
class SourceRuntimeSoundAc97OutputPages final {
    struct CpuMemory;
public:
    struct Allocation {
        std::uint32_t source{},physical{},requested{},capacity{},channel{},index{},field{};
        NativeRuntimeAddress native{},native_channel{};
    };
    struct State {
        std::vector<Allocation> allocations;
        std::uint64_t generation{};
        std::uint32_t output_device{},controller{};
        unsigned source_exclusion_level{}; // Interrupted CPU snapshot after fault.
        bool faulted{},channel_ready{},producer_ready{};
    };
    SourceRuntimeSoundAc97OutputPages(SourceRuntimeMemory&,
        std::shared_ptr<SourceRuntimeSoundDmaResources>,
        std::shared_ptr<SourceRuntimeSoundEpOutputResources>,
        std::shared_ptr<SdlSoundPlatform>,SourceRuntimeSoundResetResources::ControlLease);
    ~SourceRuntimeSoundAc97OutputPages();
    SourceRuntimeSoundAc97OutputPages(const SourceRuntimeSoundAc97OutputPages&)=delete;
    SourceRuntimeSoundAc97OutputPages& operator=(const SourceRuntimeSoundAc97OutputPages&)=delete;
    bool dispatch(SourcePhysicsContactMemory&,const SourcePhysicsContactRuntimeCall&,SourcePhysicsWorldRuntimeAbi&);
    void require_bank(const SourceRuntimeMemory&)const; // Immutable identity; no bank bytes read.
    void require_control(const SourceRuntimeMemory&,const SourceRuntimeSoundResetResources::ControlLease&)const;
    // Every nested CPU helper uses this same view. Its FS domain byte follows
    // the actual ControlLease exclusion acquisition/release, not host IRQL or
    // a hardware register image. Platform resources still receive the bank.
    SourcePhysicsContactMemory& cpu_memory();
    bool try_exclusion(const SourcePhysicsContactRuntimeCall&,SourcePhysicsWorldRuntimeAbi&);
    State diagnostics()const; // Creating thread; survives a fault for teardown.
    bool quiescent()const noexcept;
    class CallScope final {
    public:
        explicit CallScope(SourceRuntimeSoundAc97OutputPages*);
        ~CallScope();
        CallScope(const CallScope&)=delete;
        CallScope& operator=(const CallScope&)=delete;
    private:
        SourceRuntimeSoundAc97OutputPages* owner_;
        int exceptions_;
    };
private:
    friend class SourceRuntimeSoundAc97PageRelease;
    // Only the authenticated physical-release adapter, after genuine native
    // stop/reset/detach. Original CPU still owns fields/statistics cleanup.
    void release_owned_page(const Allocation&);
    SourceRuntimeMemory& memory_;
    const std::thread::id thread_=std::this_thread::get_id();
    std::shared_ptr<SourceRuntimeSoundDmaResources> dma_;
    std::shared_ptr<SourceRuntimeSoundEpOutputResources> ep_;
    std::shared_ptr<SdlSoundPlatform> output_;
    SourceRuntimeSoundResetResources::ControlLease control_;
    SourceRuntimeHostPhysicalMemory physical_;
    const std::uint64_t generation_;
    const std::uint32_t device_,controller_;
    std::map<std::uint32_t,Allocation> allocations_;
    std::unique_ptr<CpuMemory> cpu_memory_;
    std::uint32_t fs_{};
    unsigned source_level_{};
    bool fault_{};
    void require_thread()const;
    void require_live()const;
    unsigned validate_channel(std::uint32_t)const;
};
}
