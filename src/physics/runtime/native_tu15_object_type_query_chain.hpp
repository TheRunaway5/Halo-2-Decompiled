#pragma once
// Boundary adapter for the object type query chain, entered at 10DDE0
// (native_tu15_object_type_query_chain_systems.hpp). Its callers and the
// queries keep the original ABI, which the adapter owns: EDI the object
// handle, EBX the extra, the argument at [ESP+4], RET 4; EBP and ESI saved
// (the argument, the chain position); each query gets EBX, EBP and EDI
// pushed (and pops them) and returns AL, and every register it returns
// comes back; EAX, ECX and EDX as the original leaves them; the flags of
// the last instruction that sets them. Each access publishes its original
// instruction.
// HALO2_REFERENCE_OBJECT_TYPE_QUERY_CHAIN=1 restores the translated body.
// HALO2_VERIFY_OBJECT_TYPE_QUERY_CHAIN=1 records the native run and replays
// the unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_object_type_query_chain_systems.hpp"

namespace halo2::native_object_type_query_chain_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_object_type_query_chain;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_OBJECT_TYPE_QUERY_CHAIN");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_OBJECT_TYPE_QUERY_CHAIN");return p && *p;}();
    return value;
}
enum Branch : unsigned {no_query, refused, answered, exhausted, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Object type query chain verify: %llu calls; %llu ordered accesses (%llu stores) "
            "identical, coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State,class Route> struct QueryChain : Port<State> {
    using Access=logic::object_type_query_chain::Access;
    using Kept=logic::object_type_query_chain::Kept;
    Route &route;
    QueryChain(State &s,std::uint32_t *c,Trace<State> *t,Route &r):Port<State>(s,c,t,{}),route(r){}
    // [ESP+4] before the PUSH EBP that precedes its read (the original's [ESP+8]).
    std::uint32_t argument()const{return this->gp[esp]+4u;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case object_table:return 0x10dde0u;
        case object_datums:return 0x10dde6u;
        case object_address:return 0x10ddf3u;
        case type:return 0x10ddf7u;
        case definition:return 0x10ddfeu;
        case argument:this->push(0x10de05u,this->gp[ebp]);return 0x10de06u;                        // PUSH EBP
        case chain_entry:return 0x10de13u;
        case query:return 0x10de19u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case object_table:gp[ecx]=v;break;
        // MOV EDX,[ECX+44]; MOV EAX,EDI; AND EAX,FFFF; LEA EAX,[EAX+EAX*2].
        case object_datums:gp[edx]=v;gp[eax]=fl.logic(gp[edi]&0xffffu,4)*3u;break;
        case object_address:gp[eax]=v;break;
        case type:gp[ecx]=std::uint32_t(std::int32_t(std::int8_t(v)));break;                      // MOVSX ECX
        case definition:gp[ecx]=v;break;
        case argument:gp[ebp]=v;break;
        // MOV ECX,[ESI]; TEST ECX,ECX / MOV ECX,[ECX+48]; TEST ECX,ECX.
        case chain_entry:case query:gp[ecx]=v;this->test32(v);
            if(a==query && !v)this->reached(Branch::no_query);break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // PUSH ESI; XOR AL,AL; LEA ESI,[ECX+84].
    void started(const Kept &kept) {
        auto &gp=this->gp;
        this->push(0x10de0au,gp[esi]);set_low_byte(gp[eax],this->flags.logic(0u,1));gp[esi]=kept.position;
    }
    // PUSH EBX; PUSH EBP; PUSH EDI; CALL ECX.
    std::uint32_t ask(std::uint32_t query,Kept &kept) {
        auto &gp=this->gp;
        this->push(0x10de20u,gp[ebx]);this->push(0x10de21u,gp[ebp]);this->push(0x10de22u,gp[edi]);
        this->call(0x10de23u,0x10de25u,query,[&](State &v){route(v,query,0x10de23u);});
        kept.position=gp[esi];
        const auto al=gp[eax]&0xffu;
        this->reached(al?Branch::answered:Branch::refused);
        return al;
    }
    // ADD ESI,4; TEST AL,AL.
    void stepped(const Kept &){auto &gp=this->gp;gp[esi]=this->flags.plus(gp[esi],4u,4);this->test8(gp[eax]);}
    void run() {
        auto &gp=this->gp;
        Memory<QueryChain> memory{*this};
        if(!logic::object_type_query_chain::ask(memory,*this,gp[edi]))this->reached(Branch::exhausted);
        gp[esi]=this->pop(0x10de2cu);gp[ebp]=this->pop(0x10de2du);
        this->ret(0x10de2eu,4u);this->publish(0x10de2eu);
    }
};
template<class State,class Reference,class Route> bool bridge(State &s,Reference reference,Route route) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x10dde0u,0x10de2eu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return QueryChain<State,Route>(v,c,t,route);});
    return true;
}
} // namespace halo2::native_object_type_query_chain_detail
