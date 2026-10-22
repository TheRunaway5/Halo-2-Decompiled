#pragma once
#include "halo2/source_runtime_sound_event_port.hpp"
#include <functional>

namespace halo2 {
// Owns timed base-voice notifications through the same real EventPort timer
// and frontend deferred queue. Bank, frontend and EventPort outlive this owner.
// The admitted virtual callback is the authentic inactive index4 path only;
// playing voice/PIO callbacks and full audio configuration remain required.
class SourceRuntimeSoundTimedNotifications final {
public:
    struct PlatformObservation {
        std::uint32_t instruction{},target{};
        std::uint64_t system_time{};
        bool has_boolean_result{},boolean_result{};
    };
    using PlatformObserver=std::function<void(PlatformObservation)>;
    struct State {
        std::uint64_t generation{},due_time{},expirations{},completed{},cancelled{};
        NativeRuntimeAddress native_owner{};
        int timer_descriptor=-1;
        bool dpc_initialized{},armed{},retired{},faulted{};
    };
    SourceRuntimeSoundTimedNotifications(SourceRuntimeMemory&,SourceRuntimeSoundResetResources&,
        SourceRuntimeSoundFrontendEvents&,SourceRuntimeSoundEventPort&);
    // Destruction must use the creating thread, including after retirement.
    // Foreign destruction terminates before mutable state/resource access.
    ~SourceRuntimeSoundTimedNotifications();
    SourceRuntimeSoundTimedNotifications(const SourceRuntimeSoundTimedNotifications&)=delete;
    bool try_call(SourcePhysicsContactMemory&,const SourcePhysicsContactRuntimeCall&,
        SourcePhysicsWorldRuntimeAbi&,const SourcePhysicsContactRecordObserver& = {});
    // Execute whole original38CBB9 or38CBE8 against an owned index4 voice.
    void source_notification(std::uint32_t entry,SourcePhysicsWorldRuntimeAbi&,
        SourcePhysicsFactoryFloatingPoint,const SourcePhysicsWorldRuntimeServices&,
        const SourcePhysicsContactRecordObserver& = {},const PlatformObserver& = {});
    // Nonblocking real native timer expiration consumption. This enqueues its
    // actual registered DPC in the existing frontend queue; it does not run it.
    bool collect_due();
    bool dispatch_dpc(SourcePhysicsWorldRuntimeAbi&,SourcePhysicsFactoryFloatingPoint,
        const SourcePhysicsWorldRuntimeServices&,const SourcePhysicsContactRecordObserver& = {},const PlatformObserver& = {});
    State state()const;
    void retire_generation();
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
void mount_native_tu15_sound_timed_notification_data(SourceRuntimeMemory&);
}
