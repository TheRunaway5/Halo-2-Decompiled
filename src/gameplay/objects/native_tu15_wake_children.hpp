#pragma once
// Boundary adapter for the wake children, entered at BE7E0
// (native_tu15_wake_tree_systems.hpp). Its callers and the children keep
// the original ABI, which the adapter owns: the handle at [ESP+4], RET 4;
// ESI saved (the object, then each child's header); B9390 gets EAX the
// handle; each child's BE7E0 gets the child pushed and ESI its header;
// every register a child returns comes back; the flags of the last
// instruction that sets them; each access publishes its original
// instruction.
// HALO2_REFERENCE_WAKE_TREE=1 restores the translated body.
// HALO2_VERIFY_WAKE_TREE=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/objects/native_tu15_wake_tree_systems.hpp"

namespace halo2::native_wake_children_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_wake_tree;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_WAKE_TREE");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_WAKE_TREE");return p && *p;}();
    return value;
}
struct Summary : native_object_tick_detail::Tally {
    const char *name;std::atomic<std::uint64_t> calls{},hit{};
    explicit Summary(const char *n):name(n){}
    ~Summary() {
        if(calls)std::fprintf(stderr,"%s verify: %llu calls; %llu ordered accesses (%llu stores) identical, coverage %llx\n",
            name,(unsigned long long)calls.load(),(unsigned long long)accesses.load(),(unsigned long long)stores.load(),
            (unsigned long long)hit.load());
    }
};
inline Summary &children_summary(){static Summary value{"Wake children"};return value;}
inline std::atomic<std::uint64_t> &children_coverage(){return children_summary().hit;}

enum Branch : unsigned {woken_type, other_type, no_children, next_child, last_child, branch_count};
namespace children {
inline constexpr Edge edges[]{
    {0xbe7ffu,0xbe811u,woken_type},{0xbe7ffu,0xbe816u,other_type},{0xbe816u,0xbe847u,no_children},
    {0xbe83fu,0xbe820u,next_child},{0xbe83fu,0xbe847u,last_child},
};
}
template<class State,class Chain,class Child> struct WakeChildren : Port<State> {
    using Access=logic::wake_children::Access;
    using Local=logic::wake_children::Local;
    using Kept=logic::wake_children::Kept;
    Chain &chain_child;Child &child_child;
    WakeChildren(State &s,std::uint32_t *c,Trace<State> *t,Chain &a,Child &b)
        :Port<State>(s,c,t,children::edges),chain_child(a),child_child(b){}
    std::uint32_t local(Local)const{return this->gp[esp]+4u;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case handle_argument:return 0xbe7e0u;
        case headers:return 0xbe7e4u;
        case datums:return 0xbe7eau;
        case object:return 0xbe7fcu;
        case type:return 0xbe7ffu;
        case first_child:return 0xbe816u;
        case child_headers:return 0xbe820u;
        case child_datums:return 0xbe826u;
        case child_object:return 0xbe83cu;
        case sibling:return 0xbe83fu;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case handle_argument:case child_object:gp[eax]=v;break;
        case headers:case child_headers:gp[edx]=v;break;
        // MOV ECX,EAX; AND ECX,FFFF; LEA ECX,[ECX+ECX*2]; LEA ECX,[EDX+ECX*4]; PUSH ESI.
        case datums:
            gp[edx]=v;gp[ecx]=v+this->flags.logic(gp[eax]&0xffffu,4)*12u;this->push(0xbe7fbu,gp[esi]);break;
        case object:gp[esi]=v;break;
        // MOV CL; MOV EDX,1; SHL EDX,CL; TEST EDX,1003.
        case type:
            set_low_byte(gp[ecx],v);gp[edx]=this->flags.shift(1u,v&0xffu,4);this->test32(gp[edx]&logic::woken_types);break;
        case first_child:case sibling:gp[ecx]=v;this->compare32(v,logic::none);break;
        // MOV EAX,ECX; AND EAX,FFFF; LEA EAX,[EAX+EAX*2]; PUSH ECX; LEA ESI,[EDX+EAX*4].
        case child_datums:
            gp[edx]=v;gp[eax]=this->flags.logic(gp[ecx]&0xffffu,4)*3u;this->push(0xbe833u,gp[ecx]);gp[esi]=v+gp[eax]*4u;break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void returned(Kept &kept)const{kept={this->gp[esi],this->gp[esi]};}
    void wake_chain(Kept &kept){this->call(0xbe811u,0xbe816u,0xb9390u,chain_child);returned(kept);}
    void wake_child(Kept &kept){this->call(0xbe837u,0xbe83cu,0xbe7e0u,child_child);returned(kept);}
    void run() {
        auto &gp=this->gp;
        Memory<WakeChildren> memory{*this};
        logic::wake_children::wake(memory,*this);
        gp[esi]=this->pop(0xbe847u);this->ret(0xbe848u,4u);this->publish(0xbe848u);
    }
};

template<class State,class Reference,class Chain,class Child> bool bridge_children(State &s,Reference reference,Chain chain,Child child) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0xbe7e0u,0xbe848u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),children_coverage(),children_summary().calls,children_summary(),
        reference,[&](State &v,std::uint32_t *c,Trace<State> *t){return WakeChildren<State,Chain,Child>(v,c,t,chain,child);});
    return true;
}
} // namespace halo2::native_wake_children_detail
