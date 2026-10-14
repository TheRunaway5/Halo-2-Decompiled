#pragma once
#include "halo2/physics_world_runtime.hpp"
#include <exception>
#include <map>

namespace halo2 {
// Optional host diagnostics for calls retained inside one native source
// module. A static original child call does not cross required_owner.
// Registration is tied to its actual memory owner and executor thread.
using SourceInternalPhantomObserver=std::function<void(bool,const SourcePhysicsWorldRuntimeAbi &)>;
inline thread_local std::map<const SourcePhysicsContactMemory *,SourceInternalPhantomObserver>
    source_internal_phantom_observers;
inline void source_bind_internal_phantom_observer(const SourcePhysicsContactMemory &memory,
                                                  SourceInternalPhantomObserver observer) {
    if(observer)source_internal_phantom_observers.insert_or_assign(&memory,std::move(observer));
    else source_internal_phantom_observers.erase(&memory);
}
class SourceInternalPhantomObservation {
    SourceInternalPhantomObserver observer_;
    std::function<SourcePhysicsWorldRuntimeAbi()> state_;
    int exceptions_=std::uncaught_exceptions();
public:
    SourceInternalPhantomObservation(const SourcePhysicsContactMemory &memory,
                                    std::function<SourcePhysicsWorldRuntimeAbi()> state) {
        const auto found=source_internal_phantom_observers.find(&memory);
        if(found==source_internal_phantom_observers.end())return;
        observer_=found->second;state_=std::move(state);observer_(true,state_());
    }
    ~SourceInternalPhantomObservation() noexcept(false) {
        if(observer_ && exceptions_==std::uncaught_exceptions())observer_(false,state_());
    }
    SourceInternalPhantomObservation(const SourceInternalPhantomObservation &)=delete;
};
}
