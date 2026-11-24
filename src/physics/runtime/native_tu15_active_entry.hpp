#pragma once
// Boundary adapter for the active entry, entered at 105950
// (native_tu15_active_entry_systems.hpp). Its callers keep the original ABI,
// which the adapter owns: EAX the object handle, EBX the entry or NONE;
// ESI and EDI saved (the object, the count); the result in AL (above it the
// rest of EAX as the original leaves it); ECX the index and EDX the last
// state as the original leaves them; the flags of the last instruction that
// sets them; RET. Each access publishes its original instruction.
// HALO2_REFERENCE_ACTIVE_ENTRY=1 restores the translated body.
// HALO2_VERIFY_ACTIVE_ENTRY=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_active_entry_systems.hpp"

namespace halo2::native_active_entry_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_active_entry;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_ACTIVE_ENTRY");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_ACTIVE_ENTRY");return p && *p;}();
    return value;
}
enum Branch : unsigned {no_entries, other_entry, inactive, active, none_active, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Active entry verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State> struct ActiveEntry : Port<State> {
    using Access=logic::active_entry::Access;
    ActiveEntry(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case headers:return 0x105950u;
        case header_datums:return 0x105956u;
        case instances:return 0x105959u;
        case object_address:return 0x105968u;
        case object_tag:return 0x10596cu;
        case definition:return 0x105976u;
        case count:return 0x10597bu;
        case state:return 0x10599fu;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case headers:gp[ecx]=v;break;
        case header_datums:gp[edx]=v;break;
        // MOV ECX,[4F0414]; AND EAX,FFFF; LEA EAX,[EAX+EAX*2]; PUSH ESI.
        case instances:gp[ecx]=v;gp[eax]=fl.logic(gp[eax]&0xffffu,4)*3u;this->push(0x105967u,gp[esi]);break;
        case object_address:gp[esi]=v;break;
        case object_tag:gp[eax]=fl.shift(fl.logic(v&0xffffu,4),logic::tag_instance_shift,4);break;  // AND; SHL EAX,4
        case definition:gp[edx]=v;this->push(0x10597au,gp[edi]);break;                              // PUSH EDI
        // MOV EDI,[EDX+2C0]; XOR AL,AL; XOR ECX,ECX; TEST EDI,EDI.
        case count:
            gp[edi]=v;set_low_byte(gp[eax],fl.logic(0u,1));gp[ecx]=fl.logic(0u,4);this->test32(v);
            if(logic::signed_value(v)<=0)this->reached(Branch::no_entries);break;
        // MOVSX EDX,word; TEST EDX,EDX; (CMP EDX,6).
        case state: {
            gp[edx]=logic::widen16(v);this->test32(gp[edx]);
            const auto value=logic::signed_value(gp[edx]);
            if(value>0)fl.minus(gp[edx],logic::last_active,4);
            this->reached(value>0 && value<=std::int32_t(logic::last_active)?Branch::active:Branch::inactive);
            break;
        }
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // CMP EBX,-1; (CMP EBX,ECX); then MOVSX EDX,CX; SHL EDX,4 for an entry looked at.
    void looked_at(std::uint32_t index) {
        auto &gp=this->gp;auto &fl=this->flags;
        fl.minus(gp[ebx],logic::none,4);
        if(gp[ebx]!=logic::none)fl.minus(gp[ebx],gp[ecx],4);
        gp[edx]=fl.shift(logic::widen16(index&0xffffu),logic::entry_shift,4);
    }
    // (CMP EBX,-1; CMP EBX,ECX for an entry not looked at); INC ECX; CMP ECX,EDI.
    void stepped(std::uint32_t index) {
        auto &gp=this->gp;auto &fl=this->flags;
        if(gp[ebx]!=logic::none && gp[ebx]!=index){fl.minus(gp[ebx],logic::none,4);fl.minus(gp[ebx],gp[ecx],4);this->reached(Branch::other_entry);}
        gp[ecx]=fl.increment(gp[ecx],4,false);fl.minus(gp[ecx],gp[edi],4);
    }
    void run() {
        auto &gp=this->gp;
        Memory<ActiveEntry> memory{*this};
        const bool found=logic::active_entry::any(memory,*this,gp[eax],gp[ebx]);
        const bool empty=logic::signed_value(gp[edi])<=0;
        std::uint32_t exit;
        if(found){set_low_byte(gp[eax],1u);exit=0x1059bcu;gp[edi]=this->pop(0x1059bau);gp[esi]=this->pop(0x1059bbu);}
        else if(empty){exit=0x1059bcu;gp[edi]=this->pop(0x1059bau);gp[esi]=this->pop(0x1059bbu);}
        else{this->reached(Branch::none_active);exit=0x1059b7u;gp[edi]=this->pop(0x1059b5u);gp[esi]=this->pop(0x1059b6u);}
        this->ret(exit,0);this->publish(exit);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x105950u,0x1059bcu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return ActiveEntry<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_active_entry_detail
