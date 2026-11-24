#pragma once
// Boundary adapter for the side count, 15B4D0
// (native_tu15_side_count_systems.hpp). Its callers keep the original ABI,
// which the adapter owns: no arguments (RET), the count in EAX; an 8-byte
// frame below the saved EBX, EBP, ESI and EDI; the registers and flags of
// the original's four-wide team walk and its players' scan, and its two
// exits. Each access publishes its original instruction.
// HALO2_REFERENCE_SIDE_COUNT=1 restores the translated body.
// HALO2_VERIFY_SIDE_COUNT=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/multiplayer/native_tu15_side_count_systems.hpp"

namespace halo2::native_side_count_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_side_count;
using Access=logic::side_count::Access;
using Outcome=logic::side_count::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_SIDE_COUNT");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_SIDE_COUNT");return v;}
enum Branch : unsigned {teams, players, scanned, empty_record, team_set, branch_count};
static_assert(unsigned(Outcome::empty_record)==Branch::empty_record);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Side count"};return v;}
inline constexpr std::uint32_t sites[]{0x15b4d6u,0x15b4dcu,0x15b4e2u,0x15b4f4u,0x15b4fau,0x15b503u,0u,0x15b5c0u,0x15b5d3u,0x15b5dau,
    0x15b5f1u,0x15b608u,0x15b60bu,0x15b613u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::record_store)+1u);
inline constexpr std::uint32_t team_word_sites[]{0x15b526u,0x15b54bu,0x15b56du,0x15b594u};
template<class State> struct Adapter : Port<State> {
    std::uint32_t depth{};      // bytes below the return address
    unsigned slot_now{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t local(std::int32_t offset)const{return this->gp[esp]+depth+std::uint32_t(offset);}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t take(std::uint32_t pc){depth-=4u;return this->pop(pc);}
    std::uint32_t instruction(Access a,std::uint32_t){return a==Access::team_word?team_word_sites[slot_now]:sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case session_address:gp[esi]=v;break;
        case variant_index:gp[ecx]=v;break;
        case variant_entry:case game_address:gp[edx]=v;break;
        case mode_flags:set_low_byte(gp[ebx],v);break;
        case players_array:gp[ebx]=v;break;
        case first_unused:gp[edi]=v;break;
        case bitmap:gp[esi]=v;break;
        case datum_size:gp[ecx]=v;break;
        case datum_data:gp[esi]=v;break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void cleared(std::uint32_t record) {                                                               // XOR EAX,EAX; TEST EDX,EDX; PUSH EDI
        auto &gp=this->gp;
        gp[eax]=this->flags.logic(0u,4);this->test32(record);put(0x15b4edu,gp[edi]);
    }
    void team_tested(std::uint32_t flag_value) {                                                       // AND BL,1; (MOV EDX,2)
        auto &gp=this->gp;
        set_low_byte(gp[ebx],this->flags.logic(flag_value,1));
        if(flag_value)gp[edx]=2u;
    }
    // TEST BL,BL; LEA ECX (or EDX itself); TEST; JS; CMP 8; JGE; MOVZX EDI,word; (MOV ECX,EDX); SHL; TEST EBP,EDI; SETNZ CL; TEST CL,CL; INC EAX.
    void team(unsigned slot,std::uint32_t team_index,bool inside,std::uint32_t word,bool set,std::uint32_t sides) {
        auto &gp=this->gp;auto &fl=this->flags;
        slot_now=slot;
        this->test8(gp[ebx]&0xffu);
        if(slot!=2u)gp[ecx]=team_index;
        this->test32(team_index);
        if(logic::signed_value(team_index)<0)return;
        this->compare32(team_index,logic::team_count);
        if(!inside)return;
        gp[edi]=word&0xffffu;
        if(slot==2u)gp[ecx]=team_index;
        gp[ebp]=fl.shift(1u,gp[ecx]&0xffu,4);fl.logic(gp[ebp]&gp[edi],4);
        set_low_byte(gp[ecx],set?1u:0u);this->test8(set?1u:0u);
        if(set){gp[eax]=fl.increment(sides-1u,4,false);this->reached(Branch::team_set);}
    }
    void grouped(std::uint32_t index) {                                                                // ADD EDX,4; LEA ECX,[EDX-2]; CMP ECX,8
        auto &gp=this->gp;
        gp[edx]=this->flags.plus(index-logic::group,logic::group,4);gp[ecx]=index-2u;this->compare32(gp[ecx],logic::team_count);
    }
    void walking(){this->gp[edx]=this->flags.logic(0xffffffffu,4);}                                   // OR EDX,-1
    void starting(std::uint32_t index){this->gp[edx]=this->flags.increment(index-1u,4,false);}        // INC EDX
    void limit_compared(std::uint32_t index,std::uint32_t limit){this->compare32(index,limit);}
    void bit_tested(std::uint32_t index,std::uint32_t word) {                                          // MOV ECX,EDX; AND ECX,1F; MOV EBP,1; SHL; MOV ECX,EDX; SAR ECX,5; TEST
        auto &gp=this->gp;auto &fl=this->flags;
        gp[ecx]=fl.logic(index&31u,4);gp[ebp]=fl.shift(1u,gp[ecx]&0xffu,4);gp[ecx]=fl.right(index,5u,4,true);fl.logic(word&gp[ebp],4);
    }
    void advanced(std::uint32_t index,std::uint32_t limit){this->gp[edx]=this->flags.increment(index-1u,4,false);this->compare32(index,limit);}
    void found_tested(std::uint32_t index){this->compare32(index,logic::none);}
    void record(std::uint32_t datum) {                                                                 // IMUL ECX,EDX; ADD ECX,ESI
        auto &gp=this->gp;auto &fl=this->flags;
        fl.product(gp[ecx],gp[edx]);gp[ecx]=fl.plus(datum-gp[esi],gp[esi],4);
    }
    void counted(std::uint32_t sides){this->gp[eax]=this->flags.increment(sides-1u,4,false);}         // INC EAX
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        gp[esp]=fl.minus(gp[esp],8u,4);depth=8u;                                                       // SUB ESP,8
        put(0x15b4d3u,gp[ebx]);put(0x15b4d4u,gp[ebp]);put(0x15b4d5u,gp[esi]);
        Memory<Adapter> memory{*this};
        std::uint32_t sides{};
        const auto outcome=logic::side_count::count(memory,*this,sides);
        this->reached(unsigned(outcome));
        gp[eax]=sides;
        const std::uint32_t at=outcome==Outcome::scanned?0x15b5fbu:0x15b5b8u;
        gp[edi]=take(at);gp[esi]=take(at+1u);gp[ebp]=take(at+2u);gp[ebx]=take(at+3u);
        gp[esp]=fl.plus(gp[esp],8u,4);                                                                 // ADD ESP,8
        this->ret(at+7u,0u);this->publish(at+7u);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    return run_owner(s,0x15b4d0u,0x15b602u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State>(v,c,t);});
}
} // namespace halo2::native_side_count_detail
