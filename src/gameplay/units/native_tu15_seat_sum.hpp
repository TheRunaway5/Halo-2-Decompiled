#pragma once
// Boundary adapter for the seat sum, entered at 1059C0
// (native_tu15_seat_sum_systems.hpp). Its callers and 10B0F0 keep the
// original ABI, which the adapter owns: EAX the index, the object handle and
// the skip byte on the stack (RET 8); EBX, EBP, ESI and EDI saved and the
// PUSH ECX slot holding the index scaled by 16 (ECX on return); 10B0F0 gets
// EAX the handle, EBX zero, and every register it returns comes back; the
// sum in EAX; EDX as the original leaves it; the flags of the last
// instruction that sets them. The four seats are unrolled in the original,
// each with its own instructions, and an occupant's word found through its
// definition in the last seat returns along a separate epilogue. Each access
// publishes its original instruction.
// HALO2_REFERENCE_SEAT_SUM=1 restores the translated body.
// HALO2_VERIFY_SEAT_SUM=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/units/native_tu15_seat_sum_systems.hpp"

namespace halo2::native_seat_sum_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_seat_sum;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_SEAT_SUM");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_SEAT_SUM");return p && *p;}();
    return value;
}
enum Branch : unsigned {linked, unlinked, own_past, no_vehicle, skipped, seat_empty, seat_self, seat_foreign, seat_past,
    seat_added, last_added, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Seat sum verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
// The instructions of one word lookup: the object's own (before the seats)
// and each seat's.
struct WordSites {std::uint32_t tags,definition,count,entries,entry_word,local,own_word;};
inline constexpr WordSites own_sites{0x1059fau,0x105a09u,0x105a0du,0x105a15u,0x105a20u,0x105a27u,0x105a2bu};
inline constexpr WordSites seat_sites[]{
    {0x105a8au,0x105a99u,0x105a9du,0x105aa5u,0x105ab0u,0x105ab7u,0x105abbu},
    {0x105af1u,0x105b00u,0x105b04u,0x105b0cu,0x105b17u,0x105b1eu,0x105b22u},
    {0x105b58u,0x105b67u,0x105b6bu,0x105b73u,0x105b7eu,0x105b85u,0x105b89u},
    {0x105bbfu,0x105bcdu,0x105bd1u,0x105bd9u,0x105be2u,0x105bf3u,0x105bf7u}};
struct SeatSites {std::uint32_t reload,seat,handle,occupant,occupant_tag,object_tag;};
inline constexpr SeatSites seats[]{
    {0u,0x105a62u,0x105a6du,0x105a7cu,0x105a80u,0x105a82u},
    {0x105ac5u,0x105ac9u,0x105ad4u,0x105ae3u,0x105ae7u,0x105ae9u},
    {0x105b2cu,0x105b30u,0x105b3bu,0x105b4au,0x105b4eu,0x105b50u},
    {0x105b93u,0x105b97u,0x105ba2u,0x105bb1u,0x105bb5u,0x105bb7u}};
inline constexpr std::uint32_t last_seat=3u;
template<class State,class Linked> struct SeatSum : Port<State> {
    using Access=logic::seat_sum::Access;
    using Kept=logic::seat_sum::Kept;
    Linked &linked_child;
    bool saved{},in_seats{},through_definition{},last_epilogue{};
    std::uint32_t k{};
    SeatSum(State &s,std::uint32_t *c,Trace<State> *t,Linked &l):Port<State>(s,c,t,{}),linked_child(l){}
    // The stack slots after all five pushes; the handle is first read at
    // 1059C1, after PUSH ECX, but its address is taken before that push.
    std::uint32_t handle_slot()const{return this->gp[esp]+(saved?0x18u:4u);}
    std::uint32_t skip_slot()const{return this->gp[esp]+0x1cu;}
    std::uint32_t local_slot()const{return this->gp[esp]+0x10u;}
    const WordSites &sites()const{return in_seats?seat_sites[k]:own_sites;}
    bool last()const{return in_seats && k==last_seat;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case handle:this->push(0x1059c0u,gp[ecx]);return 0x1059c1u;                                // PUSH ECX
        case headers:this->push(0x1059c5u,gp[ebx]);this->push(0x1059c6u,gp[ebp]);return 0x1059c7u;   // PUSH EBX; PUSH EBP
        case header_datums:return 0x1059cdu;
        case object_address:return 0x1059deu;
        case local_store:return 0x1059e7u;
        case tag:return 0x1059f8u;
        case tags:return sites().tags;
        case definition:return sites().definition;
        case count:return sites().count;
        case entries:return sites().entries;
        case entry_word:return sites().entry_word;
        case local:return sites().local;
        case own_word:return sites().own_word;
        case vehicle:return 0x105a33u;
        case skip:return 0x105a42u;
        case vehicle_datums:return 0x105a4eu;
        case vehicle_address:return 0x105a5au;
        case vehicle_store:return 0x105a5eu;
        case vehicle_reload:return seats[k].reload;
        case seat:return seats[k].seat;
        case seat_handle:return seats[k].handle;
        // AND ECX,FFFF; LEA EDX,[ECX+ECX*2] before MOV EDX,[EBP+EDX*4+8].
        case occupant:gp[ecx]=fl.logic(gp[ecx]&0xffffu,4);gp[edx]=gp[ecx]*3u;return seats[k].occupant;
        case occupant_tag:return seats[k].occupant_tag;
        case object_tag:return seats[k].object_tag;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case handle:gp[ecx]=v;break;
        case headers:gp[ebp]=v;break;
        // MOV EDX,[EBP+44]; PUSH ESI; MOV ESI,EAX; MOV EAX,ECX; AND EAX,FFFF; LEA; PUSH EDI.
        case header_datums:gp[edx]=v;this->push(0x1059d0u,gp[esi]);gp[esi]=gp[eax];
            gp[eax]=fl.logic(gp[ecx]&0xffffu,4)*3u;this->push(0x1059ddu,gp[edi]);saved=true;break;
        case object_address:gp[edi]=v;gp[eax]=fl.shift(gp[esi],logic::object::word_stride_shift,4);break;   // MOV EAX,ESI; SHL EAX,4
        case tag:gp[ecx]=v;break;
        // MOV EDX (EAX in the last seat),[4F0414]; AND ECX,FFFF; SHL ECX,4.
        case tags:if(last())gp[eax]=v;else gp[edx]=v;gp[ecx]=fl.shift(fl.logic(gp[ecx]&0xffffu,4),4u,4);break;
        case definition:gp[ecx]=v;break;
        case count:fl.minus(v,gp[esi],4);                                                          // CMP [ECX+2C0],ESI
            if(logic::signed_value(v)<=logic::signed_value(gp[esi]))this->reached(in_seats?Branch::seat_past:Branch::own_past);break;
        // MOV ECX,[ECX+2C4]; MOV EDX,ESI; IMUL EDX,EDX,5C (IMUL ESI,ESI,5C in the last seat).
        case entries:gp[ecx]=v;if(last())gp[esi]=fl.product(gp[esi],logic::definition::entry_size);
            else gp[edx]=fl.product(gp[esi],logic::definition::entry_size);break;
        // MOVSX EBX (the object's) or EDX (an occupant's).
        case entry_word:if(in_seats)gp[edx]=logic::widen16(v);else gp[ebx]=logic::widen16(v);break;
        // MOV EDX (the object's), ECX or EAX (the last seat's),[ESP+10].
        case local:if(!in_seats)gp[edx]=v;else if(last())gp[eax]=v;else gp[ecx]=v;break;
        // MOVSX EBX (the object's), EDX or ECX (the last seat's).
        case own_word:if(!in_seats)gp[ebx]=logic::widen16(v);else if(last())gp[ecx]=logic::widen16(v);else gp[edx]=logic::widen16(v);break;
        case vehicle:gp[ecx]=v;fl.minus(v,logic::none,4);if(v==logic::none)this->reached(Branch::no_vehicle);break;
        case skip:set_low_byte(gp[edx],v);this->test8(v);if(v&0xffu)this->reached(Branch::skipped);break;   // MOV DL; TEST DL,DL
        // MOV EBP,[EBP+44]; AND ECX,FFFF; LEA ECX,[ECX+ECX*2].
        case vehicle_datums:gp[ebp]=v;gp[ecx]=fl.logic(gp[ecx]&0xffffu,4)*3u;break;
        case vehicle_address:case vehicle_reload:gp[ecx]=v;break;
        case seat:gp[ecx]=v;fl.minus(v,logic::none,4);if(v==logic::none)this->reached(Branch::seat_empty);break;
        case seat_handle:fl.minus(gp[ecx],v,4);if(gp[ecx]==v)this->reached(Branch::seat_self);break;   // CMP ECX,[ESP+18]
        case occupant:gp[edx]=v;break;
        case occupant_tag:gp[ecx]=v;break;
        // CMP ECX,[EDI]; then TEST AL,AL.
        case object_tag:fl.minus(gp[ecx],v,4);if(gp[ecx]==v)this->test8(gp[eax]);else this->reached(Branch::seat_foreign);break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // MOV EAX,ECX; XOR EBX,EBX; CALL 10B0F0; TEST AL,AL.
    std::uint32_t linked(Kept &kept,std::uint32_t object_handle) {
        auto &gp=this->gp;
        gp[eax]=object_handle;gp[ebx]=this->flags.logic(0u,4);
        this->call(0x1059efu,0x1059f4u,0x10b0f0u,linked_child);this->test8(gp[eax]);
        kept={gp[edi],gp[esi],gp[ebp],gp[ebx]};through_definition=(gp[eax]&0xffu)!=0;
        this->reached(through_definition?Branch::linked:Branch::unlinked);
        return gp[eax]&0xffu;
    }
    void seat(std::uint32_t index){in_seats=true;k=index;}
    // ADD EBX,EDX (ECX for the last seat's own word); the last seat's
    // definition word pops EDI and ESI first.
    void added(const Kept &kept,std::uint32_t word) {
        auto &gp=this->gp;
        if(last() && through_definition) {
            gp[edi]=this->pop(0x105be7u);gp[esi]=this->pop(0x105be8u);last_epilogue=true;this->reached(Branch::last_added);
        } else this->reached(Branch::seat_added);
        this->flags.plus(gp[ebx],word,4);gp[ebx]=kept.sum;
    }
    void run() {
        auto &gp=this->gp;
        Memory<SeatSum> memory{*this};
        const auto total=logic::seat_sum::sum(memory,*this,gp[eax]);
        if(last_epilogue) {
            gp[ebp]=this->pop(0x105bebu);gp[eax]=total;gp[ebx]=this->pop(0x105beeu);gp[ecx]=this->pop(0x105befu);
            this->ret(0x105bf0u,8u);this->publish(0x105bf0u);return;
        }
        gp[edi]=this->pop(0x105c01u);gp[esi]=this->pop(0x105c02u);gp[ebp]=this->pop(0x105c03u);gp[eax]=total;
        gp[ebx]=this->pop(0x105c06u);gp[ecx]=this->pop(0x105c07u);this->ret(0x105c08u,8u);this->publish(0x105c08u);
    }
};
template<class State,class Reference,class Linked> bool bridge(State &s,Reference reference,Linked linked) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x1059c0u,0x105c08u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return SeatSum<State,Linked>(v,c,t,linked);});
    return true;
}
} // namespace halo2::native_seat_sum_detail
