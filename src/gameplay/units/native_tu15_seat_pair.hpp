#pragma once
// Boundary adapter for the seat pair check, entered at D3A80
// (native_tu15_seat_pair_systems.hpp). Its callers and the children keep the
// original ABI, which the adapter owns: the handle on the stack (RET 4);
// EBP, and past the seat names ESI and EDI, saved (DI the first seat
// meanwhile); 106710 gets EAX the occupant; E8E10 gets the handle pushed
// (and pops it) and ECX 0x13; every register each returns comes back; EAX,
// ECX and EDX as the original leaves them; the flags of the last
// instruction that sets them. Each access publishes its original
// instruction.
// HALO2_REFERENCE_SEAT_PAIR=1 restores the translated body.
// HALO2_VERIFY_SEAT_PAIR=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/units/native_tu15_seat_pair_systems.hpp"

namespace halo2::native_seat_pair_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_seat_pair;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_SEAT_PAIR");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_SEAT_PAIR");return p && *p;}();
    return value;
}
enum Branch : unsigned {unnamed_seat, both_answer, first_fails, second_fails, branch_count};
static_assert(unsigned(logic::seat_pair::Outcome::second_fails)==Branch::second_fails);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Seat pair verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State,class Answer,class Notify> struct SeatPair : Port<State> {
    using Access=logic::seat_pair::Access;
    using Kept=logic::seat_pair::Kept;
    Answer &answer_child;
    Notify &notify_child;
    std::uint32_t answers{};
    SeatPair(State &s,std::uint32_t *c,Trace<State> *t,Answer &a,Notify &n):Port<State>(s,c,t,{}),answer_child(a),notify_child(n){}
    // [ESP+8] after PUSH EBP.
    std::uint32_t argument()const{return this->gp[esp]+8u;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case headers:return 0xd3a80u;
        case handle:return 0xd3a87u;
        case datums:return 0xd3a95u;
        case object:return 0xd3a9bu;
        case first_named:return 0xd3a9fu;
        case second_named:return 0xd3aa8u;
        case datums_again:return 0xd3ab1u;
        case object_again:return 0xd3ab5u;
        case first_seat:return 0xd3abau;
        case first_occupant:gp[eax]=logic::widen16(gp[edi]);return 0xd3aceu;                        // MOVSX EAX,DI
        case datums_last:return 0xd3ad5u;
        case object_last:return 0xd3ad8u;
        case second_seat:return 0xd3adcu;
        case second_occupant:gp[edx]=logic::widen16(gp[edx]);return 0xd3af0u;                       // MOVSX EDX,DX
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case headers:gp[edx]=v;this->push(0xd3a86u,gp[ebp]);break;                                 // PUSH EBP
        // MOV EBP,[ESP+8]; MOV EAX,EBP; AND EAX,FFFF; LEA ECX,[EAX+EAX*2].
        case handle:gp[ebp]=v;gp[eax]=fl.logic(v&0xffffu,4);gp[ecx]=gp[eax]*3u;break;
        case datums:gp[eax]=v;gp[ecx]=fl.shift(gp[ecx],2u,4);break;                                // MOV EAX,[EDX+44]; SHL ECX,2
        case object:gp[eax]=v;break;
        case first_named:case second_named:fl.minus(v&0xffu,logic::unnamed,1);break;                // CMP byte,FF
        case datums_again:gp[eax]=v;this->push(0xd3ab4u,gp[esi]);break;                            // PUSH ESI
        case object_again:gp[esi]=v;this->push(0xd3ab9u,gp[edi]);break;                            // PUSH EDI
        // MOVSX DI,byte; OR EAX,-1; CMP DI,-1.
        case first_seat:gp[edi]=(gp[edi]&0xffff0000u)|logic::seat_pair::widen_byte_to_word(v);
            gp[eax]=fl.logic(gp[eax]|logic::none,4);fl.minus(gp[edi]&0xffffu,0xffffu,2);break;
        case first_occupant:gp[eax]=v;break;
        case datums_last:gp[edx]=v;break;
        case object_last:gp[ecx]=v;break;
        // MOVSX DX,byte; OR ESI,-1; CMP DX,-1.
        case second_seat:gp[edx]=(gp[edx]&0xffff0000u)|logic::seat_pair::widen_byte_to_word(v);
            gp[esi]=fl.logic(gp[esi]|logic::none,4);fl.minus(gp[edx]&0xffffu,0xffffu,2);break;
        case second_occupant:gp[esi]=v;break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // (MOV EAX,ESI before the second); CALL 106710; TEST AL,AL.
    bool answer(Kept &kept,std::uint32_t occupant) {
        auto &gp=this->gp;
        static constexpr std::uint32_t sites[]{0xd3af7u,0xd3b02u};
        gp[eax]=occupant;this->call(sites[answers],sites[answers]+5u,0x106710u,answer_child);++answers;
        this->test8(gp[eax]);
        kept={gp[ebp],gp[esi]};
        return (gp[eax]&0xffu)!=0;
    }
    // PUSH EBP; MOV ECX,13; CALL E8E10.
    void notify(Kept &kept) {
        auto &gp=this->gp;
        this->push(0xd3b0bu,kept.handle);gp[ecx]=logic::notice;this->call(0xd3b11u,0xd3b16u,0xe8e10u,notify_child);
        kept={gp[ebp],gp[esi]};
    }
    void run() {
        auto &gp=this->gp;
        using Outcome=logic::seat_pair::Outcome;
        Memory<SeatPair> memory{*this};
        const auto outcome=logic::seat_pair::check(memory,*this);
        this->reached(unsigned(outcome));
        if(outcome!=Outcome::unnamed_seat){gp[edi]=this->pop(0xd3b16u);gp[esi]=this->pop(0xd3b17u);}
        gp[ebp]=this->pop(0xd3b18u);this->ret(0xd3b19u,4u);this->publish(0xd3b19u);
    }
};
template<class State,class Reference,class Answer,class Notify> bool bridge(State &s,Reference reference,Answer answer,Notify notify) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0xd3a80u,0xd3b19u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return SeatPair<State,Answer,Notify>(v,c,t,answer,notify);});
    return true;
}
} // namespace halo2::native_seat_pair_detail
