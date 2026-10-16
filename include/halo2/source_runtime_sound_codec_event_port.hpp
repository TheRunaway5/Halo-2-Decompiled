#pragma once
#include "halo2/source_runtime_sound_codec_route.hpp"
#include "halo2/source_runtime_sound_pcm_buffers.hpp"
#include "halo2/source_process_shutdown.hpp"

namespace halo2 {
// Separate actual native IRQ6 completion/lifecycle domain. This bounded
// component never admits DSP PCM, source38F0AA, configuration or startup45.
class SourceRuntimeSoundCodecEventPort final {
public:
    struct State {
        std::uint64_t generation{},identity{},delivered{},cancelled{};
        std::uint32_t cookie{},controller{};
        int event_descriptor=-1,shutdown_descriptor=-1,dispatcher_descriptor=-1;
        bool initialized{},connected{},shutdown_registered{},pending{},retired{},faulted{},consumer{};
    };
    // Bank, process-lifetime registry and creating executor outlive this owner.
    SourceRuntimeSoundCodecEventPort(SourceRuntimeMemory&,std::shared_ptr<SdlSoundPlatform>,
        SourceRuntimeSoundResetResources::ControlLease,std::shared_ptr<SourceRuntimeSoundCodecRoute>,
        std::shared_ptr<SourceRuntimeSoundPcmBuffers>,SourceProcessShutdownRegistry&);
    ~SourceRuntimeSoundCodecEventPort();
    SourceRuntimeSoundCodecEventPort(const SourceRuntimeSoundCodecEventPort&)=delete;
    bool try_call(SourcePhysicsContactMemory&,const SourcePhysicsContactRuntimeCall&,
        SourcePhysicsWorldRuntimeAbi&,const SourcePhysicsContactRecordObserver& = {});
    // Explicit native component service of the authenticated default analog
    // descriptor bytes. This is not source output-start or DSP admission.
    void consume_default_analog();
    bool collect_success();
    bool dispatch(SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
        const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
    // Actual native lifecycle request and original registered callback delivery.
    void request_shutdown();
    bool dispatch_shutdown(SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
        const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
    // Preserve this endpoint owner to account every real receipt after native
    // cancellation/fault. Teardown never acknowledges packets for its caller.
    std::shared_ptr<SourceRuntimeSoundAc97ChannelReset::EndpointLease> receipt_owner()const;
    State state()const;
    void retire();
    class CallScope final {
    public:
        explicit CallScope(SourceRuntimeSoundCodecEventPort*);
        ~CallScope();
    private:
        SourceRuntimeSoundCodecEventPort* owner_;
        int exceptions_;
    };
private:
    friend class SourceRuntimeSoundColdAnalogStart;
    void require_start_admission(SourceRuntimeMemory&,const SourceRuntimeSoundResetResources::ControlLease&,
        const SourceRuntimeSoundCodecRoute&,const SourceRuntimeSoundPcmBuffers&,const SourceRuntimeSoundAc97ChannelReset&)const;
    void adopt_source_start(std::shared_ptr<SourceRuntimeSoundAc97ChannelReset::ConsumerLease>);
    void cancel_source_start();
    struct Impl;
    std::unique_ptr<Impl> impl_;
    void abort()noexcept;
};
void mount_native_tu15_sound_codec_event_data(SourceRuntimeMemory&);
}
