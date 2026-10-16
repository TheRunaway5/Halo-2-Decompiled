#pragma once
#include "halo2/source_runtime_sound_codec_event_port.hpp"

namespace halo2 {
// Explicit one-span cold analog source38F0AA(-1) component. It owns actual
// run until original38F05C or cancellation; no producer/refetch/startup gate.
class SourceRuntimeSoundColdAnalogStart final {
public:
    struct State {
        std::uint64_t previous_generation{},successor_generation{};
        bool attempted{},source_started{},native_running{},source_stopped{},faulted{},source_cancelled{};
    };
    // Bank, referenced codec EventPort and creating executor outlive this
    // owner. Bank/executor also outlive every retained receipt lease.
    SourceRuntimeSoundColdAnalogStart(SourceRuntimeMemory&,
        std::shared_ptr<SourceRuntimeSoundAc97ChannelReset>,std::shared_ptr<SourceRuntimeSoundPcmBuffers>,
        std::shared_ptr<SourceRuntimeSoundCodecRoute>,SourceRuntimeSoundResetResources::ControlLease,
        SourceRuntimeSoundCodecEventPort&);
    ~SourceRuntimeSoundColdAnalogStart();
    SourceRuntimeSoundColdAnalogStart(const SourceRuntimeSoundColdAnalogStart&)=delete;
    void start(SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
        const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
    void stop(SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
        const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
    std::shared_ptr<SourceRuntimeSoundAc97ChannelReset::EndpointLease> previous_receipt_owner()const;
    State state()const;
private:
    friend class SourceRuntimeSoundAc97ChannelReset;
    friend class SourceRuntimeSoundCodecEventPort;
    struct Impl;
    std::unique_ptr<Impl> impl_;
    std::uint32_t read_halted(std::uint32_t,unsigned)const;
    void write_run(std::uint32_t,std::span<const std::byte>);
    void did_pause();
    void registered_shutdown_completed(SourceRuntimeSoundCodecEventPort&,
        const std::shared_ptr<SourceRuntimeSoundAc97ChannelReset::ConsumerLease>&);
    void abort()noexcept;
};
}
