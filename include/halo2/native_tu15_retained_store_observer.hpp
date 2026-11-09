#pragma once
#include "halo2/native_tu15_retained_store_observation.hpp"
#include <memory>
#include <utility>
#include <stdexcept>
namespace halo2 {
// Explicit retained-DATA contract: the caller does not request raw local
// preimages/events. The callback is synchronous and must obey its caller's
// read-only/non-reentrant observer policy. It does not authorize DATA reads.
class NativeTu15RetainedStoreObserver {
    SourcePhysicsContactRecordObserver protected_callback_;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    struct CallbackState {
        SourcePhysicsContactRecordObserver callback;
        alignas(16) std::array<std::byte,108> host_x87;
        std::uint32_t host_mxcsr;
        explicit CallbackState(SourcePhysicsContactRecordObserver value):callback(std::move(value)) {
            asm volatile("stmxcsr %0":"=m"(host_mxcsr));
            asm volatile("fnsave %0;frstor %0":"=m"(host_x87)::"memory",
                "st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        }
    };
    class CallbackFpScope {
        alignas(16) std::array<std::byte,108> native_x87_;
        std::uint32_t native_mxcsr_;
    public:
        explicit CallbackFpScope(const CallbackState&state) {
            asm volatile("stmxcsr %0":"=m"(native_mxcsr_));
            asm volatile("fnsave %0;frstor %1":"=m"(native_x87_):"m"(state.host_x87):"memory",
                "st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
            asm volatile("ldmxcsr %0"::"m"(state.host_mxcsr):"memory");
        }
        ~CallbackFpScope() {
            // Discard callback pending exceptions before the waiting FRSTOR.
            asm volatile("fninit;frstor %0"::"m"(native_x87_):"memory",
                "st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
            asm volatile("ldmxcsr %0"::"m"(native_mxcsr_):"memory");
        }
        CallbackFpScope(const CallbackFpScope&)=delete;
        CallbackFpScope& operator=(const CallbackFpScope&)=delete;
    };
#endif
public:
    NativeTu15RetainedStoreObserver()=default;
    explicit NativeTu15RetainedStoreObserver(SourcePhysicsContactRecordObserver value) {
        if(!value)return;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
        auto state=std::make_shared<CallbackState>(std::move(value));
        protected_callback_=[state](const SourcePhysicsContactRecordStore&event) {
            CallbackFpScope restore(*state);state->callback(event);
        };
#else
        throw std::runtime_error("Native retained observer requires qualified hardware FP preservation");
#endif
    }
    const SourcePhysicsContactRecordObserver& callback()const noexcept {
        return protected_callback_;
    }
};
class NativeTu15RetainedStoreObserverProvider {
public:
    virtual ~NativeTu15RetainedStoreObserverProvider()=default;
    virtual const SourcePhysicsContactRecordObserver& retained_store_observer()const noexcept=0;
};
// Reference adapters keep their existing explicit reference semantics. The
// provider callback has one shared target and outlives every synchronous writer.
inline const SourcePhysicsContactRecordObserver& native_tu15_effective_store_observer(
    const SourcePhysicsContactMemory& memory,const SourcePhysicsContactRecordObserver& explicit_observer){
    if(explicit_observer)return explicit_observer;
    if(const auto*provider=dynamic_cast<const NativeTu15RetainedStoreObserverProvider*>(&memory))
        return provider->retained_store_observer();
    return explicit_observer;
}
// Existing value adapters preserve their explicit-observer copy semantics.
// Implicit resolution forwards to one stable target, including mutable functors.
inline SourcePhysicsContactRecordObserver native_tu15_copy_effective_store_observer(
    const SourcePhysicsContactMemory&memory,const SourcePhysicsContactRecordObserver&explicit_observer){
    if(explicit_observer)return explicit_observer;
    const auto& callback=native_tu15_effective_store_observer(memory,explicit_observer);
    if(!callback)return {};
    return [target=&callback](const SourcePhysicsContactRecordStore&e){(*target)(e);};
}
} // namespace halo2
