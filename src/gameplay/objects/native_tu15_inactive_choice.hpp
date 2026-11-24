#pragma once
// Boundary adapter for the inactive choice query, entered at 106040
// (native_tu15_inactive_choice_systems.hpp). Its callers and 105950 keep the
// original ABI, which the adapter owns: EAX the object handle; EBX, ESI and
// EDI saved; 105950 gets EAX the handle, EBX and ESI NONE, EDI the
// definition, and every register it returns comes back; the result in AL
// (XOR EAX,EAX; SETNE AL) with the rest of EAX zero; ECX and EDX as the
// original leaves them; the flags of the last instruction that sets them
// (the CMP SI,-1); RET. Each access publishes its original instruction.
// HALO2_REFERENCE_INACTIVE_CHOICE=1 restores the translated body.
// HALO2_VERIFY_INACTIVE_CHOICE=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/objects/native_tu15_inactive_choice_systems.hpp"

namespace halo2::native_inactive_choice_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_inactive_choice;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_INACTIVE_CHOICE");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_INACTIVE_CHOICE");return p && *p;}();
    return value;
}
enum Branch : unsigned {active, counted, uncounted, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Inactive choice verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State,class Active> struct InactiveChoice : Port<State> {
    using Access=logic::inactive_choice::Access;
    using Kept=logic::inactive_choice::Kept;
    Active &active_child;
    InactiveChoice(State &s,std::uint32_t *c,Trace<State> *t,Active &a):Port<State>(s,c,t,{}),active_child(a){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case headers:return 0x106040u;
        case header_datums:return 0x106046u;
        case object_address:return 0x106054u;
        case tag:return 0x106058u;
        case tags:return 0x10605au;
        case definition:this->push(0x10606bu,this->gp[edi]);return 0x10606cu;                      // PUSH EDI
        case count:return 0x10607fu;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case headers:gp[edx]=v;break;
        // MOV EDX,[EDX+44]; MOV ECX,EAX; AND ECX,FFFF; LEA ECX,[ECX+ECX*2].
        case header_datums:gp[edx]=v;gp[ecx]=fl.logic(gp[eax]&0xffffu,4)*3u;break;
        case object_address:gp[ecx]=v;break;
        case tag:gp[edx]=v;break;
        // MOV ECX,[4F0414]; PUSH EBX; PUSH ESI; AND EDX,FFFF; SHL EDX,4.
        case tags:gp[ecx]=v;this->push(0x106060u,gp[ebx]);this->push(0x106061u,gp[esi]);
            gp[edx]=fl.shift(fl.logic(gp[edx]&0xffffu,4),4u,4);break;
        case definition:gp[edi]=v;break;
        case count:gp[edx]=logic::widen16(v);break;                                                 // MOVSX EDX,word
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // OR EBX,-1; OR ESI,-1; CALL 105950; TEST AL,AL.
    std::uint32_t active(Kept &kept,std::uint32_t) {
        auto &gp=this->gp;auto &fl=this->flags;
        gp[ebx]=fl.logic(gp[ebx]|logic::none,4);gp[esi]=fl.logic(kept.choice,4);
        this->call(0x106076u,0x10607bu,0x105950u,active_child);this->test8(gp[eax]);
        kept={gp[edi],gp[esi]};
        if(gp[eax]&0xffu)this->reached(Branch::active);
        return gp[eax]&0xffu;
    }
    // DEC EDX; XOR EAX,EAX; CMP EDX,-1; SETNE AL; DEC EAX; MOV ESI,EAX.
    void chose(const Kept &kept,std::uint32_t count_value) {
        auto &gp=this->gp;auto &fl=this->flags;
        gp[edx]=fl.increment(count_value,4,true);gp[eax]=fl.logic(0u,4);fl.minus(gp[edx],logic::none,4);
        gp[eax]=fl.increment(gp[edx]!=logic::none?1u:0u,4,true);gp[esi]=kept.choice;
        this->reached(count_value?Branch::counted:Branch::uncounted);
    }
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        Memory<InactiveChoice> memory{*this};
        const bool result=logic::inactive_choice::chosen(memory,*this,gp[eax]);
        // XOR EAX,EAX; POP EDI; CMP SI,-1; POP ESI; SETNE AL; POP EBX.
        gp[eax]=fl.logic(0u,4);gp[edi]=this->pop(0x106094u);fl.minus(gp[esi]&0xffffu,0xffffu,2);
        gp[esi]=this->pop(0x106099u);set_low_byte(gp[eax],result?1u:0u);gp[ebx]=this->pop(0x10609du);
        this->ret(0x10609eu,0);this->publish(0x10609eu);
    }
};
template<class State,class Reference,class Active> bool bridge(State &s,Reference reference,Active active) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x106040u,0x10609eu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return InactiveChoice<State,Active>(v,c,t,active);});
    return true;
}
} // namespace halo2::native_inactive_choice_detail
