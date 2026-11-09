#pragma once
#include "halo2/native_tu15_support_carrier.hpp"
#include "halo2/physics_contact_runtime.hpp"
namespace halo2 {
struct NativeTu15SupportCarrierHandoffPolicy {
    std::uint32_t stack_first{},stack_end{},data_first{},data_bytes{};
    bool observers_read_only{};
    const bool*first_trace_active{};
    const bool*second_trace_active{};
    std::function<void(std::uint32_t)> instruction;
};
struct NativeTu15SupportCarrierHandoffActivity {
    std::uint64_t attempts{},completed{},refusals{};
    NativeTu15SupportCarrierStats carriers;
};
// Sole actual1FA18D ->184EC0, RET24. The caller consumes only AL and DATA.
bool native_tu15_handoff_support_carrier(SourcePhysicsContactMemory&,
    const SourcePhysicsContactRuntimeCall&,SourcePhysicsContactRuntimeAbi&,
    NativeTu15SupportCarrierHandoffPolicy,const SourcePhysicsContactRecordObserver& = {},
    NativeTu15SupportCarrierHandoffActivity* = nullptr);
}
