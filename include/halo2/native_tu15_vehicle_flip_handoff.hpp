#pragma once
#include "halo2/physics_contact_runtime.hpp"
#include "halo2/native_tu15_physics_frame.hpp"
#include <thread>
namespace halo2 {
struct NativeTu15VehicleFlipHandoffPolicy {
    std::uint32_t caller_stack_begin{},caller_stack_end{};
    const bool*first_trace{};const bool*second_trace{};bool observation_only_stores{};
};
struct NativeTu15VehicleFlipHandoffActivity {std::uint64_t attempts{},completed{},refusals{};};
// Actual caller is read-only transport context for genuine owner adapters;
// no ABI image belongs to the semantic flip kernel or is retained here.
using NativeTu15VehicleFlipHandoff=std::function<bool(std::uint32_t object,
    std::uint32_t actual_entry_stack,const SourcePhysicsContactRuntimeAbi&actual_caller,
    NativeTu15PhysicsFrameFp&,unsigned preserved_x87_depth,
    const SourcePhysicsContactRecordObserver&)>;
class NativeTu15VehicleFlipHandoffBinding {
    const SourcePhysicsContactMemory*memory_;NativeTu15VehicleFlipHandoff callback_;
    NativeTu15VehicleFlipHandoffPolicy policy_;NativeTu15VehicleFlipHandoffActivity*activity_;
    std::thread::id executor_;
    friend bool native_tu15_handoff_vehicle_flip(SourcePhysicsContactMemory&,
        const SourcePhysicsContactRuntimeCall&,SourcePhysicsContactRuntimeAbi&,
        const SourcePhysicsContactRecordObserver&);
public:
    NativeTu15VehicleFlipHandoffBinding(const SourcePhysicsContactMemory&,
        NativeTu15VehicleFlipHandoff,NativeTu15VehicleFlipHandoffPolicy,
        NativeTu15VehicleFlipHandoffActivity* = nullptr);
    ~NativeTu15VehicleFlipHandoffBinding();
    NativeTu15VehicleFlipHandoffBinding(const NativeTu15VehicleFlipHandoffBinding&)=delete;
    NativeTu15VehicleFlipHandoffBinding&operator=(const NativeTu15VehicleFlipHandoffBinding&)=delete;
};
// One qualified actual F24D8->F1DD0 call. Only AL, RET0/SP+4 and genuine FP
// controls/status are published. An active unsupported context never falls back.
bool native_tu15_handoff_vehicle_flip(SourcePhysicsContactMemory&,
    const SourcePhysicsContactRuntimeCall&,SourcePhysicsContactRuntimeAbi&,
    const SourcePhysicsContactRecordObserver& = {});
} // namespace halo2
