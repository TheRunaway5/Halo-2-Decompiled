#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <functional>
#include <map>
namespace halo2 {
struct NativeTu15ComponentStateServices {
    std::function<void(std::uint32_t,NativeTu15PhysicsFrameFp&,unsigned)> remove; //1D9510, component DATA
    std::function<void(std::uint32_t,NativeTu15PhysicsFrameFp&,unsigned)> reset; //1D8470, component DATA
    std::function<void(std::uint32_t,NativeTu15PhysicsFrameFp&,unsigned)> refresh; //1D73D0, full datum
    std::function<void(std::uint32_t,NativeTu15PhysicsFrameFp&,unsigned)> activate; //1D97F0, component DATA
};
struct NativeTu15ComponentStateStats {
    std::uint64_t flag_calls{},defer_calls{},removals{},resets{},refreshes{},activations{},unbound_services{};
    std::map<std::uint32_t,std::uint64_t> unbound_owners;
};
// Actual1CC310. Preserve the saved source flag-bit, current object-datum
// reread after reset, full32-bit flag store and subsequent owner order.
void native_tu15_set_component_state_flags(SourcePhysicsContactMemory&,
    std::uint32_t datum,std::uint32_t flags,NativeTu15PhysicsFrameFp&,unsigned,
    const NativeTu15ComponentStateServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15ComponentStateStats* = nullptr);
// ActualBA8C0. Its object identity remains cached across optional1D97F0;
// the final current object+C0 byte is read again before its OR publication.
void native_tu15_defer_component_object(SourcePhysicsContactMemory&,
    std::uint32_t datum,NativeTu15PhysicsFrameFp&,unsigned,
    const NativeTu15ComponentStateServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15ComponentStateStats* = nullptr);
} // namespace halo2
