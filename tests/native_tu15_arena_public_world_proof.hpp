#pragma once
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_arena_initialize_source_frame.hpp"
#include "physics/runtime/native_tu15_arena_world_publication.hpp"
#include "physics/runtime/native_tu15_arena_world_initialize_owner.hpp"
#include "physics/runtime/native_tu15_required_owner_policy.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include <fstream>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>

// Disposable full-public-World proof only. No production source includes it.
namespace halo2::arena_public_world_proof {
using State=physics_world_runtime_detail::WorldState;using J=nlohmann::json;
inline std::string hash(std::span<const std::byte>bytes){
    std::array<unsigned char,EVP_MAX_MD_SIZE>value{};unsigned count{};
    if(EVP_Digest(bytes.data(),bytes.size(),value.data(),&count,EVP_sha256(),nullptr)!=1)throw std::runtime_error("Public Arena proof digest failed");
    constexpr char hex[]="0123456789abcdef";std::string result;
    for(unsigned n=0;n<count;++n){result+=hex[value[n]>>4];result+=hex[value[n]&15];}return result;
}
inline J packet(const SourcePhysicsContactRuntimeAbi&a){
    J live=J::array();for(unsigned n=0;n<a.x87_depth;++n)live.push_back(hash(std::span(reinterpret_cast<const std::byte*>(&a.x87[n]),10)));
    return J{{"gp",a.gp},{"eflags",a.eflags},{"fs",a.fs_base},{"xmm",a.xmm},{"live_x87_sha256",live},{"x87_count",a.x87_depth},{"x87_status",a.x87_status},{"cw",a.floating_point?a.floating_point->x87_control_word:0},{"mxcsr",a.floating_point?a.floating_point->mxcsr:0},{"mmx",bool(a.active_mmx)}};
}
struct Construction;
inline thread_local Construction*construction{};
struct CallBorrow{
    const SourcePhysicsContactMemory*memory;const SourcePhysicsContactRuntimeServices*services;const CallBorrow*previous;
    inline static thread_local const CallBorrow*active{};
    CallBorrow(const SourcePhysicsContactMemory&m,const SourcePhysicsContactRuntimeServices&s):memory(&m),services(&s),previous(active){active=this;}
    ~CallBorrow(){active=previous;}
};
struct Construction{
    SourceRuntimeMemory&bank;std::uint32_t&position;bool&first;bool&second;
    const std::thread::id executor=std::this_thread::get_id();const bool original_first,original_second;
    const unsigned policy=[]{const auto*p=std::getenv("HALO2_ARENA_PUBLIC_WORLD_POLICY");return p?unsigned(std::stoul(p)):0u;}();
    unsigned words{},changes{},native_calls{},handoffs{},retained{};std::uint32_t handoff_pc{};bool finished{};
    J children=J::array();
    Construction(SourceRuntimeMemory&m,std::uint32_t&p,bool&one,bool&two):bank(m),position(p),first(one),second(two),original_first(one),original_second(two){
        if(policy>11u)throw std::runtime_error("Unknown private public World Arena policy");
        if(construction)throw std::runtime_error("Nested actual World Arena construction proof");construction=this;
        if(policy&&policy<=7u)source_runtime_register_write_guard(bank,this,[this](auto,auto){
            if(std::this_thread::get_id()!=executor||changes)return;
            const bool trigger=policy==1?position==0x125280u:policy==2?position==0x1252b8u&&++words==1u:policy==3?position==0x1252b8u&&++words==0xff800u:policy==4?position==0x1252d2u:policy==6?position==0x1252b8u&&++words==0x7fc00u:policy==7?position==0x1252adu:position==0x125328u;
            if(trigger){++changes;first=true;}
        });
    }
    ~Construction(){source_runtime_unregister_write_guard(bank,this);first=original_first;second=original_second;construction=nullptr;}
    template<class Arena>void finish(const SourcePhysicsContactRuntimeAbi&abi,const Arena&arena,const std::vector<std::uint32_t>&trace){
        if(finished)throw std::runtime_error("Duplicate actual World Arena construction receipt");finished=true;
        if(policy&&changes!=1)throw std::runtime_error("Actual public World Arena guard was not reached");
        const auto owner=arena.ownership_checkpoint();
        if(owner.files.size()!=2||owner.physical.allocations.size()!=1||owner.synchronization.objects.size()!=3)throw std::runtime_error("Actual World Arena resource ownership differs");
        J banks=J::array();bank.visit_committed([&](auto address,auto bytes){banks.push_back(J{{"address",address},{"bytes",bytes.size()},{"sha256",hash(bytes)}});});
        J files=J::array();for(const auto&file:owner.files)files.push_back(J{{"alias",file.alias},{"position",file.position},{"name",file.name},{"bytes",file.bytes.size()},{"sha256",hash(std::span(file.bytes))}});
        J child=children;
        J result{{"passed",true},{"private_unbound",true},{"actual_public_world_constructor",true},{"scheduling_hooks_changed",false},{"policy",policy},{"guard_changes",changes},{"native_calls",native_calls},{"handoffs",handoffs},{"handoff_pc",handoff_pc},{"retained_parent_statements",retained},{"escaped_abi",packet(abi)},{"children",child},{"banks",banks},{"trace_count",trace.size()},{"trace_sha256",hash(std::as_bytes(std::span(trace)))},{"position",position},{"real_files",files},{"real_worker_retired",true}};
        const auto*out=std::getenv("HALO2_ARENA_PUBLIC_WORLD_REPORT");if(!out||!*out)throw std::runtime_error("Public World Arena proof report not declared");
        std::ofstream stream(out);stream<<result.dump(2)<<'\n';if(!stream)throw std::runtime_error("Public World Arena proof report failed");
    }
};
inline bool reference(){static const bool value=std::getenv("HALO2_ARENA_PUBLIC_WORLD_REFERENCE");return value;}
inline bool qualified_producer(){static const bool value=std::getenv("HALO2_ARENA_PUBLIC_WORLD_QUALIFIED_PRODUCER");return value;}
inline bool eligible(const State&s){
    const auto*b=CallBorrow::active;const auto*w=construction;const auto&c=s.services.instruction_cursor;std::uint16_t cw;asm volatile("fnstcw %0":"=m"(cw));
    const bool issuer=qualified_producer()?native_arena_world_publication_detail::Borrow::qualified(s.memory,s.services):b&&b->memory==&s.memory&&b->services==&s.services;
    return w&&issuer&&std::this_thread::get_id()==w->executor&&
        s.image==SourcePhysicsContactImage::TitleUpdate15&&!s.active_mmx&&s.fp.count<=8u&&s.fp.top<8u&&
        (cw&0x3fu)==0x3fu&&(_mm_getcsr()&0x1f80u)==0x1f80u&&!s.observe&&native_required_owner_policy_detail::preserves_instruction_policy(s.services)&&
        s.services.instruction&&s.instruction_cursor==&c&&s.literal_service_sites&&c.position==&w->position&&c.callback_type&&
        s.services.instruction.target_type()==*c.callback_type&&c.executor==w->executor&&c.literal_service_sites&&c.service_sites.size()==3&&
        c.service_sites[0]==0x38846eu&&c.service_sites[1]==0x21f590u&&c.service_sites[2]==0x33b595u&&
        !(c.first_trace&&*c.first_trace)&&!(c.second_trace&&*c.second_trace);
}
inline void statement(std::uint32_t){if(construction)++construction->retained;}
template<class Call>void child(State&s,std::uint32_t target,std::uint32_t site,Call call){
    if(construction)construction->children.push_back(J{{"before",true},{"target",target},{"site",site},{"abi",packet(s.save())}});
    try{call();}catch(...){const auto failure=std::current_exception();if(construction)construction->children.push_back(J{{"before",false},{"target",target},{"site",site},{"abi",packet(s.save())},{"exception",true}});std::rethrow_exception(failure);}
    if(construction)construction->children.push_back(J{{"before",false},{"target",target},{"site",site},{"abi",packet(s.save())},{"exception",false}});
    if(construction&&std::this_thread::get_id()==construction->executor&&!construction->changes){
        const auto policy=construction->policy;
        const auto expected=policy==8u?0x1252a4u:policy==9u?0x1252bau:policy==10u?0x1252edu:policy==11u?0x125321u:0u;
        if(expected&&site==expected){++construction->changes;construction->first=true;}
    }
}
template<class Call>std::uint32_t entry(State&s,Call call){
    if(reference()||!eligible(s))return 1u;++construction->native_calls;
    const auto invoke=[&](auto&state,std::uint32_t target,std::uint32_t site,bool tail){child(state,target,site,[&]{call(state,target,site,tail);});};
    const auto next=native_arena_initialize_source_detail::run_source(s,eligible,invoke);
    if(next){++construction->handoffs;construction->handoff_pc=next;}return next;
}
template<class Call>std::uint32_t owner_entry(State&s,Call call){
    if(reference())return 0u;
    const auto before=native_arena_world_initialize_owner_detail::activity();
    const auto invoke=[&](auto&state,std::uint32_t target,std::uint32_t site,bool tail){child(state,target,site,[&]{call(state,target,site,tail);});};
    // The wrapper requires the actual compiled body to have all 45 prepared
    // literal callbacks before executing this explicit prepared instantiation.
    const auto next=native_arena_world_initialize_owner_detail::bridge<true>(s,invoke);
    if(native_arena_world_initialize_owner_detail::activity()!=before){
        if(!construction)throw std::runtime_error("World Arena bridge escaped construction proof");
        ++construction->native_calls;
        if(next&&next!=0xffffffffu){++construction->handoffs;construction->handoff_pc=next;}
    }
    return next;
}
}
