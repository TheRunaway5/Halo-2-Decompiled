#pragma once
#include "halo2/physics_contact_runtime.hpp"
#include "physics/runtime/native_tu15_required_owner_policy.hpp"
#include <cstdlib>
#include <exception>
#include <initializer_list>
#include <thread>

namespace halo2 {class PhysicsLiveWorldTu15;class SourcePhysicsGameArenaNative;}
namespace halo2::native_arena_world_publication_detail {
using Services=SourcePhysicsContactRuntimeServices;

// Source construction, rather than a callback type alone, owns this contract.
// The World creates its Arena from this exact published immutable table. Its
// private scope then binds that concrete Arena and bank for one synchronous
// call. The Arena's private issuer checks its constructor's unchanged lower
// copy before publishing the actual const joined table used by the State.
inline bool same_instruction(const Services&left,const Services&right)noexcept {
    const auto&a=left.instruction_cursor;const auto&b=right.instruction_cursor;
    return left.instruction&&right.instruction&&left.instruction.target_type()==right.instruction.target_type()&&
        a.position&&a.first_trace&&a.second_trace&&a.position==b.position&&a.first_trace==b.first_trace&&a.second_trace==b.second_trace&&
        a.callback_type&&a.callback_type==b.callback_type&&left.instruction.target_type()==*a.callback_type&&
        a.executor==b.executor&&a.executor==std::this_thread::get_id()&&
        a.literal_service_sites&&b.literal_service_sites&&a.service_sites.data()==b.service_sites.data()&&
        a.service_sites.size()==3&&b.service_sites.size()==3&&a.service_sites[0]==0x38846eu&&
        a.service_sites[1]==0x21f590u&&a.service_sites[2]==0x33b595u&&
        !(a.first_trace&&*a.first_trace)&&!(a.second_trace&&*a.second_trace);
}

class Construction final {
    friend class halo2::PhysicsLiveWorldTu15;
    const SourcePhysicsGameArenaNative*const arena_;
    const SourcePhysicsContactMemory*const bank_;
    const Services*const producer_;
    const void*const world_;
    const std::thread::id executor_=std::this_thread::get_id();
    const Construction*const previous_;
    inline static thread_local const Construction*active_{};
    Construction(const SourcePhysicsGameArenaNative&arena,const SourcePhysicsContactMemory&bank,
        const Services&producer,const void*world)noexcept
        :arena_(&arena),bank_(&bank),producer_(&producer),world_(world),previous_(active_){active_=this;}
public:
    ~Construction(){if(active_!=this||executor_!=std::this_thread::get_id())std::terminate();active_=previous_;}
    Construction(const Construction&)=delete;
    Construction&operator=(const Construction&)=delete;
    static const Construction*current()noexcept{return active_;}
    static bool qualified(const SourcePhysicsGameArenaNative&arena,const SourcePhysicsContactMemory&bank,
        const Services&lower)noexcept {
        // Like the authored World callback, instruction diagnostics are fixed
        // process configuration. Raw REP has a direct trailing callback even
        // in prepared source, so a census requests the complete retained path.
        static const bool diagnostics=[]{for(const char*name:{"HALO2_INSTRUCTION_CENSUS","HALO2_REFERENCE_INSTRUCTION_CALLBACKS","HALO2_REFERENCE_LITERAL_INSTRUCTIONS"}){const auto*p=std::getenv(name);if(p&&*p)return true;}return false;}();
        const auto*c=active_;
        if(diagnostics||!c||c->arena_!=&arena||c->bank_!=&bank||!c->world_||
           c->executor_!=std::this_thread::get_id()||!same_instruction(lower,*c->producer_))return false;
        const auto&a=lower.required_owner_policy;const auto&b=c->producer_->required_owner_policy;
        return a.owner==c->world_&&a.owner==b.owner&&a.callback_type==b.callback_type&&
            a.executor==b.executor&&a.matches==b.matches&&
            native_required_owner_policy_detail::preserves_instruction_policy(lower)&&
            native_required_owner_policy_detail::preserves_instruction_policy(*c->producer_);
    }
};

class Borrow final {
    friend class halo2::SourcePhysicsGameArenaNative;
    using Match=bool(*)(const Services&,const void*,const SourcePhysicsGameArenaNative&)noexcept;
    const SourcePhysicsGameArenaNative*const arena_;
    const SourcePhysicsContactMemory*const memory_;
    const Services*const services_;
    const void*const owner_;
    const Match matches_;
    const Construction*const construction_=Construction::current();
    const std::thread::id executor_=std::this_thread::get_id();
    const Borrow*const previous_;
    inline static thread_local const Borrow*active_{};
    Borrow(const SourcePhysicsGameArenaNative&arena,const SourcePhysicsContactMemory&memory,
        const Services&services,const void*owner,Match matches)noexcept
        :arena_(&arena),memory_(&memory),services_(&services),owner_(owner),matches_(matches),previous_(active_){active_=this;}
public:
    ~Borrow(){if(active_!=this||executor_!=std::this_thread::get_id())std::terminate();active_=previous_;}
    Borrow(const Borrow&)=delete;
    Borrow&operator=(const Borrow&)=delete;
    static bool qualified(const SourcePhysicsContactMemory&memory,const Services&services)noexcept {
        const auto*b=active_;
        return b&&b->executor_==std::this_thread::get_id()&&b->memory_==&memory&&b->services_==&services&&
            b->construction_&&b->construction_==Construction::current()&&b->owner_&&b->matches_&&
            b->matches_(services,b->owner_,*b->arena_);
    }
};
} // namespace halo2::native_arena_world_publication_detail
