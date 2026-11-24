#pragma once
// Boundary adapter for the slot kinds, entered at 106230
// (native_tu15_slot_kinds_systems.hpp). Its callers keep the original ABI,
// which the adapter owns: EAX the handle; AL the result; EDI saved, ESI
// once the slots are walked; ECX the last kind, EDX the slot countdown; the
// flags of the last instruction that sets them; RET. Each access publishes
// its original instruction.
// HALO2_REFERENCE_SLOT_KINDS=1 restores the translated body.
// HALO2_VERIFY_SLOT_KINDS=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_slot_kinds_systems.hpp"

namespace halo2::native_slot_kinds_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_slot_kinds;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_SLOT_KINDS");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_SLOT_KINDS");return p && *p;}();
    return value;
}
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Slot kinds verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
enum Branch : unsigned {no_slots, active_kind, conditional_kind, other_kind, branch_count};
template<class State> struct Kinds : Port<State> {
    using Access=logic::slot_kinds::Access;
    bool walked{};
    Kinds(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case object_table:return 0x106230u;
        case object_datums:return 0x106236u;
        case object_address:gp[eax]=this->flags.logic(gp[eax]&0xffffu,4)*3u;return 0x106241u;  // AND; LEA EAX,[EAX+EAX*2]
        case object_tag:return 0x106245u;
        case instances:return 0x106247u;
        // AND EAX,FFFF; SHL EAX,4; PUSH EDI.
        case tag:gp[eax]=this->flags.shift(this->flags.logic(gp[eax]&0xffffu,4),logic::instance_shift,4);this->push(0x106255u,gp[edi]);
            return 0x106256u;
        case count:return 0x10625au;
        // (PUSH ESI; LEA ESI,[ECX+20C] the first time.)
        case kind:if(!walked){walked=true;this->push(0x106266u,gp[esi]);gp[esi]=gp[ecx]+logic::slots;}return 0x106270u;
        case conditional:return 0x10628bu;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case object_table:case object_address:gp[ecx]=v;break;
        case object_datums:case instances:gp[edx]=v;break;
        case object_tag:gp[eax]=v;break;
        case tag:gp[edi]=v;break;
        // XOR AL,AL; TEST EDX,EDX.
        case count:gp[edx]=v;set_low_byte(gp[eax],this->flags.logic(0u,1));this->test32(v);
            if(std::int32_t(v)<=0)this->reached(Branch::no_slots);
            break;
        // MOV CL; CMP CL,3/7/5/6/2.
        case kind: {
            set_low_byte(gp[ecx],v);
            static constexpr std::uint32_t order[]{3u,7u,5u,6u,2u};
            for(const auto k:order){this->flags.minus(v&0xffu,k,1);if((v&0xffu)==k)break;}
            this->reached(logic::always_active(v&0xffu)?Branch::active_kind:(v&0xffu)==logic::conditional_kind?Branch::conditional_kind:
                          Branch::other_kind);
            break;
        }
        case conditional:this->flags.minus(v&0xffffu,0u,2);break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // (MOV AL,1;) ADD ESI,C; DEC EDX.
    void slot(std::uint32_t,bool active) {
        auto &gp=this->gp;
        if(active)set_low_byte(gp[eax],1u);
        gp[esi]=this->flags.plus(gp[esi],logic::slot_size,4);gp[edx]=this->flags.increment(gp[edx],4,true);
    }
    void run() {
        auto &gp=this->gp;
        Memory<Kinds> memory{*this};
        (void)logic::slot_kinds::any(memory,*this,gp[eax]);
        if(walked)gp[esi]=this->pop(0x10629du);
        gp[edi]=this->pop(0x10629eu);this->ret(0x10629fu,0);this->publish(0x10629fu);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x106230u,0x10629fu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Kinds<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_slot_kinds_detail
