#pragma once
// Boundary adapter for the linked object query, entered at 10B0F0
// (native_tu15_linked_object_query_systems.hpp). Its callers and 15C7F0 keep
// the original ABI, which the adapter owns: EAX the object handle; EBX saved
// (BL the default 0); 15C7F0 gets the handle pushed (and pops it), and every
// register it returns comes back; the result in AL (MOV AL,1, or MOV AL,BL)
// above the rest of EAX as the original leaves it; ECX and EDX as the
// original leaves them; the flags of the last instruction that sets them;
// RET. Each access publishes its original instruction.
// HALO2_REFERENCE_LINKED_OBJECT_QUERY=1 restores the translated body.
// HALO2_VERIFY_LINKED_OBJECT_QUERY=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_linked_object_query_systems.hpp"

namespace halo2::native_linked_object_query_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_linked_object_query;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_LINKED_OBJECT_QUERY");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_LINKED_OBJECT_QUERY");return p && *p;}();
    return value;
}
enum Branch : unsigned {other_mode, unflagged, no_link, no_handle, refused, accepted, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Linked object query verify: %llu calls; %llu ordered accesses (%llu stores) "
            "identical, coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State,class Ask> struct LinkedQuery : Port<State> {
    using Access=logic::linked_object_query::Access;
    Ask &ask_child;
    LinkedQuery(State &s,std::uint32_t *c,Trace<State> *t,Ask &a):Port<State>(s,c,t,{}),ask_child(a){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case game:return 0x10b0f0u;
        case mode:return 0x10b0f6u;
        case headers:return 0x10b101u;
        case header_datums:return 0x10b107u;
        case object_address:return 0x10b112u;
        case flags:return 0x10b116u;
        case link:return 0x10b11fu;
        case linked_address:return 0x10b132u;
        case handle:return 0x10b136u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case game:gp[ecx]=v;break;
        // MOV EDX,[ECX+8]; PUSH EBX; XOR BL,BL; CMP EDX,2.
        case mode:gp[edx]=v;this->push(0x10b0f9u,gp[ebx]);set_low_byte(gp[ebx],fl.logic(0u,1));fl.minus(v,logic::queried_mode,4);break;
        case headers:gp[edx]=v;break;
        // MOV ECX,[EDX+44]; AND EAX,FFFF; LEA EAX,[EAX+EAX*2].
        case header_datums:gp[ecx]=v;gp[eax]=fl.logic(gp[eax]&0xffffu,4)*3u;break;
        case object_address:gp[eax]=v;break;
        case flags:this->test8(v&logic::queried_flag);break;                                       // TEST byte,1
        // MOV EAX,[EAX+154]; CMP EAX,-1; (AND EAX,FFFF; LEA EDX,[EAX+EAX*2]).
        case link:
            gp[eax]=v;fl.minus(v,logic::none,4);
            if(v!=logic::none){gp[eax]=fl.logic(v&0xffffu,4);gp[edx]=gp[eax]*3u;}break;
        case linked_address:gp[ecx]=v;break;
        case handle:gp[eax]=v;fl.minus(v,logic::none,4);break;                                      // CMP EAX,-1
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // PUSH EAX; CALL 15C7F0 (which pops it); TEST AL,AL.
    std::uint32_t ask(std::uint32_t handle) {
        auto &gp=this->gp;
        this->push(0x10b141u,handle);this->call(0x10b142u,0x10b147u,0x15c7f0u,ask_child);
        this->test8(gp[eax]);
        return gp[eax]&0xffu;
    }
    void run() {
        auto &gp=this->gp;
        Memory<LinkedQuery> memory{*this};
        std::uint32_t result=0;
        using logic::linked_object_query::Outcome;
        const auto outcome=logic::linked_object_query::query(memory,*this,gp[eax],result);
        static constexpr Branch branches[]{Branch::other_mode,Branch::unflagged,Branch::no_link,Branch::no_handle};
        if(outcome==Outcome::asked)this->reached(result?Branch::accepted:Branch::refused);
        else this->reached(branches[unsigned(outcome)]);
        set_low_byte(gp[eax],result?1u:gp[ebx]);                                                  // MOV AL,1 / MOV AL,BL
        gp[ebx]=this->pop(0x10b14fu);this->ret(0x10b150u,0);this->publish(0x10b150u);
    }
};
template<class State,class Reference,class Ask> bool bridge(State &s,Reference reference,Ask ask) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x10b0f0u,0x10b150u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return LinkedQuery<State,Ask>(v,c,t,ask);});
    return true;
}
} // namespace halo2::native_linked_object_query_detail
