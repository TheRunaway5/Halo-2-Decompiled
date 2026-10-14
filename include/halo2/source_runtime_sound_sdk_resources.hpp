#pragma once
#include "halo2/physics_world_runtime.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/source_tu15_settings.hpp"
#include <memory>
namespace halo2 {
// Real native resources for the reached SDK factory. Pool capacity follows
// the recovered reference allocation rules. Address selection and
// native lock ABI are explicit platform policies, without Xbox kernel
// record/clobber emulation. No settings, hardware or device is supplied.
class SourceRuntimeSoundSdkResources {
public:
    SourceRuntimeSoundSdkResources(SourceRuntimeMemory &,std::uint32_t passive_fs_base);
    ~SourceRuntimeSoundSdkResources();
    SourceRuntimeSoundSdkResources(const SourceRuntimeSoundSdkResources &)=delete;
    SourceRuntimeSoundSdkResources &operator=(const SourceRuntimeSoundSdkResources &)=delete;
    bool dispatch(SourcePhysicsContactMemory &,const SourcePhysicsContactRuntimeCall &,SourcePhysicsWorldRuntimeAbi &,
        const SourcePhysicsContactRecordObserver & = {});
    // Explicit actual settings resource; absent input remains unavailable.
    // Immutable user settings stay local and do not initialize any source device.
    void bind_user_settings(SourceTu15UserSettings);
    std::size_t live_allocations() const noexcept;
    unsigned lock_depth() const noexcept;
    bool quiescent() const noexcept;
    void require_quiescent() const;
    class CallScope {
    public:
        explicit CallScope(SourceRuntimeSoundSdkResources *);
        ~CallScope();
        CallScope(const CallScope &)=delete;
        CallScope &operator=(const CallScope &)=delete;
    private:
        SourceRuntimeSoundSdkResources *owner_;
        std::uint32_t cursor_;
        unsigned locks_;
        int exceptions_;
    };
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    void abort_new_resources(std::uint32_t,unsigned) noexcept;
};
}
