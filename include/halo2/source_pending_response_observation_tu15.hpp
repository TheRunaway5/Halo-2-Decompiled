#pragma once
#include "halo2/native_tu15_pending_response.hpp"
#include "halo2/physics_contact_record.hpp"
#include <functional>
#include <utility>

namespace halo2 {
struct SourcePendingResponseNativeObservationTu15 {
    std::uint32_t address{};
    std::uint16_t before{},after{};
    std::uint8_t response{};
    NativeTu15PendingResponseStatus status{};
};
namespace source_pending_response_observation_detail {
struct Binding {
    const SourcePhysicsContactMemory* memory{};
    std::function<void(const SourcePendingResponseNativeObservationTu15&)> observe;
};
inline thread_local Binding* active{};
}
// Synchronous, bank-bound diagnostics after the native step's actual store
// (or inactive decision). No result can be returned to the kernel. An unrelated
// world on the same thread cannot publish into this journal; nested scopes
// restore the previous binding. The caller owns the bank and executor lifetime.
class SourcePendingResponseObservationScopeTu15 {
    source_pending_response_observation_detail::Binding binding_;
    source_pending_response_observation_detail::Binding* previous_;
public:
    SourcePendingResponseObservationScopeTu15(const SourcePhysicsContactMemory* memory,
        std::function<void(const SourcePendingResponseNativeObservationTu15&)> observe)
        :binding_{memory,std::move(observe)},previous_(source_pending_response_observation_detail::active) {
        source_pending_response_observation_detail::active=memory?&binding_:nullptr;
    }
    ~SourcePendingResponseObservationScopeTu15(){source_pending_response_observation_detail::active=previous_;}
    SourcePendingResponseObservationScopeTu15(const SourcePendingResponseObservationScopeTu15&)=delete;
    SourcePendingResponseObservationScopeTu15& operator=(const SourcePendingResponseObservationScopeTu15&)=delete;
};
inline void source_observe_pending_response_native_tu15(const SourcePhysicsContactMemory& memory,
    std::uint32_t address,std::uint16_t before,const NativeTu15PendingResponseStep& step) {
    const auto* binding=source_pending_response_observation_detail::active;
    if(binding && binding->memory==&memory && binding->observe)
        binding->observe({address,before,step.word,step.response,step.status});
}
}
