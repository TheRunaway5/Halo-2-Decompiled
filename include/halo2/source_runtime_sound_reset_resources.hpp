#pragma once
#include "halo2/physics_world_runtime.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/sdl_sound_platform.hpp"
#include <memory>
#include <mutex>
#include <functional>

namespace halo2 {
// A real native control queue and retained stereo voice/completion owner for
// the reached38EC8F reset. This does not implement MCPX, advertise source voice
// capacity, or execute a DSP program. Processor contexts remain unconfigured;
// the original factory must continue to its next genuine unsupported request.
class SourceRuntimeSoundResetResources final {
    struct Impl;
public:
    // Keep this handoff while borrowing a voice beyond the graph owner's
    // lifetime. It preserves the native ID, source index/context and actual
    // output lease, so pending receipts remain recoverable after teardown.
    // Account every receipt, then retire; destruction never acknowledges PCM.
    class VoiceLease final {
    public:
        VoiceLease(std::shared_ptr<SdlSoundPlatform>,std::uint16_t,std::uint64_t,SdlSoundPlatform::Voice);
        std::uint16_t source_index()const noexcept{return index_;}
        std::uint64_t context()const noexcept{return context_;}
        SdlSoundPlatform::Voice native_voice()const noexcept{return voice_;}
        std::optional<SdlSoundCompletion> completion()const;
        SdlSoundVoiceStatus status()const;
        void acknowledge(std::uint64_t submission);
        void retire();
    private:
        std::shared_ptr<SdlSoundPlatform> output_;
        std::uint16_t index_;
        std::uint64_t context_;
        SdlSoundPlatform::Voice voice_;
        std::uint32_t device_;
        mutable std::mutex mutex_;
        bool retired_{};
        void require_live()const;
    };
    // Pins the real control worker for another owned native component. Jobs
    // are generation-checked; this never supplies source FIFO availability.
    class ControlLease final {
    public:
        // Immutable native worker identity, independent of source addresses
        // and generation counters. Safe before inspecting mutable state.
        bool same_owner(const ControlLease& other)const noexcept{return impl_==other.impl_;}
        std::uint64_t generation()const;
        std::uint32_t source_apu()const;
        void require_bank(const SourceRuntimeMemory&)const;
        // Read-only creating-executor/actual bank/FS preflight before source CPU.
        void require_passive_context(const SourceRuntimeMemory&,std::uint32_t fs_base)const;
        // Creating executor first, then exact retained SDL object/device.
        void require_output(const SdlSoundPlatform&)const;
        std::shared_ptr<VoiceLease> voice(std::uint16_t)const;
        void run(std::uint64_t expected_generation,const std::function<void()>&)const;
        std::uint32_t voice_head(unsigned list)const;
        void voice_head(std::uint64_t expected_generation,unsigned list,std::uint32_t)const;
        void retire_dispatch(std::uint64_t expected_generation,std::uint16_t)const;
        bool try_exclusion(SourcePhysicsContactMemory&,const SourcePhysicsContactRuntimeCall&,
            SourcePhysicsWorldRuntimeAbi&)const;
        // The existing reset-owned native privileged frontend scope. Only the
        // authenticated38E599 bit0 operation is admitted; this is not a
        // general MCPX register or configuration facility.
        bool frontend_privileged_access()const;
        void frontend_privileged_access(std::uint64_t expected_generation,bool enabled)const;
        void abort_transaction()const;
        // Wait for the actual retained worker queue to become quiescent during
        // resource retirement, including after an interrupted source call.
        void fence_retirement()const;
    private:
        friend class SourceRuntimeSoundResetResources;
        explicit ControlLease(std::shared_ptr<Impl> p):impl_(std::move(p)){}
        std::shared_ptr<Impl> impl_;
    };
    struct State {
        std::uint64_t generation{},completed_commands{},reset_count{};
        std::size_t queued_commands{},command_capacity{},retained_voices{};
        std::uint32_t source_apu{},output_device{};
        NativeRuntimeAddress native_apu{};
        bool gp_program_loaded{},ep_program_loaded{},faulted{};
    };
    SourceRuntimeSoundResetResources(SourceRuntimeMemory&,std::shared_ptr<SdlSoundPlatform>,
        std::uint32_t passive_fs_base);
    ~SourceRuntimeSoundResetResources();
    SourceRuntimeSoundResetResources(const SourceRuntimeSoundResetResources&)=delete;
    SourceRuntimeSoundResetResources& operator=(const SourceRuntimeSoundResetResources&)=delete;
    // Also admits the exact sequential383D88/383D9D exclusion imports after
    // reset, used by38A603 storage. Outside this reset's memory seam a nested
    // source exclusion scope remains unsupported and is rejected explicitly.
    bool try_call(SourcePhysicsContactMemory&,const SourcePhysicsContactRuntimeCall&,
        SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
        const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
    State state() const;
    ControlLease control_lease()const;
    // Explicit native stereo service bindings. These are not original SDK
    // voice constructors/capability results. Reset flushes actual PCM while
    // preserving both packet slots until the caller accounts their receipts.
    [[nodiscard]] std::shared_ptr<VoiceLease> bind_voice(std::uint16_t source_index,
        std::uint64_t context,const SdlSoundVoiceParameters&);
    SdlSoundPlatform::Voice native_voice(std::uint16_t source_index) const;
    SdlSoundPlatform::Submission submit(std::uint16_t source_index,SdlSoundPacket);
    std::optional<SdlSoundCompletion> completion(std::uint16_t source_index) const;
    void acknowledge(std::uint16_t source_index,std::uint64_t submission);
    void retire_voice(std::uint16_t source_index);
    bool quiescent() const noexcept;
    // Guard a larger retained source call that can acquire the native scoped
    // exclusion service. Interrupted source stores remain diagnostic; do not
    // reuse the graph after such a partial transaction.
    class CallScope {
    public:
        explicit CallScope(SourceRuntimeSoundResetResources*);
        ~CallScope();
        CallScope(const CallScope&)=delete;
    private:
        SourceRuntimeSoundResetResources* owner_;
        int exceptions_;
    };
private:
    std::shared_ptr<Impl> impl_;
};
}
