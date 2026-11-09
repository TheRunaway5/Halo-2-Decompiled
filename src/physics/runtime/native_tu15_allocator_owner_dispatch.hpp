#pragma once
#include <algorithm>
#include <map>
#include "physics/runtime/native_tu15_raw_allocator_abi.hpp"
#include "physics/runtime/native_tu15_pool_allocator_abi.hpp"
#include "physics/runtime/native_tu15_main_heap_allocate_abi.hpp"
#include "platform/native_tu15_tick_conversion.hpp"
#include "physics/runtime/native_tu15_physical_epoch_abi.hpp"
#include "physics/runtime/native_tu15_bsp_surface_2d.hpp"
#include "physics/runtime/native_tu15_bsp_surface_polygon.hpp"
#include "physics/runtime/native_tu15_datum_delete_abi.hpp"
#include "physics/runtime/native_tu15_stack_probe_abi.hpp"
#include "physics/runtime/native_tu15_resource_busy_abi.hpp"
#include "physics/runtime/native_tu15_object_prediction_copy.hpp"

namespace halo2::native_allocator_owner_detail {
// These actual TU1.5 entries have no run_* body in the live-support owner.
// Keep the complete inventory visible to native-owner accounting. Pool and
// main-heap entries reuse their independently qualified shared ABI kernels.
inline constexpr std::array<std::uint32_t,10> owner_entries{0x232aa0u,0x232ac0u,0x2e1030u,
    0x233250u,0x233420u,0x232ad0u,0x232c80u,0x232d00u,0x232d20u,0x232d30u};
inline constexpr std::array<std::uint32_t,3> tick_conversion_entries{0x22a5c0u,0x32ae70u,0x32acd0u};
inline constexpr std::array<std::uint32_t,3> physical_epoch_entries{0x12f040u,0x12f0c0u,0x13fd10u};
inline constexpr std::array<std::uint32_t,1> bsp_surface_2d_entries{0x2a0230u};
inline constexpr std::array<std::uint32_t,1> bsp_polygon_entries{0x1e6e80u};
inline constexpr std::array<std::uint32_t,1> datum_delete_entries{0x16e8a0u};
inline constexpr std::array<std::uint32_t,1> stack_probe_entries{0x32a660u};
inline constexpr std::array<std::uint32_t,2> resource_busy_entries{0x12fcb0u,0x4057f0u};
inline constexpr std::array<std::uint32_t,1> prediction_copy_entries{0xadcd0u};
struct Activity {std::map<std::uint32_t,std::uint64_t> entries,tick_entries,epoch_entries,bsp_entries,bsp_polygon_entries,delete_entries,probe_entries,resource_entries,prediction_entries;std::uint64_t crt_import_tails{};};
inline Activity &activity(){thread_local Activity value;return value;}
template<class State,class Call> bool dispatch(State &s,std::uint32_t target,Call call) {
    // Imported CPU leaves share the authenticated actual-owner fallback, but
    // retain their own admission and reference controls, independent of heaps.
    if(std::find(prediction_copy_entries.begin(),prediction_copy_entries.end(),target)!=prediction_copy_entries.end()) {
        try {
            const bool accepted=native_object_prediction_copy_detail::bridge(s);
            if(accepted)++activity().prediction_entries[target];return accepted;
        }catch(...){++activity().prediction_entries[target];throw;}
    }
    if(std::find(resource_busy_entries.begin(),resource_busy_entries.end(),target)!=resource_busy_entries.end()) {
        try {
            const bool accepted=target==0x4057f0u?native_resource_busy_detail::query(s):native_resource_busy_detail::callback(s,call);
            if(accepted)++activity().resource_entries[target];return accepted;
        }catch(...){++activity().resource_entries[target];throw;}
    }
    if(std::find(stack_probe_entries.begin(),stack_probe_entries.end(),target)!=stack_probe_entries.end()) {
        try {
            const bool accepted=native_stack_probe_detail::run(s);
            if(accepted)++activity().probe_entries[target];return accepted;
        }catch(...){++activity().probe_entries[target];throw;}
    }
    if(std::find(bsp_polygon_entries.begin(),bsp_polygon_entries.end(),target)!=bsp_polygon_entries.end()) {
        try {
            const bool accepted=native_bsp_surface_polygon_detail::bridge(s);
            if(accepted)++activity().bsp_polygon_entries[target];return accepted;
        }catch(...){++activity().bsp_polygon_entries[target];throw;}
    }
    if(std::find(datum_delete_entries.begin(),datum_delete_entries.end(),target)!=datum_delete_entries.end()) {
        try {
            const bool accepted=native_datum_delete_detail::run(s);
            if(accepted)++activity().delete_entries[target];return accepted;
        }catch(...){++activity().delete_entries[target];throw;}
    }
    if(std::find(bsp_surface_2d_entries.begin(),bsp_surface_2d_entries.end(),target)!=bsp_surface_2d_entries.end()) {
        try {
            const bool accepted=native_bsp_surface_2d_detail::bridge(s);
            if(accepted)++activity().bsp_entries[target];return accepted;
        }catch(...){++activity().bsp_entries[target];throw;}
    }
    if(std::find(tick_conversion_entries.begin(),tick_conversion_entries.end(),target)!=tick_conversion_entries.end()) {
        if(!native_partition_temporary_detail::admitted(s))return false;
        try {
            const bool accepted=native_tick_conversion_detail::run(s,target);
            if(accepted)++activity().tick_entries[target];return accepted;
        }catch(...){++activity().tick_entries[target];throw;}
    }
    if(std::find(physical_epoch_entries.begin(),physical_epoch_entries.end(),target)!=physical_epoch_entries.end()) {
        if(!native_partition_temporary_detail::admitted(s))return false;
        try {
            const bool accepted=native_physical_epoch_detail::run(s,target,call);
            if(accepted)++activity().epoch_entries[target];return accepted;
        }catch(...){++activity().epoch_entries[target];throw;}
    }
    if(native_raw_allocator_detail::reference_mode()||!native_partition_temporary_detail::admitted(s))return false;
    if(std::find(owner_entries.begin(),owner_entries.end(),target)==owner_entries.end())return false;
    ++activity().entries[target];auto &g=s.gp;
    const auto at=[&](std::uint32_t pc){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;};
    const auto child=[&](auto &v,std::uint32_t owner,std::uint32_t site){call(v,owner,site,false);};
    switch(target) {
    case 0x233250u:return native_pool_allocator_detail::allocate(s);
    case 0x233420u:return native_pool_allocator_detail::release(s,child);
    case 0x232ad0u:return native_main_heap_allocation_detail::allocate(s,child);
    case 0x232c80u:return native_main_heap_detail::remove_allocation(s);
    case 0x232d00u:return native_main_heap_allocation_detail::allocate_sized(s,child);
    case 0x232d20u:return native_main_heap_detail::release(s,child);
    case 0x232d30u:return native_main_heap_allocation_detail::probe_gap(s);
    case 0x232aa0u:{
        at(0x232aa0u);g[2]=physics_step_detail::r(s.memory,g[4]+8u,4);
        at(0x232aa4u);g[0]=physics_step_detail::r(s.memory,g[1],4);
        s.push(0x232aa6u,g[2]);at(0x232aa7u);g[2]=physics_step_detail::r(s.memory,g[4]+8u,4);
        s.push(0x232aabu,g[2]);s.push(0x232aacu,1u);
        at(0x232aaeu);const auto owner=physics_step_detail::r(s.memory,g[0]+8u,4);
        s.push(0x232aaeu,0x232ab1u);call(s,owner,0x232aaeu,false);
        s.pop();g[4]+=8u;at(0x232ab1u);return true;
    }
    case 0x232ac0u:{
        at(0x232ac0u);g[0]=physics_step_detail::r(s.memory,g[1],4);
        at(0x232ac2u);const auto owner=physics_step_detail::r(s.memory,g[0]+12u,4);
        call(s,owner,0x232ac2u,true);return true;
    }
    case 0x2e1030u:
        // Original JMP1CB560 enters the CRT import. It neither supplies an
        // allocator result nor translates that import to a host free.
        ++activity().crt_import_tails;at(0x2e1030u);call(s,0x1cb560u,0x2e1030u,true);return true;
    default:return false;
    }
}
} // namespace halo2::native_allocator_owner_detail
