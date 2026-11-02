#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <functional>
namespace halo2 {
struct NativeTu15ContactPressureCandidate {
    std::uint32_t body{},mode{},excluded{0xffffffffu};
    bool first_option{},second_option{};
};
struct NativeTu15ContactPressureServices {
    std::function<std::uint32_t(std::uint32_t shape,std::uint32_t owner,
        NativeTu15PhysicsFrameFp&)> classify_shape;
    std::function<std::uint32_t(NativeTu15ContactPressureCandidate,
        NativeTu15PhysicsFrameFp&)> select_candidate; //1CCAD0
    std::function<void(std::uint32_t system,std::uint32_t effect,
        std::uint32_t position,std::uint32_t direction,
        NativeTu15PhysicsFrameFp&)> emit_effect; //179470, remaining args0,1
    std::function<void(std::uint32_t,NativeTu15PhysicsFrameFp&)> remove_component; //1D9510
    std::function<void(std::uint32_t,NativeTu15PhysicsFrameFp&)> destroy_component; //1D7CA0
    std::function<void(std::uint32_t pool,std::uint32_t datum,
        NativeTu15PhysicsFrameFp&)> free_component; //16E8A0
    std::function<void(std::uint32_t datum,NativeTu15PhysicsFrameFp&)> delete_object; //BA570
    std::function<void(NativeTu15PhysicsFrameFp&)> collect_memory; //1496F0
    std::function<void(NativeTu15PhysicsFrameFp&)> update_contacts; //282C10
};
struct NativeTu15ContactPressureStats {
    std::uint64_t calls{},null_worlds{},reentrant_skips{},islands{},dense_islands{},
        shape_queries{},candidate_queries{},effect_calls{},component_removals{},
        component_destructions{},component_frees{},object_deletions{},unbound_services{};
};
// Actual1CCE00: pressure-driven object pruning, including live World/array
// reloads and source guard/store phases. Remaining children retain typed
// ownership; no private CPU/register image or old-parent fallback is used.
void native_tu15_prune_contact_pressure(SourcePhysicsContactMemory&,
    NativeTu15PhysicsFrameFp&,const NativeTu15ContactPressureServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15ContactPressureStats* = nullptr);
}
