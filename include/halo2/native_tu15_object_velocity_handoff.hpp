#pragma once
#include "halo2/physics_contact_runtime.hpp"
#include <functional>
#include <thread>
namespace halo2 {
struct NativeTu15ObjectVelocityHandoffActivity {
    std::uint64_t attempts{},completed{},refusals{};
};
using NativeTu15ObjectVelocityHandoff=std::function<void(SourcePhysicsContactMemory&,
    const SourcePhysicsContactRuntimeCall&,SourcePhysicsContactRuntimeAbi&,
    const SourcePhysicsContactRecordObserver&)>;
// Explicit adapter ownership for the one authenticated F2091->B9800 void call.
// The registry is tied to a memory owner and executor, has no thread-local
// state, and retains no GP/SIMD data, CPU results or callback postimages.
class NativeTu15ObjectVelocityHandoffBinding {
    SourcePhysicsContactMemory*memory_;
    NativeTu15ObjectVelocityHandoff callback_;
    NativeTu15ObjectVelocityHandoffActivity*activity_;
    std::thread::id executor_;
    friend bool native_tu15_handoff_object_velocity(SourcePhysicsContactMemory&,
        const SourcePhysicsContactRuntimeCall&,SourcePhysicsContactRuntimeAbi&,
        const SourcePhysicsContactRecordObserver&);
public:
    NativeTu15ObjectVelocityHandoffBinding(SourcePhysicsContactMemory&,
        NativeTu15ObjectVelocityHandoff,NativeTu15ObjectVelocityHandoffActivity* = nullptr);
    ~NativeTu15ObjectVelocityHandoffBinding();
    NativeTu15ObjectVelocityHandoffBinding(const NativeTu15ObjectVelocityHandoffBinding&)=delete;
    NativeTu15ObjectVelocityHandoffBinding&operator=(const NativeTu15ObjectVelocityHandoffBinding&)=delete;
};
// False means no explicit binding exists and the legacy owner remains selected.
// An active binding must either finish the genuine native owner or fail; an
// unsupported descriptor, failed callback or wrong RET4 never falls back.
bool native_tu15_handoff_object_velocity(SourcePhysicsContactMemory&,
    const SourcePhysicsContactRuntimeCall&,SourcePhysicsContactRuntimeAbi&,
    const SourcePhysicsContactRecordObserver& = {});
} // namespace halo2
