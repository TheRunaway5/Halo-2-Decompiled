#pragma once
#include "halo2/source_runtime_sound_ac97_channel_reset.hpp"
#include <array>

namespace halo2 {
// Actual default PCM buffer/descriptor binding for the original38E484
// calls to38EF59. Source CPU retains all descriptor/index/exclusion writes.
// No source output start, DSP producer, encoded SPDIF or20xx scheduler word
// is admitted by this resource prerequisite.
class SourceRuntimeSoundPcmBuffers final {
public:
    using Kind=SourceRuntimeSoundAc97ChannelReset::Kind;
    struct Binding {
        Kind kind{};
        std::uint32_t channel{},descriptor_source{},buffer_source{},buffer_physical{},bytes{};
        NativeRuntimeAddress native_channel{},native_descriptors{},native_buffer{};
        SdlSoundPlatform::Voice native_voice{};
        std::uint64_t endpoint_generation{};
    };
    struct View {Binding binding;std::span<const std::byte> bytes;};
    struct State {std::vector<Binding> bindings;std::uint64_t generation{};bool retired{},faulted{},source_started{},producer_ready{};};
    SourceRuntimeSoundPcmBuffers(SourceRuntimeMemory&,
        std::shared_ptr<SourceRuntimeSoundDmaResources>,std::shared_ptr<SourceRuntimeSoundEpOutputResources>,
        std::shared_ptr<SourceRuntimeSoundAc97OutputPages>,std::shared_ptr<SourceRuntimeSoundAc97ChannelReset>,
        SourceRuntimeSoundResetResources::ControlLease);
    ~SourceRuntimeSoundPcmBuffers();
    SourceRuntimeSoundPcmBuffers(const SourceRuntimeSoundPcmBuffers&)=delete;
    SourceRuntimeSoundPcmBuffers& operator=(const SourceRuntimeSoundPcmBuffers&)=delete;
    bool try_bind(const SourcePhysicsContactRuntimeCall&,SourcePhysicsWorldRuntimeAbi&,
        SourcePhysicsFactoryFloatingPoint,const SourcePhysicsWorldRuntimeServices&,
        const SourcePhysicsContactRecordObserver& = {});
    // Borrowed creating-thread views. Do not retain across source writes,
    // reset, retirement or destruction; bank/executor outlive this owner.
    View buffer(Kind)const;
    State diagnostics()const;
    // Retire the actual native binding before authentic source38F05C page
    // release. Retained typed prepared-range borrowers are cancelled/fenced
    // before this channel relinquishes actual descriptor registration.
    void retire(Kind);
    bool channel_quiescent(Kind)const noexcept;
    void retire();
    bool quiescent()const noexcept;
private:
    friend class SourceRuntimeSoundPcmPreparedRange;
    friend class SourceRuntimeSoundAc97ChannelReset;
    friend class SourceRuntimeSoundCodecEventPort;
    friend class SourceRuntimeSoundColdAnalogStart;
    SourceRuntimeMemory& memory_;
    const std::thread::id thread_=std::this_thread::get_id();
    std::shared_ptr<SourceRuntimeSoundDmaResources> dma_;
    std::shared_ptr<SourceRuntimeSoundEpOutputResources> ep_;
    std::shared_ptr<SourceRuntimeSoundAc97OutputPages> pages_;
    std::shared_ptr<SourceRuntimeSoundAc97ChannelReset> channels_;
    SourceRuntimeSoundResetResources::ControlLease control_;
    const std::uint64_t generation_;
    const std::uint32_t device_,controller_;
    std::map<Kind,Binding> bindings_;
    std::map<Kind,std::shared_ptr<SourceRuntimeSoundAc97ChannelReset::EndpointLease>> endpoints_;
    std::map<Kind,std::weak_ptr<SourceRuntimeSoundEpOutputResources::PreparedRangeLease>> prepared_borrowers_;
    bool retired_{},fault_{};
    void require_thread()const;
    void require_live()const;
    const Binding& resolve(Kind)const;
};
// Thread-safe ownership admission for the physical-page teardown adapter.
// This queries a real live binding, never source hardware/ready state.
bool source_runtime_sound_pcm_descriptor_bound(NativeRuntimeAddress descriptor_page);
}
