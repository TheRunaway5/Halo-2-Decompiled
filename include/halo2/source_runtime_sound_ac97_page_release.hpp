#pragma once
#include "halo2/source_runtime_sound_ac97_channel_reset.hpp"

namespace halo2 {
// Authentic initialized38F395 teardown. Original CPU owns channel cleanup,
// physical statistics and shared-pool return. Native admission requires
// retired PCM bindings and completed endpoint reset before actual page free.
class SourceRuntimeSoundAc97PageRelease final {
public:
    struct State {
        std::vector<SourceRuntimeSoundAc97OutputPages::Allocation> released;
        bool faulted{},source_started{},producer_ready{};
    };
    SourceRuntimeSoundAc97PageRelease(SourceRuntimeMemory&,
        std::shared_ptr<SourceRuntimeSoundAc97OutputPages>,
        std::shared_ptr<SourceRuntimeSoundAc97ChannelReset>,
        SourceRuntimeSoundResetResources::ControlLease);
    void run_retirement(SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
        const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {});
    State diagnostics()const;
private:
    SourceRuntimeMemory& memory_;
    const std::thread::id thread_=std::this_thread::get_id();
    std::shared_ptr<SourceRuntimeSoundAc97OutputPages> pages_;
    std::shared_ptr<SourceRuntimeSoundAc97ChannelReset> channels_;
    SourceRuntimeSoundResetResources::ControlLease control_;
    const std::uint64_t generation_;
    std::optional<SourceRuntimeSoundAc97OutputPages::Allocation> pending_;
    std::vector<SourceRuntimeSoundAc97OutputPages::Allocation> released_;
    std::uint32_t caller_sp_{},parent_{},statistics_{};
    bool running_{},fault_{};
    void require_thread()const;
    void require_live()const;
    bool dispatch(SourcePhysicsContactMemory&,const SourcePhysicsContactRuntimeCall&,SourcePhysicsWorldRuntimeAbi&);
};
void mount_native_tu15_sound_ac97_page_release_data(SourceRuntimeMemory&);
}
