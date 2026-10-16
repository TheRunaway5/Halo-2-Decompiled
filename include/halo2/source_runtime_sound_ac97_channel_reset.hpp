#pragma once
#include "halo2/source_runtime_sound_ac97_output_pages.hpp"
#include <mutex>

namespace halo2 {
class SourceRuntimeSoundPcmBuffers;
class SourceRuntimeSoundColdAnalogStart;
// Genuine38EFEA channel reset binds actual paused native endpoints to the
// retained physical descriptor tables. Reset uses the shared control worker,
// flushes only its endpoint and preserves every native PCM receipt. This is
// not codec/interrupt initialization,38F0AA start/recovery or a DSP producer.
class SourceRuntimeSoundAc97ChannelReset final {
    struct Endpoint;
    struct CpuMemory;
public:
    enum class Kind { Analog,Spdif };
    class EndpointLease final {
    public:
        Kind kind()const noexcept;
        std::uint32_t source_channel()const noexcept;
        SdlSoundPlatform::Voice native_voice()const noexcept;
        // Creating-executor native PCM service, not source DSP admission. A reset
        // invalidates submission by older leases but preserves their receipts.
        SdlSoundPlatform::Submission submit_native(SdlSoundPacket);
        SdlSoundVoiceStatus status()const;
        std::optional<SdlSoundCompletion> completion()const;
        void acknowledge(std::uint64_t submission);
        // Creating thread, after all actual receipts have been accounted.
        // Stale handles become inert before last page/output references drop.
        void retire();
    private:
        friend class SourceRuntimeSoundAc97ChannelReset;
        EndpointLease(std::shared_ptr<Endpoint>,std::uint64_t);
        std::shared_ptr<Endpoint> endpoint_;
        const std::uint64_t generation_;
    };
    // Exclusive native component consumer for one authenticated default
    // analog descriptor. This does not admit source38F0AA or a DSP producer.
    class ConsumerLease final {
        struct Domain;
    public:
        ~ConsumerLease();
        void validate_delivery()const;
        void submit_and_start();
        bool pause_after_success();
        bool observe_success_running();
        std::optional<SdlSoundCompletion> completion()const;
        void acknowledge();
        void cancel();
        std::shared_ptr<EndpointLease> receipt_owner()const;
    private:
        friend class SourceRuntimeSoundAc97ChannelReset;
        friend class SourceRuntimeSoundColdAnalogStart;
        explicit ConsumerLease(std::unique_ptr<Domain>);
        std::unique_ptr<Domain> domain_;
    };
    std::shared_ptr<ConsumerLease> claim_default_analog_consumer(
        std::shared_ptr<SourceRuntimeSoundPcmBuffers>,std::shared_ptr<const std::vector<float>>);
    // Creating-thread cancellation before actual descriptor ownership drops.
    void cancel_native_consumer(Kind);
    struct EndpointState {
        Kind kind{};
        std::uint32_t source_channel{},descriptor_physical{};
        std::uint32_t iec958_channel_status{};
        NativeRuntimeAddress native_channel{},native_descriptors{};
        SdlSoundPlatform::Voice native_voice{};
        std::uint64_t generation{},completed_resets{};
        std::uint8_t source_current{},source_last{};
        bool resetting{},descriptor_table_bound{},pcm48_profile_bound{},admission_closed{},retired{},faulted{};
    };
    struct State {
        std::vector<EndpointState> endpoints;
        bool faulted{},source_channel_started{},producer_ready{};
    };
    SourceRuntimeSoundAc97ChannelReset(SourceRuntimeMemory&,
        std::shared_ptr<SourceRuntimeSoundAc97OutputPages>,std::shared_ptr<SdlSoundPlatform>,
        SourceRuntimeSoundResetResources::ControlLease);
    ~SourceRuntimeSoundAc97ChannelReset();
    SourceRuntimeSoundAc97ChannelReset(const SourceRuntimeSoundAc97ChannelReset&)=delete;
    SourceRuntimeSoundAc97ChannelReset& operator=(const SourceRuntimeSoundAc97ChannelReset&)=delete;
    // Reject foreign executors/banks before any mutable source or endpoint
    // access, then authenticate the actual control/page/output generation.
    void require_bank(const SourceRuntimeMemory&)const;
    void require_control(const SourceRuntimeMemory&,const SourceRuntimeSoundResetResources::ControlLease&)const;
    // Authentic cleanup only, after source stop/reset/indices0. Fence and
    // detach a genuinely owned table on the shared worker, retaining voice
    // and unaccounted PCM receipts. Caller first retires PCM buffer owners.
    void prepare_page_release(const SourceRuntimeSoundAc97OutputPages::Allocation&);
    void bind_channel(std::uint32_t); // After actual38EEF1, before38F104.
    std::shared_ptr<EndpointLease> endpoint(Kind)const;
    // The same scoped view/PC observer covers all nested CPU modules. Native
    // platform services receive the original bank through required_owner.
    void run_cpu(std::uint32_t,SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
        const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
    State diagnostics()const;
    class CallScope final {
    public:
        explicit CallScope(SourceRuntimeSoundAc97ChannelReset*);
        ~CallScope();
        CallScope(const CallScope&)=delete;
        CallScope& operator=(const CallScope&)=delete;
    private:
        SourceRuntimeSoundAc97ChannelReset* owner_;
        int exceptions_;
    };
private:
    friend class SourceRuntimeSoundColdAnalogStart;
    friend class SourceRuntimeSoundCodecEventPort;
    SourceRuntimeSoundColdAnalogStart* cold_start_{};
    SourceRuntimeSoundColdAnalogStart* cold_start_owner_{};
    SourceRuntimeMemory& memory_;
    const std::thread::id thread_=std::this_thread::get_id();
    std::shared_ptr<SourceRuntimeSoundAc97OutputPages> pages_;
    std::shared_ptr<SdlSoundPlatform> output_;
    SourceRuntimeSoundResetResources::ControlLease control_;
    const std::uint64_t generation_;
    const std::uint32_t device_,controller_;
    std::map<Kind,std::shared_ptr<Endpoint>> endpoints_;
    std::unique_ptr<CpuMemory> cpu_memory_;
    std::uint32_t pc_{};
    bool running_{},fault_{};
    void require_thread()const;
    void require_live()const;
    std::shared_ptr<Endpoint> at_address(std::uint32_t,unsigned)const;
    std::uint32_t read_control(std::uint32_t,unsigned)const;
    void write_control(std::uint32_t,unsigned,std::uint32_t);
    void close_source_channel(std::uint32_t);
    void abort()noexcept;
};
}
