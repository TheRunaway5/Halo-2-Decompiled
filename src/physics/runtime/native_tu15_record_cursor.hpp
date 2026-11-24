#pragma once
// Boundary adapter for the record cursor step, entered at 1E5160
// (native_tu15_record_cursor_systems.hpp). Its callers and 1E5C80 keep the
// original ABI, which the adapter owns: EAX the cursor, EDX the list; ESI
// and EDI saved; 1E5C80 gets the handle pushed (and pops it), and every
// register it returns comes back; the result in AL (XOR AL,AL; MOV AL,1)
// above the rest of EAX; ECX (the index word, then the record's +0x1C) as
// the original leaves it; the flags of the last instruction that sets them
// (the CMP of the handle's high word reaches the JZ across the copies).
// Each access publishes its original instruction.
// HALO2_REFERENCE_RECORD_CURSOR=1 restores the translated body.
// HALO2_VERIFY_RECORD_CURSOR=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_record_cursor_systems.hpp"

namespace halo2::native_record_cursor_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_record_cursor;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_RECORD_CURSOR");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_RECORD_CURSOR");return p && *p;}();
    return value;
}
enum Branch : unsigned {ended, copied, notified, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Record cursor verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State,class Notify> struct RecordCursor : Port<State> {
    using Access=logic::record_cursor::Access;
    Notify &notify_child;
    std::uint32_t k{};
    RecordCursor(State &s,std::uint32_t *c,Trace<State> *t,Notify &n):Port<State>(s,c,t,{}),notify_child(n){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        static constexpr std::uint32_t copy_loads[]{0x1e5188u,0x1e518eu,0x1e5194u,0x1e5199u,0x1e51a2u,0x1e51a8u,0x1e51aeu,0x1e51b4u},
            copy_stores[]{0x1e518bu,0x1e5191u,0x1e5197u,0x1e519cu,0x1e51a5u,0x1e51abu,0x1e51b1u,0x1e51b7u};
        switch(a) {
        case index_word:return 0x1e5160u;
        // PUSH EDI before CMP ESI,[EDX+3C].
        case count:this->push(0x1e516au,gp[edi]);return 0x1e516bu;
        case index_store:return 0x1e5170u;
        case records:return 0x1e5174u;
        // LEA ECX,[ESI+ESI*4]; LEA ECX,[EDI+ECX*8] before MOV EDI,[ECX+4].
        case handle:gp[ecx]=gp[edi]+8u*(gp[esi]*5u);return 0x1e517du;
        case handle_store:return 0x1e5180u;
        case handle_high:return 0x1e5183u;
        case copy:return copy_loads[k];
        case copy_store:return copy_stores[k];
        case index:return 0x1e519fu;
        case handle_again:return 0x1e51bcu;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        // MOV CX,[EAX+2A]; INC CX; PUSH ESI; MOVSX ESI,CX.
        case index_word:gp[ecx]=(gp[ecx]&0xffff0000u)|fl.increment(v&0xffffu,2,false);this->push(0x1e5166u,gp[esi]);
            gp[esi]=logic::widen16(gp[ecx]);break;
        case count:fl.minus(gp[esi],v,4);if(logic::signed_value(gp[esi])>=logic::signed_value(v))this->reached(Branch::ended);break;
        case records:gp[edi]=v;break;
        case handle:gp[edi]=v;break;
        case handle_high:fl.minus(v&0xffffu,0xffffu,2);break;                                      // CMP word [EAX+26],-1
        // MOV EDI (the first four), ESI (the next three), ECX (the last).
        case copy:if(k<4)gp[edi]=v;else if(k<7)gp[esi]=v;else gp[ecx]=v;break;
        case handle_again:gp[eax]=v;break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void flagged(std::uint32_t){}
    void copied(std::uint32_t index){k=index;}
    // PUSH EAX; CALL 1E5C80.
    void notify(std::uint32_t handle) {
        this->push(0x1e51bfu,handle);this->call(0x1e51c0u,0x1e51c5u,0x1e5c80u,notify_child);this->reached(Branch::notified);
    }
    void run() {
        auto &gp=this->gp;
        Memory<RecordCursor> memory{*this};
        if(logic::record_cursor::advance(memory,*this,gp[eax],gp[edx])) {
            this->reached(Branch::copied);
            gp[edi]=this->pop(0x1e51c5u);set_low_byte(gp[eax],1u);gp[esi]=this->pop(0x1e51c8u);       // POP EDI; MOV AL,1; POP ESI
            this->ret(0x1e51c9u,0);this->publish(0x1e51c9u);return;
        }
        gp[edi]=this->pop(0x1e51cau);set_low_byte(gp[eax],this->flags.logic(0u,1));gp[esi]=this->pop(0x1e51cdu);   // XOR AL,AL
        this->ret(0x1e51ceu,0);this->publish(0x1e51ceu);
    }
};
template<class State,class Reference,class Notify> bool bridge(State &s,Reference reference,Notify notify) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x1e5160u,0x1e51ceu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return RecordCursor<State,Notify>(v,c,t,notify);});
    return true;
}
} // namespace halo2::native_record_cursor_detail
