#pragma once
#include "halo2/physics_game_arena_native.hpp"
#include "physics/runtime/native_tu15_arena_publication.hpp"
#include <cstdlib>
#include <thread>
#include <utility>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_arena_initialize_frame.hpp"
#include "physics/runtime/native_tu15_required_abi_fp.hpp"
#endif

namespace halo2::native_arena_initialize_owner_detail {
// Private paired-producer candidate. Only the actual synchronous owner may
// publish the borrowed const Services table used by this generated-runtime view.
struct Noop{template<class State>void operator()(State&,std::uint32_t)const{}};
inline bool reference(){static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_ARENA_INITIALIZE");return p&&*p;}();return enabled;}
inline std::uint64_t&activity(){thread_local std::uint64_t value{};return value;}
template<class State>bool admitted(const State&s){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(reference()||s.image!=SourcePhysicsContactImage::TitleUpdate15||
       !native_arena_publication_detail::Borrow::qualified(s.memory,s.services)||
       !native_raw_x87_detail::supported(s))return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    const auto&c=s.services.instruction_cursor;
    if(!s.services.instruction)return !s.instruction_cursor&&!s.literal_service_sites&&
        !c.position&&!c.literal_service_sites&&!s.observe;
    return s.instruction_cursor==&c&&s.literal_service_sites&&c.position&&c.callback_type&&
        s.services.instruction.target_type()==*c.callback_type&&
        c.executor==std::this_thread::get_id()&&c.literal_service_sites&&
        c.service_sites.size()==3&&c.service_sites[0]==0x38846eu&&
        c.service_sites[1]==0x21f590u&&c.service_sites[2]==0x33b595u&&
        !(c.first_trace&&*c.first_trace)&&!(c.second_trace&&*c.second_trace)&&
        (!s.observe||c.observation_only_stores);
#else
    (void)s;return false;
#endif
}
template<class State,class Call,class Before=Noop>bool bridge(State&s,Call&&local_child,Before before={}){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(!admitted(s))return false;
    const auto child=[&](auto&current,std::uint32_t target,std::uint32_t site,bool tail){
        if(target==native_arena_initialize_detail::Operands::reserve_child){
            if(site!=0x1252a4u||tail)throw std::runtime_error("Foreign native arena reservation call");
            // The exact owning dispatch sends this target to required(). Keep
            // that real provider/referenced ABI/observer, with the independently
            // qualified host-FP fence before importing its actual continuation.
            auto abi=current.save();
            native_required_abi_fp_detail::invoke(current,abi,[&]{
                current.services.required_owner(current.memory,{site,target,tail},abi,current.observe);
            });
        }else local_child(current,target,site,tail);
    };
    ++activity();native_arena_initialize_detail::run_private(s,std::move(before),child,{true,true});return true;
#else
    (void)s;(void)local_child;(void)before;return false;
#endif
}
} // namespace halo2::native_arena_initialize_owner_detail
