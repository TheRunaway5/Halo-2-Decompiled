#pragma once
// Boundary adapter for the entry kind query, entered at 1062A0
// (native_tu15_entry_kind_systems.hpp). Its callers keep the original ABI,
// which the adapter owns: EAX the object handle; EDI, and around the walk
// EBP and ESI, saved; the result in AL (XOR AL,AL, MOV AL,1) above the rest
// of EAX as the original leaves it; ECX (the state cursor) and EDX (the
// remaining count) as the original leaves them; the flags of the last
// instruction that sets them (the DEC, with the ADD's carry); RET. Each
// access publishes its original instruction.
// HALO2_REFERENCE_ENTRY_KIND=1 restores the translated body.
// HALO2_VERIFY_ENTRY_KIND=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/units/native_tu15_entry_kind_systems.hpp"

namespace halo2::native_entry_kind_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_entry_kind;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_ENTRY_KIND");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_ENTRY_KIND");return p && *p;}();
    return value;
}
enum Branch : unsigned {no_entries, busy, other_kind, matched, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Entry kind verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State> struct EntryKind : Port<State> {
    using Access=logic::entry_kind::Access;
    bool walked{};
    EntryKind(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case headers:return 0x1062a0u;
        case header_datums:return 0x1062a6u;
        case object_address:return 0x1062b1u;
        case tag:return 0x1062b5u;
        case tags:return 0x1062b7u;
        case tag_definition:this->push(0x1062c5u,this->gp[edi]);return 0x1062c6u;                  // PUSH EDI
        case count:return 0x1062cau;
        case state:return 0x1062e0u;
        case entries:return 0x1062e5u;
        case kind:return 0x1062ebu;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case headers:gp[ecx]=v;break;
        case header_datums:gp[edx]=v;gp[eax]=fl.logic(gp[eax]&0xffffu,4)*3u;break;                 // AND EAX,FFFF; LEA
        case object_address:gp[ecx]=v;break;
        case tag:gp[eax]=v;break;
        case tags:gp[edx]=v;gp[eax]=fl.shift(fl.logic(gp[eax]&0xffffu,4),4u,4);break;              // AND EAX,FFFF; SHL EAX,4
        case tag_definition:gp[edi]=v;break;
        // MOV EDX,[EDI+2C8]; XOR AL,AL; TEST EDX,EDX.
        case count:gp[edx]=v;set_low_byte(gp[eax],fl.logic(0u,1));this->test32(v);
            if(logic::signed_value(v)<=0)this->reached(Branch::no_entries);break;
        case state:fl.minus(v&0xffu,0u,1);if(v&0xffu)this->reached(Branch::busy);break;            // CMP byte [ECX],0
        case entries:gp[ebp]=v;break;
        case kind:fl.minus(v&0xffffu,logic::wanted_kind,2);                                         // CMP word [ESI+EBP+6],5
            if((v&0xffffu)==logic::wanted_kind)this->reached(Branch::matched);
            else this->reached(Branch::other_kind);
            break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // PUSH EBP; PUSH ESI; XOR ESI,ESI; ADD ECX,20C.
    void walking() {
        auto &gp=this->gp;auto &fl=this->flags;
        this->push(0x1062d6u,gp[ebp]);this->push(0x1062d7u,gp[esi]);gp[esi]=fl.logic(0u,4);
        gp[ecx]=fl.plus(gp[ecx],logic::states,4);walked=true;
    }
    // ADD ECX,C; ADD ESI,40; DEC EDX.
    void stepped(std::uint32_t state,std::uint32_t offset,std::uint32_t remaining) {
        auto &gp=this->gp;auto &fl=this->flags;
        gp[ecx]=fl.plus(state,logic::state_size,4);gp[esi]=fl.plus(offset,logic::definition::entry_size,4);
        gp[edx]=fl.increment(remaining,4,true);
    }
    void run() {
        auto &gp=this->gp;
        Memory<EntryKind> memory{*this};
        const auto found=logic::entry_kind::any(memory,*this,gp[eax]);
        if(found)set_low_byte(gp[eax],1u);                                                         // MOV AL,1
        if(walked){gp[esi]=this->pop(0x1062feu);gp[ebp]=this->pop(0x1062ffu);}
        gp[edi]=this->pop(0x106300u);this->ret(0x106301u,0);this->publish(0x106301u);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x1062a0u,0x106301u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return EntryKind<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_entry_kind_detail
