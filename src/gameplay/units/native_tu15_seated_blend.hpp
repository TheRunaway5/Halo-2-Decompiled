#pragma once
// Boundary adapter for the seated blend, 113B70
// (native_tu15_seated_blend_systems.hpp). Its callers and the children keep
// the original ABI, which the adapter owns: EDI the unit (RET), the answer
// in AL; ESI (and EBX past the seat) saved; C9550 (ECX the unit) and 113C70
// (EAX the unit, 2 pushed, RET 4) direct, every register each returns coming
// back; the flags of the original's binary search over the action names.
// Each access publishes its original instruction.
// HALO2_REFERENCE_SEATED_BLEND=1 restores the translated body.
// HALO2_VERIFY_SEATED_BLEND=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/units/native_tu15_seated_blend_systems.hpp"

namespace halo2::native_seated_blend_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_seated_blend;
using Access=logic::seated_blend::Access;
using Kept=logic::seated_blend::Kept;
using Outcome=logic::seated_blend::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_SEATED_BLEND");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_SEATED_BLEND");return v;}
enum Branch : unsigned {unseated, partial, full, overruled, restricted_unit, named_action, branch_count};
static_assert(unsigned(Outcome::overruled)==Branch::overruled);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Seated blend"};return v;}
inline constexpr std::uint32_t sites[]{0x113b70u,0x113b76u,0x113b84u,0x113b88u,0x113b98u,0x113bb7u,0x113bbeu};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::action)+1u);
template<class State,class Restricted,class LastWord> struct Adapter : Port<State> {
    Restricted &restricted_child;
    LastWord &last_word_child;
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Restricted &r,LastWord &l):Port<State>(s,c,t,{}),restricted_child(r),last_word_child(l){}
    void refresh(Kept &kept){kept={this->gp[ebx]&0xffu,this->gp[esi],this->gp[edi]};}
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case headers:gp[ecx]=v;break;
        case datums:                                                                                   // MOV EAX,EDI; AND EAX,FFFF; LEA EAX,[EAX+EAX*2]; PUSH ESI
            gp[edx]=v;gp[eax]=3u*fl.logic(gp[edi]&0xffffu,4);this->push(0x113b83u,gp[esi]);break;
        case object_address:gp[esi]=v;break;
        case parent:gp[edx]=v;break;
        case animation:gp[eax]=logic::widen16(v);break;
        case action:gp[eax]=v;break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void parent_tested(std::uint32_t parent_value) {                                                   // OR ECX,-1; XOR AL,AL; CMP EDX,ECX
        auto &gp=this->gp;auto &fl=this->flags;
        gp[ecx]=fl.logic(0xffffffffu,4);set_low_byte(gp[eax],fl.logic(0u,1));this->compare32(parent_value,gp[ecx]);
    }
    void seat_tested(std::uint32_t seat_value){this->flags.minus(seat_value&0xffffu,0xffffu,2);}       // CMP word [ESI+1FC],CX
    std::uint32_t restricted(Kept &kept) {                                                             // PUSH EBX; MOV ECX,EDI; MOV BL,1; CALL C9550
        auto &gp=this->gp;
        this->push(0x113ba5u,gp[ebx]);gp[ecx]=kept.unit;set_low_byte(gp[ebx],kept.answer);
        this->call(0x113baau,0x113bafu,0xc9550u,restricted_child);
        refresh(kept);
        this->test8(gp[eax]&0xffu);
        if(gp[eax]&0xffu)this->reached(Branch::restricted_unit);
        return gp[eax];
    }
    // The original's compares over the action (signed JG/JL/JLE; three SUBs in one branch).
    void action_compared(std::uint32_t action) {
        auto &gp=this->gp;auto &fl=this->flags;
        const auto above=[&](std::uint32_t c){this->compare32(action,c);return std::int32_t(action)>std::int32_t(c);};
        const auto equal=[&](std::uint32_t c){this->compare32(action,c);return action==c;};
        if(above(0xa000767u)) {
            if(above(0xe00002au)){if(!equal(0xe00003bu) && !equal(0xe0000c3u))equal(0xf00003au);}
            else if(action!=0xe00002au) {
                gp[eax]=fl.minus(action,0xc000043u,4);
                if(gp[eax]){gp[eax]=fl.minus(gp[eax],0xffffe8u,4);if(gp[eax])gp[eax]=fl.minus(gp[eax],0x17u,4);}
            }
        } else if(action!=0xa000767u) {
            if(above(0x600008cu)){if(!equal(0x700076eu) && !equal(0x80000c4u))equal(0x800076fu);}
            else if(action!=0x600008cu) {
                if(above(0x50000c3u)){this->compare32(action,0x5000768u);if(!(std::int32_t(action)<0x5000768))this->compare32(action,0x500076bu);}
                else if(action!=0x50000c3u && !equal(0x400004au)) {
                    this->compare32(action,0x400076bu);
                    if(std::int32_t(action)>0x400076b)this->compare32(action,0x400076du);
                }
            }
        }
        if(logic::partial(action))this->reached(Branch::named_action);
    }
    void cleared(const Kept &kept){set_low_byte(this->gp[ebx],this->flags.logic(kept.answer,1));}      // XOR BL,BL
    std::uint32_t last_word(Kept &kept) {                                                              // PUSH 2; MOV EAX,EDI; CALL 113C70
        auto &gp=this->gp;
        this->push(0x113c54u,logic::last_word_mode);gp[eax]=kept.unit;
        this->call(0x113c58u,0x113c5du,0x113c70u,last_word_child);
        refresh(kept);
        this->test8(gp[eax]&0xffu);
        return gp[eax];
    }
    void answered(const Kept &kept){set_low_byte(this->gp[eax],kept.answer);}                          // MOV AL,BL
    void run() {
        auto &gp=this->gp;
        Memory<Adapter> memory{*this};
        const auto outcome=logic::seated_blend::full(memory,*this,Kept{gp[ebx],0u,gp[edi]});
        this->reached(unsigned(outcome));
        if(outcome==Outcome::overruled){gp[ebx]=this->pop(0x113c61u);gp[esi]=this->pop(0x113c62u);this->ret(0x113c63u,0u);this->publish(0x113c63u);return;}
        if(outcome!=Outcome::unseated)gp[ebx]=this->pop(0x113c66u);
        gp[esi]=this->pop(0x113c67u);this->ret(0x113c68u,0u);this->publish(0x113c68u);
    }
};
template<class State,class Reference,class Restricted,class LastWord> bool bridge(State &s,Reference reference,Restricted restricted,LastWord last_word) {
    return run_owner(s,0x113b70u,0x113c68u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Restricted,LastWord>(v,c,t,restricted,last_word);});
}
} // namespace halo2::native_seated_blend_detail
