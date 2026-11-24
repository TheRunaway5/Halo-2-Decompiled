#pragma once
// Boundary adapter for the slot handle, entered at CE230
// (native_tu15_object_slots_systems.hpp, slot_handle). Its callers keep the
// original ABI, which the adapter owns: EAX the object handle, CX the slot
// index; the handle in EAX; ECX (the index, MOVSX) and EDX (NONE, or the
// object) as the original leaves them; the flags of the last instruction
// that sets them; RET. Each access publishes its original instruction.
// HALO2_REFERENCE_OBJECT_SLOTS=1 restores the translated body.
// HALO2_VERIFY_OBJECT_SLOTS=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_object_slots_systems.hpp"

namespace halo2::native_slot_handle_detail {
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
enum Branch : unsigned {no_index, slotted, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Slot handle verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State> struct SlotHandle : Port<State> {
    using Access=logic::slot_handle::Access;
    SlotHandle(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case headers:return 0xce239u;
        case header_datums:return 0xce23fu;
        case object_address:return 0xce24au;
        case slot:return 0xce251u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case headers:gp[edx]=v;break;
        // MOV EDX,[EDX+44]; AND EAX,FFFF; LEA EAX,[EAX+EAX*2].
        case header_datums:gp[edx]=v;gp[eax]=this->flags.logic(gp[eax]&0xffffu,4)*3u;break;
        // MOV EDX,[...]; MOVSX ECX,CX.
        case object_address:gp[edx]=v;gp[ecx]=logic::widen16(gp[ecx]&0xffffu);break;
        case slot:gp[eax]=v;break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void run() {
        auto &gp=this->gp;
        // OR EDX,-1; CMP CX,-1.
        gp[edx]=this->flags.logic(gp[edx]|logic::none,4);this->flags.minus(gp[ecx]&0xffffu,logic::no_index16,2);
        const bool no_index=(gp[ecx]&0xffffu)==logic::no_index16;
        Memory<SlotHandle> memory{*this};
        const auto result=logic::slot_handle::of(memory,gp[eax],gp[ecx]);
        std::uint32_t exit=0xce258u;
        if(no_index){this->reached(Branch::no_index);gp[eax]=result;exit=0xce25bu;}                // MOV EAX,EDX (NONE)
        else this->reached(Branch::slotted);
        this->ret(exit,0);this->publish(exit);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0xce230u,0xce25bu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return SlotHandle<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_slot_handle_detail
