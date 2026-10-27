#pragma once
#include "halo2/native_tu15_physics_frame_residuals.hpp"
#include <exception>
#include <map>
#include <stdexcept>

namespace halo2 {
// Offline diagnostics of the semantic frame boundary. The entering call state
// is the genuine standalone host call. At return only FP policy/status and
// live x87 values are outputs; the last residual's GPRs are not root outputs.
struct NativeTu15PhysicsFrameObservationEvent {
    const SourcePhysicsContactRuntimeAbi *entry{};
    const NativeTu15PhysicsFrameFp &floating_point;
    const SourcePhysicsContactRuntimeAbi &residual_live_values;
};
using NativeTu15PhysicsFrameObserver=
    std::function<void(const NativeTu15PhysicsFrameObservationEvent &)>;
// Function-local TLS preserves one registry per thread across translation
// units without MinGW namespace-scope TLS initializer aliases.
inline std::map<const SourcePhysicsContactMemory *,NativeTu15PhysicsFrameObserver>& native_tu15_physics_frame_observers() {
    static thread_local std::map<const SourcePhysicsContactMemory *,NativeTu15PhysicsFrameObserver> value;
    return value;
}
class NativeTu15PhysicsFrameObserverBinding {
    const SourcePhysicsContactMemory &memory_;
public:
    NativeTu15PhysicsFrameObserverBinding(const SourcePhysicsContactMemory &memory,
        NativeTu15PhysicsFrameObserver observer):memory_(memory) {
        if(!native_tu15_physics_frame_observers().emplace(&memory,std::move(observer)).second)
            throw std::runtime_error("Native physics frame diagnostics already bound");
    }
    ~NativeTu15PhysicsFrameObserverBinding(){native_tu15_physics_frame_observers().erase(&memory_);}
    NativeTu15PhysicsFrameObserverBinding(const NativeTu15PhysicsFrameObserverBinding &)=delete;
};
class NativeTu15PhysicsFrameObservation {
    NativeTu15PhysicsFrameObserver observer_;
    const NativeTu15PhysicsFrameFp &fp_;
    const SourcePhysicsContactRuntimeAbi &state_;
    int exceptions_=std::uncaught_exceptions();
public:
    NativeTu15PhysicsFrameObservation(const SourcePhysicsContactMemory &memory,
        const NativeTu15PhysicsFrameFp &fp,const SourcePhysicsContactRuntimeAbi &state):fp_(fp),state_(state) {
        const auto found=native_tu15_physics_frame_observers().find(&memory);
        if(found==native_tu15_physics_frame_observers().end())return;
        observer_=found->second;observer_({&state_,fp_,state_});
    }
    ~NativeTu15PhysicsFrameObservation() noexcept(false) {
        if(observer_&&std::uncaught_exceptions()==exceptions_)observer_({nullptr,fp_,state_});
    }
    NativeTu15PhysicsFrameObservation(const NativeTu15PhysicsFrameObservation &)=delete;
};
} // namespace halo2
