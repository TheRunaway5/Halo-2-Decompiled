#pragma once
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_required_owner_policy.hpp"
#include <array>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <utility>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_mopp_ray_controller.hpp"
#include "physics/runtime/native_tu15_mopp_ray_slab_frame.hpp"
#include "physics/runtime/native_tu15_mopp_ray_diagnostic_frame.hpp"
#endif

namespace halo2::native_mopp_ray_traversal_detail {
struct Noop {
    static constexpr bool silent_hook = true;
    template<class State> void operator()(State &, std::uint32_t) const noexcept {}
};
inline bool reference(){
    static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_MOPP_RAY_TRAVERSAL");return p&&*p;}();return enabled;
}
// Both the private traversal and the entry tranche use the exact cursor and
// floating-point admission. Retained tails keep their original child owners.
template<class State>bool initial_state_admitted(const State&s){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(reference()||s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    const auto&c=s.services.instruction_cursor;
    if(s.services.instruction){
        if(s.instruction_cursor!=&c||!s.literal_service_sites||!c.position||!c.callback_type||
           s.services.instruction.target_type()!=*c.callback_type||c.executor!=std::this_thread::get_id()||
           !c.literal_service_sites||c.service_sites.size()!=3||c.service_sites[0]!=0x38846eu||
           c.service_sites[1]!=0x21f590u||c.service_sites[2]!=0x33b595u||
           (c.first_trace&&*c.first_trace)||(c.second_trace&&*c.second_trace)||
           (s.observe&&!c.observation_only_stores))return false;
    }else if(s.instruction_cursor||s.literal_service_sites||c.position||c.literal_service_sites||s.observe)return false;
    // The complete original CFG reaches three temporary x87 values. Every
    // recursive/external call is balanced at zero temporaries. Capacity for
    // all authored branches therefore requires at most five incoming values.
    return s.fp.count<=5u&&native_raw_x87_detail::supported(s);
#else
    (void)s;return false;
#endif
}
template<class State>bool admitted(const State&s){
    if(!initial_state_admitted(s))return false;
    // A real child can rebind policy while this continuation is parked. This
    // explicit owned callback/capture contract is independent of the cursor
    // and store-observer capability; arbitrary providers remain refused.
    return native_required_owner_policy_detail::preserves_instruction_policy(s.services);
}
// The entry tranche ends before a retained branch or diagnostic instruction.
enum class EntryContinuation : std::uint8_t { retained, classify, diagnostic };
enum class EntryOwner : std::uint8_t { death, ai };

struct EntryRouteCounts {
    const char *owner;
    std::array<std::atomic<std::uint64_t>,3> calls{};
    ~EntryRouteCounts() {
        std::fprintf(stderr,"MOPP ray entry %s: retained=%llu classify=%llu diagnostic=%llu\n",owner,
            static_cast<unsigned long long>(calls[0].load(std::memory_order_relaxed)),
            static_cast<unsigned long long>(calls[1].load(std::memory_order_relaxed)),
            static_cast<unsigned long long>(calls[2].load(std::memory_order_relaxed)));
    }
};
template<EntryOwner Owner> EntryContinuation count_entry_route(EntryContinuation route) noexcept {
    // A route witness for live qualification. Never replay a guest store.
    static const bool report=[] {
        const auto *value=std::getenv("HALO2_REPORT_MOPP_RAY_ENTRY");
        return value && *value;
    }();
    if(report) {
        static EntryRouteCounts counts{Owner==EntryOwner::death ? "death" : "ai"};
        counts.calls[static_cast<unsigned>(route)].fetch_add(1,std::memory_order_relaxed);
    }
    return route;
}

template<EntryOwner Owner,class State> EntryContinuation entry_prefix(State &s) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(!admitted(s))return count_entry_route<Owner>(EntryContinuation::retained);
    Noop hook;
    native_mopp_ray_caller_detail::HostedMemory before{hook};
    const auto route=native_mopp_ray_caller_detail::with_deferred_hosted_frame(s,before,
        [&](auto &frame,auto) {
            if(frame.begin().valid_program_class)return EntryContinuation::classify;
            // 30AA94 is the last retired instruction on the invalid edge.
            if(s.instruction_cursor)*s.instruction_cursor->position=0x30aa94u;
            return EntryContinuation::diagnostic;
        });
    return count_entry_route<Owner>(route);
#else
    (void)s;
    return count_entry_route<Owner>(EntryContinuation::retained);
#endif
}

template<class State,class Dispatch,class Before=Noop>
bool bridge(State&s,Dispatch dispatch,Before before={}){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(!admitted(s))return false;
    auto slab=[](auto&state,auto&observe,auto&scope,auto opcode,auto&program){return native_mopp_ray_caller_detail::slab(state,observe,scope,opcode,program);};
    auto diagnostic=[](auto&state,auto&observe,auto&scope,auto&child){native_mopp_ray_caller_detail::DiagnosticFrame view{state,observe,scope,child};view.invoke();};
    native_mopp_ray_caller_detail::traverse(s,before,dispatch,slab,diagnostic);return true;
#else
    (void)s;(void)dispatch;(void)before;return false;
#endif
}

// Private equivalent of the retained caller's memory and observer interface.
template<class State, class Dispatch, class Before = Noop>
bool bridge_hosted(State &state, Dispatch dispatch, Before before = {}) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    return bridge(state, std::move(dispatch), native_mopp_ray_caller_detail::HostedMemory{before});
#else
    (void)state; (void)dispatch; (void)before; return false;
#endif
}
}
