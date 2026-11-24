#pragma once
// Boundary adapter for the both-slots test, entered at CFB40
// (native_tu15_object_slots_systems.hpp, both_slots). Its callers keep the
// original ABI, which the adapter owns: EAX the object handle; EBX saved
// (BL the first index); the result in AL above the rest of EAX as the
// original leaves it; ECX (the object) and EDX (DL NONE) as the original
// leaves them; the flags of the last CMP; RET. Each access publishes its
// original instruction.
// HALO2_REFERENCE_OBJECT_SLOTS=1 restores the translated body.
// HALO2_VERIFY_OBJECT_SLOTS=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_object_slots_systems.hpp"

namespace halo2::native_both_slots_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_object_slots;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_OBJECT_SLOTS");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_OBJECT_SLOTS");return p && *p;}();
    return value;
}
enum Branch : unsigned {first_empty, second_empty, both_set, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Both slots verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State> struct BothSlots : Port<State> {
    using Access=logic::both_slots::Access;
    BothSlots(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case headers:return 0xcfb40u;
        case header_datums:return 0xcfb46u;
        case object_address:return 0xcfb51u;
        case first:this->push(0xcfb55u,this->gp[ebx]);return 0xcfb56u;                                 // PUSH EBX
        case second:return 0xcfb65u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case headers:case object_address:gp[ecx]=v;break;
        // MOV EDX,[ECX+44]; AND EAX,FFFF; LEA EAX,[EAX+EAX*2].
        case header_datums:gp[edx]=v;gp[eax]=fl.logic(gp[eax]&0xffffu,4)*3u;break;
        // MOV BL; OR DL,FF; XOR AL,AL; CMP BL,DL.
        case first:
            set_low_byte(gp[ebx],v);set_low_byte(gp[edx],fl.logic((gp[edx]|0xffu)&0xffu,1));
            set_low_byte(gp[eax],fl.logic(0u,1));fl.minus(v&0xffu,gp[edx]&0xffu,1);
            if((v&0xffu)==logic::no_index8)this->reached(Branch::first_empty);break;
        case second:fl.minus(v&0xffu,gp[edx]&0xffu,1);                                              // CMP byte,DL
            this->reached((v&0xffu)==logic::no_index8?Branch::second_empty:Branch::both_set);break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void run() {
        auto &gp=this->gp;
        Memory<BothSlots> memory{*this};
        if(logic::both_slots::set(memory,gp[eax]))set_low_byte(gp[eax],1u);                          // MOV AL,1
        gp[ebx]=this->pop(0xcfb6fu);this->ret(0xcfb70u,0);this->publish(0xcfb70u);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0xcfb40u,0xcfb70u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return BothSlots<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_both_slots_detail
