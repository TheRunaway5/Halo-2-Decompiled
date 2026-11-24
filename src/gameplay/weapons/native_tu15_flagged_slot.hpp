#pragma once
// Boundary adapter for the flagged slot, entered at D2330
// (native_tu15_flagged_slot_systems.hpp). Its callers keep the original ABI,
// which the adapter owns: ECX the object handle; EBP, ESI and EDI saved (NONE,
// the slot walk, the tag instances); the result in EAX (the index, or
// MOV AX,BP above a zero high word); ECX and EDX as the original leaves
// them; the flags of the last instruction that sets them; RET. Each access
// publishes its original instruction.
// HALO2_REFERENCE_FLAGGED_SLOT=1 restores the translated body.
// HALO2_VERIFY_FLAGGED_SLOT=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/weapons/native_tu15_flagged_slot_systems.hpp"

namespace halo2::native_flagged_slot_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_flagged_slot;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_FLAGGED_SLOT");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_FLAGGED_SLOT");return p && *p;}();
    return value;
}
enum Branch : unsigned {empty_slot, unflagged, found, not_found, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Flagged slot verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State> struct FlaggedSlot : Port<State> {
    using Access=logic::flagged_slot::Access;
    std::uint32_t index{};
    FlaggedSlot(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case headers:return 0xd2330u;
        case header_datums:return 0xd2335u;
        case object_address:return 0xd2343u;
        case instances:return 0xd2348u;
        case slot:return 0xd2360u;
        case slot_object:return 0xd2370u;
        case slot_tag:return 0xd2374u;
        case slot_definition:return 0xd237fu;
        case flags:return 0xd2383u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case headers:gp[eax]=v;break;
        // MOV EDX,[EAX+44]; AND ECX,FFFF; PUSH EBP; PUSH ESI; LEA ECX,[ECX+ECX*2].
        case header_datums:
            gp[edx]=v;gp[ecx]=fl.logic(gp[ecx]&0xffffu,4);this->push(0xd233eu,gp[ebp]);this->push(0xd233fu,gp[esi]);
            gp[ecx]*=3u;break;
        case object_address:gp[ecx]=v;this->push(0xd2347u,gp[edi]);break;                           // PUSH EDI
        // MOV EDI,[4F0414]; OR EBP,-1; XOR EAX,EAX; LEA ESI,[ECX+218].
        case instances:
            gp[edi]=v;gp[ebp]=fl.logic(gp[ebp]|logic::none,4);gp[eax]=fl.logic(0u,4);gp[esi]=gp[ecx]+logic::slots;break;
        // MOV ECX,[ESI]; CMP ECX,-1; (AND ECX,FFFF; LEA ECX,[ECX+ECX*2]).
        case slot:
            gp[ecx]=v;fl.minus(v,logic::none,4);
            if(v==logic::none)this->reached(Branch::empty_slot);else gp[ecx]=fl.logic(v&0xffffu,4)*3u;
            break;
        case slot_object:gp[ecx]=v;break;
        case slot_tag:gp[ecx]=fl.shift(fl.logic(v&0xffffu,4),logic::tag_instance_shift,4);break;  // AND ECX,FFFF; SHL ECX,4
        case slot_definition:gp[ecx]=v;break;
        // MOV ECX,[ECX+12C]; SHR ECX,3; TEST CL,1.
        case flags:
            gp[ecx]=fl.right(v,logic::flag_bit,4,false);this->test8(gp[ecx]&1u);
            if(!(gp[ecx]&1u))this->reached(Branch::unflagged);break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // INC EAX; ADD ESI,4; CMP EAX,4 before every slot past the first.
    void slot_reached(std::uint32_t next) {
        auto &gp=this->gp;auto &fl=this->flags;
        if(next) {
            gp[eax]=fl.increment(gp[eax],4,false);gp[esi]=fl.plus(gp[esi],4u,4);fl.minus(gp[eax],logic::slot_count,4);
        }
        index=next;
    }
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        Memory<FlaggedSlot> memory{*this};
        const auto result=logic::flagged_slot::find(memory,*this,gp[ecx]);
        if(result==logic::not_found) {
            // The last slot's INC EAX; ADD ESI,4; CMP EAX,4; then MOV AX,BP.
            gp[eax]=fl.increment(gp[eax],4,false);gp[esi]=fl.plus(gp[esi],4u,4);fl.minus(gp[eax],logic::slot_count,4);
            gp[eax]=(gp[eax]&0xffff0000u)|(gp[ebp]&0xffffu);this->reached(Branch::not_found);
        } else this->reached(Branch::found);
        gp[edi]=this->pop(0xd239du);gp[esi]=this->pop(0xd239eu);gp[ebp]=this->pop(0xd239fu);
        this->ret(0xd23a0u,0);this->publish(0xd23a0u);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0xd2330u,0xd23a0u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return FlaggedSlot<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_flagged_slot_detail
