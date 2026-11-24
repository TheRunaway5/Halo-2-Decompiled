#pragma once
// Boundary adapter for the repeat tracker, entered at 1F0F90
// (native_tu15_repeat_tracker_systems.hpp). Its callers keep the original
// ABI, which the adapter owns: EAX the tracker, ECX the source; ESI and EDI
// saved (popped after the first kind test); EDX as the original leaves it
// (DX the tracker's kind, its upper word zero after a restart); the flags of
// the last instruction that sets them (the INC of the count included);
// RET. Each access publishes its original instruction.
// HALO2_REFERENCE_REPEAT_TRACKER=1 restores the translated body.
// HALO2_VERIFY_REPEAT_TRACKER=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/objects/native_tu15_repeat_tracker_systems.hpp"

namespace halo2::native_repeat_tracker_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_repeat_tracker;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_REPEAT_TRACKER");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_REPEAT_TRACKER");return p && *p;}();
    return value;
}
enum Branch : unsigned {uncounted, unflagged, counted, restarted, branch_count};
static_assert(unsigned(logic::repeat_tracker::Outcome::counted)==Branch::counted);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Repeat tracker verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State> struct RepeatTracker : Port<State> {
    using Access=logic::repeat_tracker::Access;
    bool restored{};
    RepeatTracker(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case source_kind:return 0x1f0f90u;
        case tracker_kind:this->push(0x1f0f94u,this->gp[esi]);this->push(0x1f0f95u,this->gp[edi]);return 0x1f0f96u;   // PUSH ESI; PUSH EDI
        case source_handle:return 0x1f0fa0u;
        case tracker_handle:return 0x1f0fa3u;
        case kind_store:this->reached(Branch::restarted);return 0x1f0fa8u;
        case handle_read:return 0x1f0fabu;
        case handle_store:return 0x1f0faeu;
        case level:return 0x1f0fb1u;
        case level_store:return 0x1f0fb6u;
        case count_clear:this->gp[edx]=this->flags.logic(0u,4);return 0x1f0fbcu;                    // XOR EDX,EDX
        case second_clear:return 0x1f0fc0u;
        case kind_check:return 0x1f0fc4u;
        case source_flags:return 0x1f0fe7u;
        case count:case count_store:return 0x1f0fedu;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case source_kind:gp[edx]=(gp[edx]&0xffff0000u)|(v&0xffffu);break;                          // MOV DX,[ECX+28]
        // MOVZX EDI,word [EAX]; MOVSX ESI,DX; CMP ESI,EDI.
        case tracker_kind:gp[edi]=v&0xffffu;gp[esi]=logic::widen16(gp[edx]);fl.minus(gp[esi],gp[edi],4);break;
        case source_handle:gp[esi]=v;break;
        case tracker_handle:fl.minus(gp[esi],v,4);break;                                            // CMP ESI,[EAX+4]
        case handle_read:gp[edx]=v;break;
        case level:gp[edx]=(gp[edx]&0xffff0000u)|(v&0xffu);break;                                   // MOVZX DX,byte
        case kind_check:gp[edx]=(gp[edx]&0xffff0000u)|(v&0xffffu);break;
        case source_flags:this->test8(v&logic::repeat_flag);break;                                  // TEST byte [ECX+1C],8
        case count:fl.increment(v&0xffffu,2,false);break;                                           // INC word [EAX+8]
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // CMP DX,kind; POP EDI; POP ESI after the first.
    void compared(std::uint32_t kind,std::uint32_t wanted) {
        auto &gp=this->gp;
        this->flags.minus(kind,wanted,2);
        if(!restored){gp[edi]=this->pop(0x1f0fcbu);gp[esi]=this->pop(0x1f0fccu);restored=true;}
    }
    void run() {
        auto &gp=this->gp;
        Memory<RepeatTracker> memory{*this};
        this->reached(unsigned(logic::repeat_tracker::track(memory,*this,gp[eax],gp[ecx])));
        this->ret(0x1f0ff1u,0);this->publish(0x1f0ff1u);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x1f0f90u,0x1f0ff1u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return RepeatTracker<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_repeat_tracker_detail
