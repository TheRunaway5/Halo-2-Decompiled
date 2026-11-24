#pragma once
// Boundary adapter for the wake tree, entered at BDA50
// (native_tu15_wake_tree_systems.hpp). Its callers and the children keep
// the original ABI, which the adapter owns: EAX the handle, RET; EBX, ESI
// and EDI saved (the datums, the handle, the header); BCFB0 gets ECX the
// handle; B9390 EAX; BE7E0 the handle pushed; every register a child
// returns comes back; the flags of the last instruction that sets them;
// each access publishes its original instruction.
// HALO2_REFERENCE_WAKE_TREE=1 restores the translated body.
// HALO2_VERIFY_WAKE_TREE=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/objects/native_tu15_wake_tree_systems.hpp"

namespace halo2::native_wake_tree_detail {
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
inline Summary &wake_summary(){static Summary value{"Wake tree"};return value;}
inline std::atomic<std::uint64_t> &wake_coverage(){return wake_summary().hit;}

enum Branch : unsigned {indirect, direct, active, inactive, branch_count};
namespace wake {
inline constexpr Edge edges[]{
    {0xbda6cu,0xbda74u,indirect},{0xbda6cu,0xbda86u,direct},{0xbda86u,0xbda8eu,active},{0xbda86u,0xbda97u,inactive},
};
}
template<class State,class Resolve,class Chain,class Children> struct Wake : Port<State> {
    using Access=logic::wake::Access;
    using Kept=logic::wake::Kept;
    Resolve &resolve_child;Chain &chain_child;Children &children_child;
    Wake(State &s,std::uint32_t *c,Trace<State> *t,Resolve &r,Chain &a,Children &b)
        :Port<State>(s,c,t,wake::edges),resolve_child(r),chain_child(a),children_child(b){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case headers:return 0xbda54u;
        case datums:return 0xbda59u;
        case flags:return 0xbda66u;
        case active_flags:return 0xbda86u;
        case moved_read:case moved_store:return 0xbda93u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case headers:gp[eax]=v;break;
        // MOV EAX,ESI; AND EAX,FFFF; LEA ECX,[EAX+EAX*2].
        case datums:gp[ebx]=v;gp[eax]=this->flags.logic(gp[esi]&0xffffu,4);gp[ecx]=gp[eax]*3u;break;
        // TEST AL,AL; PUSH EDI; LEA EDI,[EBX+ECX*4].
        case flags:set_low_byte(gp[eax],v);this->test8(v);this->push(0xbda6cu,gp[edi]);gp[edi]=gp[ebx]+gp[ecx]*4u;break;
        case active_flags:this->test8(v&logic::header_flags::active);break;
        case moved_read:this->flags.logic((v|logic::header_flags::moved)&0xffu,1);break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void returned(Kept &kept)const{kept={this->gp[ebx],this->gp[esi],this->gp[edi]};}
    // MOV ECX,ESI; CALL BCFB0; MOV ESI,EAX; AND EAX,FFFF; LEA EDX,[EAX+EAX*2]; LEA EDI,[EBX+EDX*4].
    std::uint32_t resolve(Kept &kept) {
        auto &gp=this->gp;
        gp[ecx]=gp[esi];this->call(0xbda74u,0xbda79u,0xbcfb0u,resolve_child);returned(kept);
        const auto handle=gp[eax];
        gp[esi]=handle;gp[eax]=this->flags.logic(handle&0xffffu,4);gp[edx]=gp[eax]*3u;gp[edi]=gp[ebx]+gp[edx]*4u;
        return handle;
    }
    void wake_chain(Kept &kept){this->gp[eax]=this->gp[esi];this->call(0xbda8eu,0xbda93u,0xb9390u,chain_child);returned(kept);}
    void wake_children(Kept &kept) {
        this->push(0xbda97u,this->gp[esi]);this->call(0xbda98u,0xbda9du,0xbe7e0u,children_child);returned(kept);
    }
    void run() {
        auto &gp=this->gp;
        this->push(0xbda50u,gp[ebx]);this->push(0xbda51u,gp[esi]);gp[esi]=gp[eax];                // PUSH EBX; PUSH ESI; MOV ESI,EAX
        Memory<Wake> memory{*this};
        logic::wake::wake(memory,*this,gp[esi]);
        gp[edi]=this->pop(0xbda9du);gp[esi]=this->pop(0xbda9eu);gp[ebx]=this->pop(0xbda9fu);
        this->ret(0xbdaa0u,0);this->publish(0xbdaa0u);
    }
};
template<class State,class Reference,class Resolve,class Chain,class Children>
bool bridge_wake(State &s,Reference reference,Resolve resolve,Chain chain,Children children) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0xbda50u,0xbdaa0u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),wake_coverage(),wake_summary().calls,wake_summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Wake<State,Resolve,Chain,Children>(v,c,t,resolve,chain,children);});
    return true;
}
} // namespace halo2::native_wake_tree_detail
